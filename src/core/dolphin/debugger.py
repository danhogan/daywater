"""Minimal GDB Remote Serial Protocol client for Dolphin's built-in GDB stub.

Dolphin embeds a GDB stub that speaks the standard GDB RSP over TCP. When
enabled via ``GDBPort=<port>`` in ``Dolphin.ini``, Dolphin starts with the
CPU paused, waiting for a debugger to connect and send ``c`` (continue).

This module provides just enough RSP to:
- Connect and continue execution
- Single-step a PowerPC instruction while stopped
- Set/remove execution breakpoints (``Z0``/``z0``)
- Set/remove hardware write watchpoints (``Z2``/``z2``)
- Read the program counter (PC) when a watchpoint fires
- Collect unique PCs that write to a given address

This is the key primitive for the noclip workflow: set a write watchpoint on
the player position address, let the game run, and discover which functions
modify that address. The agent can then decompile those functions via Ghidra.

GDB RSP packet format::

    $<data>#<2-hex-checksum>

Responses are preceded by ``+`` (ACK) or ``-`` (NAK).
"""

from __future__ import annotations

import re
import socket
import subprocess
import time
from dataclasses import dataclass, field
from pathlib import Path

from src.core.logging import logger


def _read_log_tail(log_path: Path | None, lines: int = 50) -> str:
    """Read the last *lines* lines from a log file, or return a placeholder."""
    if log_path is None or not log_path.exists():
        return "(no dolphin.log available)"
    try:
        text = log_path.read_text(errors="replace")
        tail = "\n".join(text.splitlines()[-lines:])
        return tail or "(dolphin.log is empty)"
    except OSError:
        return "(could not read dolphin.log)"


def _checksum(data: str) -> str:
    """Compute GDB RSP checksum (sum of bytes mod 256, as 2-char hex)."""
    return f"{sum(ord(c) for c in data) % 256:02x}"


def _make_packet(data: str) -> bytes:
    """Wrap data in GDB RSP packet framing: $<data>#<checksum>."""
    return f"${data}#{_checksum(data)}".encode("ascii")


class GDBError(Exception):
    """GDB protocol or connection error."""


class DolphinDiedDuringBoot(GDBError):
    """Dolphin process exited before the GDB stub became available."""


