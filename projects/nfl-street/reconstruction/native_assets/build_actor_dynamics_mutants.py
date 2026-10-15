#!/usr/bin/env python3
"""Compile isolated actor-dynamics mutants; this script executes no Windows code."""

import argparse
import json
import subprocess
from pathlib import Path

HERE = Path(__file__).resolve().parent
OUTPUT = Path("/mnt/w/Daywater/research-temp/nflstreet-actor-dynamics-20261014")


def definitions() -> list[dict[str, str]]:
    mutants = json.loads((HERE / "actor_dynamics_mutants.json").read_text())
    names: set[str] = set()
    for mutant in mutants:
        if set(mutant) != {"name", "file", "old", "new"} or not all(isinstance(v, str) for v in mutant.values()):
            raise ValueError("Each mutant must contain four strings: name, file, old, new")
        name = mutant["name"]
        if not name or any(c not in "abcdefghijklmnopqrstuvwxyz0123456789-" for c in name) or name in names:
            raise ValueError(f"Invalid or repeated mutant name: {name}")
        names.add(name)
        if mutant["file"] != "actor_dynamics.c":
            raise ValueError("Only the dynamics implementation may be mutated")
        source = (HERE / mutant["file"]).read_text()
        if source.count(mutant["old"]) != 1 or mutant["old"] == mutant["new"]:
            raise ValueError(f"Mutant {name} does not select exactly one source fragment")
    return mutants


def compile_mutant(mutant: dict[str, str]) -> Path:
    out = OUTPUT / "mutants" / mutant["name"]
    out.mkdir(parents=True, exist_ok=True)
    source = (HERE / mutant["file"]).read_text()
    if source.count(mutant["old"]) != 1:
        raise ValueError(f"Source no longer matches mutant {mutant['name']}")
    changed = out / mutant["file"]
    changed.write_text(source.replace(mutant["old"], mutant["new"], 1))
    with (out / "build.log").open("w") as log:
        subprocess.run(
            ["bash", str(HERE / "build_windows_actor_dynamics_original_check.sh"), str(out), str(changed), "all"],
            stdout=log,
            stderr=subprocess.STDOUT,
            check=True,
        )
    return out / "actor-dynamics-original-check.exe"


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--validate-only", action="store_true", help="Validate replacements without compiling")
    args = parser.parse_args()
    mutants = definitions()
    if args.validate_only:
        print(f"{len(mutants)} unique mutation definitions validated; none executed.")
        return
    for mutant in mutants:
        print(f"Compiled {mutant['name']}: {compile_mutant(mutant)}", flush=True)
    print(f"Compiled {len(mutants)} isolated mutants; none executed.")


if __name__ == "__main__":
    main()
