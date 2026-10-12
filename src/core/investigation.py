"""Runtime observations linked to Ghidra's generated static index."""

from __future__ import annotations

import hashlib
from dataclasses import asdict, dataclass, field
from pathlib import Path
from typing import Any, Protocol

from src.core.campaign import Campaign
from src.core.ghidra.cache import (
    callees_of,
    callers_of,
    function_containing,
    read_decompiled,
    read_disassembly,
    strings_for_function,
)


class RuntimeSession(Protocol):
    def read_bytes(self, gc_address: int, size: int) -> bytes: ...
    def find_writers(self, address: int, *, size: int, duration: float, max_hits: int) -> list[Any]: ...


@dataclass
class InvestigationResult:
    address: str
    binary_sha1: str
    observed_value_hex: str | None = None
    writers: list[dict[str, Any]] = field(default_factory=list)
    functions: list[dict[str, Any]] = field(default_factory=list)
    evidence_ids: list[str] = field(default_factory=list)
    limitations: list[str] = field(default_factory=list)


def investigate_address(
    campaign: Campaign, cache_dir: Path, address: int, *,
    session: RuntimeSession | None = None, writer_pcs: list[int] | None = None,
    experiment_key: str | None = None, size: int = 4,
    watch_seconds: float = 3.0,
) -> InvestigationResult:
    """Persist observations and static context without inferring field meaning.

    A supplied PC can come from a prior watchpoint result. A live session
    reads the value and optionally collects write PCs. Static resolution is
    exact for a function entry or an indexed function-body range. Older
    indexes without ranges can only resolve entry PCs.
    """
    if not 0x80000000 <= address < 0x81800000 or size < 1 or size > 16:
        raise ValueError("address must be in MEM1 and size must be 1..16")
    if experiment_key and f"experiment:{experiment_key}" not in campaign.data["entities"]:
        raise KeyError(f"unknown experiment: {experiment_key}")
    result = InvestigationResult(address=f"0x{address:08x}", binary_sha1=campaign.binary_sha1)
    runtime = campaign.entity("runtime_address", f"{address:08x}", status="triaged",
                              attributes={"size": size})
    if session is not None:
        raw = session.read_bytes(address, size)
        result.observed_value_hex = raw.hex()
        value_ev = campaign.record("evidence", {"method": "runtime_read", "address": result.address,
                                                 "value_hex": raw.hex(), "size": size})
        result.evidence_ids.append(value_ev["id"])
        campaign.relate(value_ev["id"], "observes", runtime["id"])
        if experiment_key:
            campaign.relate(f"experiment:{experiment_key}", "produced", value_ev["id"])
    else:
        result.limitations.append("No live session: runtime value and new watchpoint hits unavailable")

    counts: dict[int, int] = {pc: 1 for pc in (writer_pcs or [])}
    observed_pcs: set[int] = set()
    if session is not None:
        try:
            hits = session.find_writers(address, size=size, duration=watch_seconds, max_hits=200)
        except RuntimeError as exc:
            hits = []
            result.limitations.append(f"Write watchpoint unavailable: {exc}")
        for hit in hits:
            counts[int(hit.pc)] = counts.get(int(hit.pc), 0) + int(hit.count)
            observed_pcs.add(int(hit.pc))
    if not counts:
        result.limitations.append("No writer PC observed or supplied")

    for pc, count in sorted(counts.items()):
        writer = {"pc": f"0x{pc:08x}", "count": count}
        result.writers.append(writer)
        ev = campaign.record("evidence", {"method": "write_watchpoint" if pc in observed_pcs else "supplied_pc",
                                          "runtime_address": result.address, **writer})
        result.evidence_ids.append(ev["id"])
        campaign.relate(ev["id"], "observes", runtime["id"])
        if experiment_key:
            campaign.relate(f"experiment:{experiment_key}", "produced", ev["id"])
        entry = function_containing(cache_dir, pc)
        if entry is None:
            writer["resolution"] = "unresolved in selected binary"
            continue
        _, code = read_decompiled(cache_dir, entry.addr)
        try:
            _, disassembly = read_disassembly(cache_dir, entry.addr)
        except FileNotFoundError:
            disassembly = ""
            result.limitations.append("Disassembly missing from this older index; regenerate analysis")
        context = {
            "address": f"0x{entry.addr}", "name": entry.name, "size": entry.size,
            "callers": [asdict(e) for e in callers_of(cache_dir, entry.addr)],
            "callees": [asdict(e) for e in callees_of(cache_dir, entry.addr)],
            "strings": [asdict(s) for s in strings_for_function(cache_dir, entry.addr)],
            "decompilation_sha256": hashlib.sha256(code.encode()).hexdigest(),
            "pseudocode": code[:16000], "pseudocode_truncated": len(code) > 16000,
            "disassembly": disassembly[:16000], "disassembly_truncated": len(disassembly) > 16000,
            "generated_paths": {
                "pseudocode": str(cache_dir / "decompiled" / f"{entry.addr}.txt"),
                "disassembly": str(cache_dir / "disassembly" / f"{entry.addr}.txt"),
            },
        }
        result.functions.append(context)
        func = campaign.entity("function", entry.addr, status="triaged",
                               attributes={"name_at_analysis": entry.name, "size": entry.size})
        campaign.relate(func["id"], "discovered_by_runtime_watchpoint", ev["id"], evidence=ev["id"])
        campaign.relate(func["id"], "observed_writing", runtime["id"], evidence=ev["id"])
        for callee in context["callees"]:
            callee_entity = campaign.entity("function", callee["addr"], attributes={"name_at_analysis": callee["name"]})
            campaign.relate(func["id"], "calls", callee_entity["id"])
    if experiment_key:
        campaign.write_experiment_output(experiment_key, asdict(result))
    return result