@dataclass
class GDBClient:
    """Minimal GDB RSP client for Dolphin's built-in debugger stub."""

    host: str = "127.0.0.1"
    port: int = 2345
    _sock: socket.socket | None = field(default=None, repr=False, init=False)
    _recv_buffer: bytes = field(default=b"", repr=False, init=False)

    def connect(
        self,
        timeout: float = 10.0,
        proc: subprocess.Popen[bytes] | None = None,
        log_path: Path | None = None,
    ) -> None:
        """Connect to Dolphin's GDB stub. Retries until timeout.

        Args:
            timeout: Maximum seconds to wait for the GDB stub to accept a
                connection.
            proc: Optional Dolphin subprocess handle.  When provided the
                connect loop polls ``proc.poll()`` and raises
                :class:`DolphinDiedDuringBoot` immediately if the process
                exits, instead of waiting for the full timeout.
            log_path: Path to ``dolphin.log``.  When a connection failure
                occurs the last 50 lines are included in the exception
                message so the agent (or human) can diagnose the crash.
        """
        deadline = time.monotonic() + timeout
        last_err: OSError | None = None

        while time.monotonic() < deadline:
            # Fast-fail if Dolphin already exited.
            if proc is not None and proc.poll() is not None:
                tail = _read_log_tail(log_path)
                raise DolphinDiedDuringBoot(
                    f"Dolphin (pid {proc.pid}) exited with code "
                    f"{proc.returncode} before GDB stub bound port "
                    f"{self.port}\n--- last 50 lines of dolphin.log ---\n"
                    f"{tail}"
                )

            try:
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                sock.settimeout(5.0)
                sock.connect((self.host, self.port))
                self._sock = sock
                # Dolphin may send an initial stop reply — drain it
                self._recv_response(timeout=2.0)
                logger.info("gdb_connected", host=self.host, port=self.port)
                return
            except OSError as e:
                last_err = e
                sock.close()
                time.sleep(0.5)

        tail = _read_log_tail(log_path)
        raise GDBError(
            f"Could not connect to GDB stub at {self.host}:{self.port} "
            f"within {timeout}s: {last_err}\n"
            f"--- last 50 lines of dolphin.log ---\n{tail}"
        )

    def _drain_pending(self) -> None:
        """Read and discard any pending data in the socket buffer."""
        if self._sock is None:
            return
        self._recv_buffer = b""
        self._sock.setblocking(False)
        try:
            while True:
                data = self._sock.recv(4096)
                if not data:
                    break
        except BlockingIOError:
            pass
        finally:
            self._sock.setblocking(True)
            self._sock.settimeout(5.0)

    def close(self) -> None:
        """Disconnect from the GDB stub."""
        if self._sock is not None:
            try:
                # Detach cleanly so Dolphin continues running
                self._send("D")
            except OSError:
                pass
            try:
                self._sock.close()
            except OSError:
                pass
            self._sock = None
            self._recv_buffer = b""

    def continue_execution(self) -> None:
        """Send ``c`` (continue). Does not wait for a stop reply."""
        self._send_raw(_make_packet("c"))

    def step_instruction(self, timeout: float = 5.0) -> str:
        """Execute one PowerPC instruction while stopped and return its stop reply."""
        self._send("s")
        return self.wait_for_stop(timeout)

    def interrupt(self) -> str:
        """Send Ctrl-C (\\x03) to pause the CPU.

        GDB RSP commands can only be sent when the CPU is stopped. If
        the CPU is running (after a ``c`` command), send this to pause it.
        Returns the stop reply.
        """
        self._send_raw(b"\x03")
        # Dolphin may send an empty reply alongside a stop notification.
        deadline = time.monotonic() + 5.0
        while time.monotonic() < deadline:
            resp = self._recv_response(timeout=deadline - time.monotonic())
            if resp is None:
                break
            if resp.startswith("T"):
                logger.debug("gdb_interrupt_reply", reply=resp)
                return resp
        raise GDBError("No stop reply after interrupt")

    def read_pc(self) -> int:
        """Read the program counter (NIP) from Dolphin's PPC CPU.

        Must be called when the CPU is stopped (e.g. after a watchpoint hit).
        Uses ``p`` command to read a specific register.

        Dolphin's PPC GDB stub maps register 64 (0x40) as the PC (NIP).
        """
        resp = self._command("p40")
        if resp.startswith("E"):
            raise GDBError(f"Failed to read PC: {resp}")
        # Response is hex-encoded 32-bit value (big-endian for PPC)
        try:
            pc = int(resp, 16)
            return pc
        except ValueError:
            raise GDBError(f"Invalid PC response: {resp!r}")

    def read_register(self, number: int) -> int:
        """Read one PPC register by its GDB register number while stopped."""
        if not 0 <= number <= 0xFF:
            raise ValueError("register number must fit in one byte")
        resp = self._command(f"p{number:x}")
        if resp.startswith("E"):
            raise GDBError(f"Failed to read register {number}: {resp}")
        try:
            return int(resp, 16)
        except ValueError as exc:
            raise GDBError(f"Invalid register response: {resp!r}") from exc

    def set_execution_breakpoint(self, address: int) -> None:
        """Arm a four-byte PowerPC execution breakpoint while stopped."""
        resp = self._command(f"Z0,{address:x},4")
        if resp != "OK":
            raise GDBError(f"Failed to set breakpoint at {hex(address)}: {resp}")

    def remove_execution_breakpoint(self, address: int) -> None:
        """Remove a PowerPC execution breakpoint while stopped."""
        resp = self._command(f"z0,{address:x},4")
        if resp != "OK":
            raise GDBError(f"Failed to remove breakpoint at {hex(address)}: {resp}")

    def set_write_watchpoint(self, address: int, size: int = 4) -> str:
        """Set a hardware write watchpoint at *address* for *size* bytes.

        Returns the response (should be "OK").
        """
        resp = self._command(f"Z2,{address:x},{size:x}")
        if resp != "OK":
            raise GDBError(f"Failed to set watchpoint at {hex(address)}: {resp}")
        logger.debug("gdb_watchpoint_set", address=hex(address), size=size)
        return resp

    def remove_write_watchpoint(self, address: int, size: int = 4) -> str:
        """Remove a write watchpoint."""
        resp = self._command(f"z2,{address:x},{size:x}")
        return resp

    def wait_for_stop(self, timeout: float = 30.0) -> str:
        """Wait for Dolphin to send a stop reply (e.g. watchpoint hit).

        Returns the stop reason string (e.g. "T05watch:80123456;").
        """
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            resp = self._recv_response(timeout=deadline - time.monotonic())
            if resp is None:
                break
            if resp.startswith(("T", "W", "X")):
                return resp
        raise GDBError(f"Timeout waiting for stop reply ({timeout}s)")

    # --- Internal --------------------------------------------------------- #

    def _command(self, data: str) -> str:
        """Send a command and wait for the response."""
        self._send(data)
        deadline = time.monotonic() + 10.0
        while time.monotonic() < deadline:
            resp = self._recv_response(timeout=deadline - time.monotonic())
            if resp is None:
                break
            # A stop may be reported twice around an interrupt. It is not
            # the reply to a register or breakpoint command. Dolphin may
            # also emit an empty packet in the same burst.
            if not resp or resp.startswith("T"):
                continue
            logger.debug("gdb_command", cmd=data[:20], resp=resp[:60])
            return resp
        raise GDBError(f"No response to command: {data}")

    def _send(self, data: str) -> None:
        """Send a GDB RSP packet."""
        self._send_raw(_make_packet(data))

    def _send_raw(self, raw: bytes) -> None:
        """Send raw bytes to the socket."""
        if self._sock is None:
            raise GDBError("Not connected")
        self._sock.sendall(raw)

    def _recv_response(self, timeout: float = 10.0) -> str | None:
        """Receive a GDB RSP response packet.

        Strips ``+``/``-`` ACK bytes and ``$..#xx`` framing.
        Returns the payload string, or None on timeout.
        """
        if self._sock is None:
            raise GDBError("Not connected")

        deadline = time.monotonic() + timeout

        try:
            while True:
                # Skip ACK bytes
                while self._recv_buffer[:1] in (b"+", b"-"):
                    self._recv_buffer = self._recv_buffer[1:]

                # Look for complete packet: $<data>#<xx>
                if b"$" in self._recv_buffer and b"#" in self._recv_buffer:
                    start = self._recv_buffer.index(b"$")
                    # Find the checksum (2 chars after #)
                    hash_pos = self._recv_buffer.index(b"#", start)
                    if hash_pos + 2 < len(self._recv_buffer):
                        payload = self._recv_buffer[start + 1 : hash_pos].decode(
                            "ascii", errors="replace"
                        )
                        self._recv_buffer = self._recv_buffer[hash_pos + 3 :]
                        # Send ACK
                        try:
                            self._sock.sendall(b"+")
                        except OSError:
                            pass
                        return payload
                # If we've accumulated a lot without a packet, bail
                if len(self._recv_buffer) > 65536:
                    self._recv_buffer = b""
                    return None

                remaining = deadline - time.monotonic()
                if remaining <= 0:
                    return None
                self._sock.settimeout(remaining)
                chunk = self._sock.recv(4096)
                if not chunk:
                    return None
                self._recv_buffer += chunk

        except socket.timeout:
            return None
        except OSError:
            return None


