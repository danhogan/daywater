"""Durable, project-scoped reverse-engineering knowledge.

The existing Project owns the ISO and savestates; Campaign adds a small graph
without copying Ghidra's regenerable binary cache into curated knowledge.
Single writer, atomic JSON updates, and stable entity IDs keep the first
milestone easy to inspect and migrate.
"""

from __future__ import annotations

import json
import os
import tempfile
import time
import uuid
from pathlib import Path
from typing import Any

ENTITY_KINDS = {
    "function", "global", "runtime_address", "structure", "structure_field",
    "enum", "string", "savestate", "experiment", "hypothesis", "evidence", "finding",
}
STATUSES = {"unknown", "triaged", "named", "understood", "typed", "reconstructed"}
STATUS_ORDER = ("unknown", "triaged", "named", "understood", "typed", "reconstructed")
CONFIDENCES = {"candidate", "probable", "verified"}


def _write_json(path: Path, value: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile("w", dir=path.parent, delete=False, prefix=".campaign-") as f:
        json.dump(value, f, indent=2, sort_keys=True)
        f.write("\n")
        f.flush()
        os.fsync(f.fileno())
        temp = f.name
    # NamedTemporaryFile defaults to 0600. Campaign knowledge is project data
    # that also needs to be inspectable from the host when Docker's UID differs.
    os.chmod(temp, 0o644)
    os.replace(temp, path)


class Campaign:
    """One executable's knowledge under an existing Daywater project.

    ``binary_sha1`` is required so addresses from different executables do
    not silently merge. Runtime addresses and experiments remain scoped here.
    """

    def __init__(self, project_root: Path, binary_sha1: str) -> None:
        if len(binary_sha1) != 40 or any(c not in "0123456789abcdef" for c in binary_sha1.lower()):
            raise ValueError("binary_sha1 must be a 40-character SHA-1")
        self.root = project_root / "campaigns" / binary_sha1.lower()
        self.binary_sha1 = binary_sha1.lower()
        self.path = self.root / "knowledge" / "graph.json"
        if self.path.exists():
            self.data = json.loads(self.path.read_text())
        else:
            self.data = {"version": 1, "binary_sha1": self.binary_sha1, "entities": {}, "relations": []}

    def save(self) -> None:
        _write_json(self.path, self.data)

    def entity(
        self, kind: str, key: str, *, status: str = "unknown",
        confidence: str | None = None, notes: str = "", tags: list[str] | None = None,
        attributes: dict[str, Any] | None = None,
    ) -> dict[str, Any]:
        if (kind not in ENTITY_KINDS or status not in STATUSES
                or (confidence is not None and confidence not in CONFIDENCES)):
            raise ValueError("invalid entity kind, status, or confidence")
        eid = f"{kind}:{key}"
        now = time.time()
        entry = self.data["entities"].get(eid)
        if entry is None:
            entry = {"id": eid, "kind": kind, "key": key, "status": status,
                     "confidence": confidence or "candidate", "notes": notes, "tags": tags or [],
                     "attributes": attributes or {}, "evidence": [],
                     "created_at": now, "updated_at": now, "history": []}
            self.data["entities"][eid] = entry
        else:
            changes = {}
            for field, value in (("status", status), ("confidence", confidence), ("notes", notes)):
                if (value and value != entry[field]
                        and (field != "status" or STATUS_ORDER.index(status) > STATUS_ORDER.index(entry[field]))):
                    changes[field] = [entry[field], value]
                    entry[field] = value
            if tags:
                entry["tags"] = sorted(set(entry["tags"]) | set(tags))
            if attributes:
                entry["attributes"].update(attributes)
            if changes:
                entry["history"].append({"at": now, "changes": changes})
            entry["updated_at"] = now
        self.save()
        return entry

    def record(self, kind: str, attributes: dict[str, Any], *, key: str | None = None,
               confidence: str = "candidate") -> dict[str, Any]:
        if kind not in {"experiment", "hypothesis", "evidence", "finding"}:
            raise ValueError("record kind must be experiment, hypothesis, evidence, or finding")
        return self.entity(kind, key or uuid.uuid4().hex, status="triaged",
                           confidence=confidence, attributes=attributes)

    def relate(self, source: str, predicate: str, target: str, *, evidence: str | None = None) -> None:
        entities = self.data["entities"]
        if source not in entities or target not in entities:
            raise KeyError("both relation endpoints must exist")
        relation = {"source": source, "predicate": predicate, "target": target,
                    "evidence": evidence}
        if relation not in self.data["relations"]:
            self.data["relations"].append(relation)
            self.save()

    def query(self, *, kind: str | None = None, status: str | None = None,
              tag: str | None = None) -> list[dict[str, Any]]:
        return [e for e in self.data["entities"].values()
                if (kind is None or e["kind"] == kind)
                and (status is None or e["status"] == status)
                and (tag is None or tag in e["tags"])]

    def write_experiment_output(self, experiment_key: str, output: dict[str, Any]) -> Path:
        if f"experiment:{experiment_key}" not in self.data["entities"]:
            raise KeyError(experiment_key)
        path = self.root / "experiments" / experiment_key / f"{uuid.uuid4().hex}.json"
        _write_json(path, output)
        return path
