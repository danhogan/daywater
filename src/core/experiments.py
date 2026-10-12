"""Small, reproducible runtime experiment plans.

Timing is in seconds because Dolphin's current input pipe has no exact frame
step primitive. A caller must start Dolphin from ``savestate_id`` before run.
"""

from __future__ import annotations

from dataclasses import asdict, dataclass, field
from typing import Any

from src.core.campaign import Campaign
from src.core.dolphin.input import InputCommand, InputSequence
from src.core.investigation import investigate_address


@dataclass
class ExperimentSpec:
    name: str
    savestate_id: str
    description: str = ""
    actions: list[dict[str, Any]] = field(default_factory=list)
    observe_addresses: list[int] = field(default_factory=list)
    watch_address: int | None = None
    duration_seconds: float = 3.0

    def validate(self) -> None:
        if not self.name or not self.savestate_id or self.duration_seconds <= 0:
            raise ValueError("name, savestate_id, and positive duration_seconds required")
        for action in self.actions:
            if set(action) != {"at_seconds", "command"}:
                raise ValueError("actions require at_seconds and command")
            if float(action["at_seconds"]) < 0 or float(action["at_seconds"]) > self.duration_seconds:
                raise ValueError("action time outside experiment")
            command = str(action["command"])
            if not command.startswith(("PRESS ", "RELEASE ", "SET ")) or "\n" in command:
                raise ValueError("unsupported Dolphin pipe command")

    def input_sequence(self) -> InputSequence:
        self.validate()
        return InputSequence([InputCommand(float(a["at_seconds"]), str(a["command"]))
                              for a in self.actions])


def run_experiment(campaign: Campaign, cache_dir: Any, spec: ExperimentSpec,
                   session: Any, *, active_savestate_id: str) -> dict[str, Any]:
    """Run on a session already booted from the declared savestate.

    Caller controls lifecycle. The run records raw observations, never names
    a memory field or promotes its confidence automatically.
    """
    spec.validate()
    if active_savestate_id != spec.savestate_id:
        raise ValueError("active session savestate differs from experiment start")
    experiment = campaign.record("experiment", asdict(spec))
    before = {f"0x{a:08x}": session.read_bytes(a, 4).hex() for a in spec.observe_addresses}
    if spec.actions:
        session.play_sequence_async(spec.input_sequence())
    if spec.watch_address is not None:
        result = investigate_address(campaign, cache_dir, spec.watch_address,
                                     session=session, experiment_key=experiment["key"],
                                     watch_seconds=spec.duration_seconds)
        writers = result.writers
    else:
        import time
        time.sleep(spec.duration_seconds)
        writers = []
    after = {f"0x{a:08x}": session.read_bytes(a, 4).hex() for a in spec.observe_addresses}
    output = {"spec": asdict(spec), "before": before, "after": after,
              "changed_addresses": [a for a in before if before[a] != after[a]],
              "writers": writers}
    for address in output["changed_addresses"]:
        runtime = campaign.entity("runtime_address", address.removeprefix("0x"), status="triaged")
        evidence = campaign.record("evidence", {"method": "before_after_memory", "address": address,
                                                "before_hex": before[address], "after_hex": after[address]})
        campaign.relate(experiment["id"], "produced", evidence["id"])
        campaign.relate(evidence["id"], "observes", runtime["id"])
    path = campaign.write_experiment_output(experiment["key"], output)
    return {"experiment_id": experiment["id"], "output_path": str(path), **output}