# --------------------------------------------------------------------------- #
# High-level: find all writers to an address                                  #
# --------------------------------------------------------------------------- #


@dataclass
class WriteHit:
    """A single write watchpoint hit."""

    pc: int  # program counter of the writing instruction
    count: int = 1  # how many times this PC wrote to the address


def _raw_send_recv(
    sock: socket.socket, data: str, timeout: float = 5.0
) -> str:
    """Send a GDB RSP command and return the response payload.

    Simple implementation that handles Dolphin's response format
    (ACK prefix, packet framing) without complex buffering.
    """
    pkt = _make_packet(data)
    sock.sendall(pkt)
    sock.settimeout(timeout)

    buf = b""
    while True:
        try:
            chunk = sock.recv(4096)
        except socket.timeout:
            return ""
        if not chunk:
            return ""
        buf += chunk

        # Strip leading ACK bytes
        while buf and buf[0:1] in (b"+", b"-"):
            buf = buf[1:]

        # Find complete packet
        if b"$" in buf and b"#" in buf:
            dollar = buf.index(b"$")
            hash_pos = buf.index(b"#", dollar + 1)
            if hash_pos + 2 < len(buf):
                payload = buf[dollar + 1 : hash_pos].decode("ascii", errors="replace")
                # Send ACK
                try:
                    sock.sendall(b"+")
                except OSError:
                    pass
                return payload


def _parse_pc_from_stop_reply(reply: str) -> int | None:
    """Extract the PC from a GDB stop reply like ``T0540:80159d64;...``

    Register 0x40 is the PPC NIP (next instruction pointer / PC).
    """
    # Format: T<signal><reg>:<hex>;...
    # Look for "40:" prefix (register 64 = NIP)
    m = re.search(r"40:([0-9a-fA-F]+)", reply)
    if m:
        return int(m.group(1), 16)
    return None


