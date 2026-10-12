"""Offline packet-boundary checks for Dolphin's GDB client."""

import socket

from src.core.dolphin.debugger import GDBClient, _make_packet


def test_recv_response_preserves_coalesced_packets():
    client_sock, stub_sock = socket.socketpair()
    try:
        client = GDBClient()
        client._sock = client_sock
        stub_sock.sendall(b"+" + _make_packet("OK") + _make_packet("T0540:80104294;"))

        assert client._recv_response(timeout=0.1) == "OK"
        assert client._recv_response(timeout=0.1) == "T0540:80104294;"
        assert stub_sock.recv(2) == b"++"
    finally:
        client_sock.close()
        stub_sock.close()


def test_recv_response_preserves_partial_packet_after_timeout():
    client_sock, stub_sock = socket.socketpair()
    try:
        client = GDBClient()
        client._sock = client_sock
        packet = _make_packet("12345678")
        stub_sock.sendall(packet[:6])

        assert client._recv_response(timeout=0.01) is None
        stub_sock.sendall(packet[6:])
        assert client._recv_response(timeout=0.1) == "12345678"
    finally:
        client_sock.close()
        stub_sock.close()


def test_read_pc_skips_delayed_stop_and_empty_reply():
    client_sock, stub_sock = socket.socketpair()
    try:
        client = GDBClient()
        client._sock = client_sock
        stub_sock.sendall(
            _make_packet("T0540:80204070;01:803b57a0;")
            + _make_packet("")
            + _make_packet("8015F764")
        )

        assert client.read_pc() == 0x8015F764
        assert stub_sock.recv(len(_make_packet("p40")) + 3) == _make_packet("p40") + b"+++"
    finally:
        client_sock.close()
        stub_sock.close()


def test_interrupt_skips_empty_reply():
    client_sock, stub_sock = socket.socketpair()
    try:
        client = GDBClient()
        client._sock = client_sock
        stub_sock.sendall(_make_packet("") + _make_packet("T0540:80204070;"))

        assert client.interrupt() == "T0540:80204070;"
        assert stub_sock.recv(3) == b"\x03++"
    finally:
        client_sock.close()
        stub_sock.close()


def test_step_instruction_waits_for_stop_reply():
    client_sock, stub_sock = socket.socketpair()
    try:
        client = GDBClient()
        client._sock = client_sock
        stub_sock.sendall(_make_packet("T0540:80068734;"))

        assert client.step_instruction(timeout=0.1) == "T0540:80068734;"
        assert stub_sock.recv(len(_make_packet("s")) + 1) == _make_packet("s") + b"+"
    finally:
        client_sock.close()
        stub_sock.close()
