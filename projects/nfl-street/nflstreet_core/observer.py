"""Decode verified NFL Street fields from a read-only GameCube memory source.

Addresses here apply to GNNE69 revision 0, DOL SHA-1
46e3de854998a0f66c9be8c72a1164d22cd29eb4. See the research logs in
``research/nfl_street`` for the experiments behind each pointer and offset.
"""

from __future__ import annotations

import math
import struct
from dataclasses import asdict, dataclass
from typing import Protocol

MEM1_START = 0x80000000
MEM1_END = 0x81800000
GAME_ID = b"GNNE69"

SCORE_ROOT = 0x803A29C8
SCORE_RIGHT_OFFSET = 0x3C
SCORE_LEFT_OFFSET = 0x3E
FNIG_ROOT = 0x8039E4E8
FNIG_NODE_DESCRIPTOR_OFFSET = 0x2C
FNIG_RECORD_OFFSET = 0x14
FNIG_TAG_OFFSET = 0x1C
FNIG_SCORE_OFFSET = 0x10
FNIG_SCHEMA_OFFSET = 0x8C
FNIG_SCORE_SCHEMA = bytes.fromhex(
    "00 00 00 80 43 53 41 47 00 00 00 08 00 00 00 03 "
    "00 00 00 88 43 53 48 47 00 00 00 08 00 00 00 03"
)

MAPPING_ROOT = 0x803A1E30
ROSTER_ROOT = 0x803A1E10
ACTOR_STRIDE = 0x14D4
ACTOR_NAME_OFFSET = 0xB80
ACTOR_POSITION_OFFSET = 0x1A8


class MemoryReader(Protocol):
    def read_bytes(self, address: int, size: int) -> bytes: ...


class UnavailableMemoryError(ValueError):
    """A pointer or read is unavailable in this observation."""


@dataclass(frozen=True)
class Score:
    left: int
    right: int


@dataclass(frozen=True)
class Position:
    x: float
    y: float
    z: float


@dataclass(frozen=True)
class SelectedPlayer:
    team_index: int
    roster_index: int
    name: str
    position: Position


@dataclass(frozen=True)
class Observation:
    score: Score | None
    score_status: str
    selected_player: SelectedPlayer | None
    selection_status: str
    named_score: Score | None = None
    named_score_status: str = "unavailable"

    def to_dict(self) -> dict[str, object]:
        return asdict(self)


class NFLStreetObserver:
    """Read both score representations and controller-0 selection without writes."""

    def __init__(self, memory: MemoryReader) -> None:
        self.memory = memory

    def _read(self, address: int, size: int) -> bytes:
        if address < MEM1_START or size < 0 or address + size > MEM1_END:
            raise UnavailableMemoryError("pointer_outside_mem1")
        try:
            data = self.memory.read_bytes(address, size)
        except Exception as exc:
            raise UnavailableMemoryError("read_failed") from exc
        if len(data) != size:
            raise UnavailableMemoryError("short_read")
        return data

    def _u32(self, address: int) -> int:
        return int.from_bytes(self._read(address, 4), "big")

    def _score(self) -> tuple[Score | None, str]:
        try:
            base = self._u32(SCORE_ROOT)
            if base == 0:
                return None, "pointer_unavailable"
            raw = self._read(base + SCORE_RIGHT_OFFSET, 4)
            if self._u32(SCORE_ROOT) != base:
                return None, "pointer_changed"
            right, left = struct.unpack(">HH", raw)
            return Score(left=left, right=right), "available"
        except UnavailableMemoryError as exc:
            return None, str(exc)

    def _named_score(self) -> tuple[Score | None, str]:
        try:
            node = self._u32(FNIG_ROOT)
            if node == 0:
                return None, "pointer_unavailable"
            descriptor = self._u32(node + FNIG_NODE_DESCRIPTOR_OFFSET)
            if descriptor == 0:
                return None, "descriptor_unavailable"
            if self._read(descriptor + FNIG_TAG_OFFSET, 4) != b"FNIG":
                return None, "tag_mismatch"
            if self._read(descriptor + FNIG_SCHEMA_OFFSET, len(FNIG_SCORE_SCHEMA)) != FNIG_SCORE_SCHEMA:
                return None, "schema_mismatch"
            record = self._u32(descriptor + FNIG_RECORD_OFFSET)
            if record == 0:
                return None, "record_unavailable"
            left, right = self._read(record + FNIG_SCORE_OFFSET, 2)
            if (self._u32(FNIG_ROOT) != node
                    or self._u32(node + FNIG_NODE_DESCRIPTOR_OFFSET) != descriptor
                    or self._u32(descriptor + FNIG_RECORD_OFFSET) != record):
                return None, "pointer_changed"
            return Score(left=left, right=right), "available"
        except UnavailableMemoryError as exc:
            return None, str(exc)

    def _selected_player(self) -> tuple[SelectedPlayer | None, str]:
        try:
            mapping = self._u32(MAPPING_ROOT)
            if mapping == 0:
                return None, "mapping_unavailable"
            entries = self._u32(mapping)
            if entries == 0:
                return None, "entries_unavailable"
            team, index = self._read(entries, 2)
            if team == 0xFF or index == 0xFF:
                return None, "no_selection"
            manager = self._u32(ROSTER_ROOT)
            if manager == 0:
                return None, "roster_unavailable"
            actors = self._u32(manager)
            count = int.from_bytes(self._read(manager + 8, 2), "big")
            if actors == 0 or count == 0 or team > 1 or index >= count:
                return None, "roster_invalid"
            actor = actors + (team * count + index) * ACTOR_STRIDE
            name_bytes = self._read(actor + ACTOR_NAME_OFFSET, 16).split(b"\0", 1)[0]
            if not name_bytes or any(value < 0x20 or value > 0x7E for value in name_bytes):
                return None, "actor_name_invalid"
            name = name_bytes.decode("ascii")
            x, y, z = struct.unpack(">fff", self._read(actor + ACTOR_POSITION_OFFSET, 12))
            if not all(math.isfinite(value) for value in (x, y, z)):
                return None, "position_invalid"
            if (self._u32(MAPPING_ROOT) != mapping or self._u32(mapping) != entries
                    or self._read(entries, 2) != bytes((team, index))
                    or self._u32(ROSTER_ROOT) != manager or self._u32(manager) != actors):
                return None, "pointer_changed"
            return SelectedPlayer(team, index, name, Position(x, y, z)), "available"
        except UnavailableMemoryError as exc:
            return None, str(exc)

    def observe(self) -> Observation:
        if self._read(MEM1_START, len(GAME_ID)) != GAME_ID:
            raise ValueError("MEM1 does not contain the tested GNNE69 game ID")
        score, score_status = self._score()
        named_score, named_score_status = self._named_score()
        selected_player, selection_status = self._selected_player()
        return Observation(score, score_status, selected_player, selection_status,
                           named_score, named_score_status)


class Mem1Image:
    """Adapter for an existing 24 MiB MEM1 capture."""

    def __init__(self, data: bytes) -> None:
        if len(data) != MEM1_END - MEM1_START:
            raise ValueError("expected a complete 24 MiB MEM1 image")
        self.data = data

    def read_bytes(self, address: int, size: int) -> bytes:
        start = address - MEM1_START
        if start < 0 or size < 0 or start + size > len(self.data):
            raise UnavailableMemoryError("pointer_outside_mem1")
        return self.data[start : start + size]
