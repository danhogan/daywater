"""Offline checks for the NFL Street read-only state decoder."""

from __future__ import annotations

import struct
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from nflstreet_core.observer import NFLStreetObserver, Score  # noqa: E402


class SparseMemory:
    def __init__(self) -> None:
        self.bytes: dict[int, int] = {}

    def put(self, address: int, data: bytes) -> None:
        self.bytes.update((address + offset, value) for offset, value in enumerate(data))

    def read_bytes(self, address: int, size: int) -> bytes:
        return bytes(self.bytes.get(address + offset, 0) for offset in range(size))


def populated_memory() -> SparseMemory:
    memory = SparseMemory()
    memory.put(0x80000000, b"GNNE69")
    memory.put(0x803A29C8, (0x803B6C20).to_bytes(4, "big"))
    memory.put(0x803B6C5C, struct.pack(">HH", 0, 7))
    memory.put(0x8039E4E8, (0x80421B98).to_bytes(4, "big"))
    memory.put(0x80421BC4, (0x804AC900).to_bytes(4, "big"))
    memory.put(0x804AC914, (0x804A21C0).to_bytes(4, "big"))
    memory.put(0x804AC91C, b"FNIG")
    memory.put(0x804AC98C, bytes.fromhex(
        "00 00 00 80 43 53 41 47 00 00 00 08 00 00 00 03 "
        "00 00 00 88 43 53 48 47 00 00 00 08 00 00 00 03"
    ))
    memory.put(0x804A21D0, bytes((7, 0)))
    memory.put(0x803A1E30, (0x803CD920).to_bytes(4, "big"))
    memory.put(0x803CD920, (0x803CD960).to_bytes(4, "big"))
    memory.put(0x803CD960, bytes((1, 1)))
    memory.put(0x803A1E10, (0x803B7300).to_bytes(4, "big"))
    memory.put(0x803B7300, (0x803B7320).to_bytes(4, "big"))
    memory.put(0x803B7308, (7).to_bytes(2, "big"))
    actor = 0x803B7320 + (1 * 7 + 1) * 0x14D4
    memory.put(actor + 0xB80, b"Thomas\0")
    memory.put(actor + 0x1A8, struct.pack(">fff", 2.5, -31.0, 0.0))
    return memory


def test_decodes_score_and_selected_player() -> None:
    observation = NFLStreetObserver(populated_memory()).observe()

    assert observation.score_status == "available"
    assert observation.score.left == 7
    assert observation.score.right == 0
    assert observation.named_score_status == "available"
    assert observation.named_score == observation.score
    assert observation.selection_status == "available"
    assert observation.selected_player.name == "Thomas"
    assert observation.selected_player.team_index == 1
    assert observation.selected_player.position.x == 2.5
    assert observation.to_dict()["selected_player"]["position"]["y"] == -31.0


def test_null_score_pointer_and_no_selection_are_explicit() -> None:
    memory = populated_memory()
    memory.put(0x803A29C8, b"\0\0\0\0")
    memory.put(0x803CD960, bytes((1, 0xFF)))

    observation = NFLStreetObserver(memory).observe()

    assert observation.score is None
    assert observation.score_status == "pointer_unavailable"
    assert observation.named_score == Score(left=7, right=0)
    assert observation.selected_player is None
    assert observation.selection_status == "no_selection"


def test_bad_roster_pointer_does_not_decode_an_actor() -> None:
    memory = populated_memory()
    memory.put(0x803B7300, (0x90000000).to_bytes(4, "big"))

    observation = NFLStreetObserver(memory).observe()

    assert observation.score_status == "available"
    assert observation.selected_player is None
    assert observation.selection_status == "pointer_outside_mem1"


def test_named_score_requires_fnig_schema() -> None:
    memory = populated_memory()
    memory.put(0x804AC994, (7).to_bytes(4, "big"))

    observation = NFLStreetObserver(memory).observe()

    assert observation.score == Score(left=7, right=0)
    assert observation.named_score is None
    assert observation.named_score_status == "schema_mismatch"


def test_score_representations_can_disagree_during_update() -> None:
    memory = populated_memory()
    memory.put(0x804A21D0, bytes((6, 0)))

    observation = NFLStreetObserver(memory).observe()

    assert observation.score == Score(left=7, right=0)
    assert observation.named_score == Score(left=6, right=0)
    assert observation.score_status == observation.named_score_status == "available"


def test_wrong_game_is_rejected() -> None:
    memory = populated_memory()
    memory.put(0x80000000, b"ABCDEF")

    with pytest.raises(ValueError, match="GNNE69"):
        NFLStreetObserver(memory).observe()