def find_writers(
    gdb: GDBClient,
    address: int,
    *,
    size: int = 4,
    duration: float = 3.0,
    max_hits: int = 200,
) -> list[WriteHit]:
    """Discover all instructions that write to a GameCube memory address.

    Sets a write watchpoint, lets the game run, and collects the PCs of
    all instructions that trigger the watchpoint. Returns unique PCs sorted
    by hit count (most frequent first).

    Args:
        gdb: Connected GDBClient.
        address: GameCube address to watch (0x80XXXXXX).
        size: Number of bytes to watch (default 4 for a float).
        duration: How long to collect hits (seconds).
        max_hits: Stop early after this many total hits.

    Returns:
        List of WriteHit with unique PCs and their hit counts.
    """
    # Use raw socket for reliability — the GDBClient's _recv_response
    # can struggle with Dolphin's multi-packet responses.
    sock = gdb._sock
    if sock is None:
        raise GDBError("GDB not connected")

    # Step 1: Pause the CPU (send Ctrl-C, read stop reply)
    sock.setblocking(False)
    try:
        while sock.recv(4096):
            pass  # drain any pending data
    except BlockingIOError:
        pass
    sock.setblocking(True)
    sock.settimeout(5.0)

    sock.sendall(b"\x03")
    time.sleep(0.3)
    try:
        resp = sock.recv(4096)
        logger.debug("gdb_interrupt_raw", resp=resp[:80] if resp else b"")
    except socket.timeout:
        raise GDBError("No stop reply after interrupt")

    # Step 2: Set write watchpoint
    resp_str = _raw_send_recv(sock, f"Z2,{address:x},{size:x}")
    if resp_str != "OK":
        raise GDBError(f"Failed to set watchpoint at {hex(address)}: {resp_str!r}")
    logger.debug("gdb_watchpoint_set", address=hex(address))

    pc_counts: dict[int, int] = {}
    total_hits = 0
    deadline = time.monotonic() + duration

    # Step 3: Continue and collect hits.
    # After sending `c`, Dolphin first ACKs with `+`, then pauses on
    # a watchpoint hit and sends `$T05..#xx`. We accumulate received
    # data until we find a complete stop-reply packet.
    sock.sendall(_make_packet("c"))

    recv_buf = b""
    while time.monotonic() < deadline and total_hits < max_hits:
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            break

        sock.settimeout(min(2.0, remaining))
        try:
            chunk = sock.recv(4096)
        except socket.timeout:
            continue  # timeout is normal — game is running, no hit yet
        except OSError:
            break

        if not chunk:
            break

        recv_buf += chunk

        # Strip ACK bytes from front
        while recv_buf and recv_buf[0:1] in (b"+", b"-"):
            recv_buf = recv_buf[1:]

        # Look for complete packet(s) in the buffer
        while b"$" in recv_buf and b"#" in recv_buf:
            dollar = recv_buf.index(b"$")
            try:
                hash_pos = recv_buf.index(b"#", dollar + 1)
            except ValueError:
                break
            if hash_pos + 2 >= len(recv_buf):
                break  # need more data for checksum

            payload = recv_buf[dollar + 1 : hash_pos].decode(
                "ascii", errors="replace"
            )
            recv_buf = recv_buf[hash_pos + 3 :]  # consume packet + checksum

            # Send ACK
            try:
                sock.sendall(b"+")
            except OSError:
                pass

            # Skip empty packets
            if not payload:
                continue

            # Parse stop reply
            if payload.startswith("T"):
                pc = _parse_pc_from_stop_reply(payload)
                if pc is not None:
                    pc_counts[pc] = pc_counts.get(pc, 0) + 1
                    total_hits += 1

                    if total_hits % 20 == 0:
                        logger.debug(
                            "find_writers_progress",
                            hits=total_hits,
                            unique=len(pc_counts),
                        )

                # Continue to next hit
                if time.monotonic() < deadline and total_hits < max_hits:
                    sock.sendall(_make_packet("c"))
                    recv_buf = b""  # clear for next round
            elif payload.startswith("W") or payload.startswith("X"):
                # Process exited or terminated
                break

    # Step 4: Cleanup — pause, remove watchpoint, resume
    try:
        sock.sendall(b"\x03")
        time.sleep(0.3)
        sock.setblocking(False)
        try:
            sock.recv(4096)
        except BlockingIOError:
            pass
        sock.setblocking(True)
        sock.settimeout(5.0)
        _raw_send_recv(sock, f"z2,{address:x},{size:x}")
        sock.sendall(_make_packet("c"))
    except OSError:
        pass

    results = [
        WriteHit(pc=pc, count=count) for pc, count in pc_counts.items()
    ]
    results.sort(key=lambda h: -h.count)

    logger.info(
        "find_writers_done",
        address=hex(address),
        total_hits=total_hits,
        unique_pcs=len(results),
    )

    return results
