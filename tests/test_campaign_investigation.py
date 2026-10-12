"""Offline campaign and bridge checks; no Dolphin or Ghidra process needed."""

import json
from pathlib import Path
from types import SimpleNamespace

from src.core.campaign import Campaign
from src.core.experiments import ExperimentSpec, run_experiment
from src.core.ghidra.cache import function_containing
from src.core.investigation import investigate_address

SHA = "a" * 40


def _cache(root: Path) -> Path:
    cache = root / SHA
    (cache / "decompiled").mkdir(parents=True)
    (cache / "disassembly").mkdir()
    (cache / "functions.json").write_text(json.dumps({"functions": [
        {"addr": "80100000", "name": "FUN_80100000", "size": 8,
         "ranges": [["80100000", "80100003"], ["80100010", "80100013"]]}]}))
    (cache / "callgraph.json").write_text(json.dumps({"80100000": {"callers": [], "callees": []}}))
    (cache / "strings.json").write_text('{"strings": []}')
    (cache / "decompiled" / "80100000.txt").write_text("void FUN_80100000() {}")
    (cache / "disassembly" / "80100000.txt").write_text("80100010: stw r3,0(r4)\n")
    return cache


class FakeSession:
    def read_bytes(self, address: int, size: int) -> bytes:
        return bytes.fromhex("00000007")[:size]

    def find_writers(self, address: int, *, size: int, duration: float, max_hits: int):
        return [SimpleNamespace(pc=0x80100010, count=2)]


def test_runtime_to_static_evidence_persists(tmp_path: Path) -> None:
    cache = _cache(tmp_path)
    campaign = Campaign(tmp_path / "project", SHA)
    result = investigate_address(campaign, cache, 0x80001000, session=FakeSession())
    assert result.observed_value_hex == "00000007"
    assert result.functions[0]["name"] == "FUN_80100000"
    assert "stw" in result.functions[0]["disassembly"]
    reloaded = Campaign(tmp_path / "project", SHA)
    assert len(reloaded.query(kind="evidence")) == 2
    assert len(reloaded.query(kind="function", status="triaged")) == 1
    assert any(r["predicate"] == "observed_writing" for r in reloaded.data["relations"])
    assert function_containing(cache, 0x80100008) is None  # gap between body ranges


def test_supplied_pc_is_not_labeled_a_watchpoint(tmp_path: Path) -> None:
    result = investigate_address(Campaign(tmp_path / "p", SHA), _cache(tmp_path),
                                 0x80001000, writer_pcs=[0x80100010])
    assert result.functions
    evidence = Campaign(tmp_path / "p", SHA).query(kind="evidence")
    assert evidence[0]["attributes"]["method"] == "supplied_pc"


def test_experiment_validation() -> None:
    spec = ExperimentSpec(name="score_change", savestate_id="before", duration_seconds=2,
                          actions=[{"at_seconds": 0.5, "command": "PRESS A"}])
    assert spec.input_sequence().commands[0].time_offset == 0.5


def test_experiment_links_watchpoint_evidence(tmp_path: Path) -> None:
    campaign = Campaign(tmp_path / "project", SHA)
    spec = ExperimentSpec(name="transition", savestate_id="before",
                          watch_address=0x80001000, duration_seconds=0.01)
    result = run_experiment(campaign, _cache(tmp_path), spec, FakeSession(),
                            active_savestate_id="before")
    assert Path(result["output_path"]).exists()
    assert result["writers"][0]["pc"] == "0x80100010"
    assert any(r["predicate"] == "produced" for r in campaign.data["relations"])


def test_observation_does_not_demote_curated_confidence(tmp_path: Path) -> None:
    cache = _cache(tmp_path)
    campaign = Campaign(tmp_path / "project", SHA)
    campaign.entity("function", "80100000", status="understood", confidence="probable")
    investigate_address(campaign, cache, 0x80001000, writer_pcs=[0x80100010])
    function = Campaign(tmp_path / "project", SHA).query(kind="function")[0]
    assert function["status"] == "understood"
    assert function["confidence"] == "probable"
