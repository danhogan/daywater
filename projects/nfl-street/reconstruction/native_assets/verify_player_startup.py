#!/usr/bin/env python3
"""Verify connected native player construction against original constructor bodies."""
import argparse
import json
import os
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / 'menus'))
from verify_controller_checks import PRIVATE, BASELINES, baseline_hashes, digest, private, run, windows


def source_hashes(root):
    """Include local transitive headers and shared checker fixtures in provenance."""
    pending = [root / name for name in (
        'player_startup.c', 'player_startup_original_check.c', 'player_constructor.c',
        'actor_model_init.c', 'actor_tail_init.c', 'player_appearance_init.c',
        'build_windows_player_startup_original_check.sh')]
    headers = Path(os.environ.get('DAYWATER_MODERNGEKKO_HEADERS',
                   str(PRIVATE / 'tools/ModernGekko-Template/lib/ModernGekko/include'))).resolve()
    found = {}
    while pending:
        path = pending.pop().resolve()
        key = (str(path.relative_to(root)) if path.is_relative_to(root)
               else 'ModernGekko/' + str(path.relative_to(headers)))
        if key in found:
            continue
        found[key] = digest(path)
        for name in re.findall(r'^\s*#include\s+"([^"]+)"', path.read_text(), re.M):
            local = (path.parent / name).resolve()
            if not local.is_file():
                local = (headers / name).resolve()
            if not (local.is_relative_to(root) or local.is_relative_to(headers)):
                raise RuntimeError('Unexpected include outside declared source roots')
            pending.append(local)
    return found


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--no-build', action='store_true')
    args = parser.parse_args()
    root = Path(__file__).resolve().parent
    out = private(PRIVATE / 'research-temp/nflstreet-player-startup-20261012')
    out.mkdir(parents=True, exist_ok=True)
    report = out / 'verification.json'
    record = {'status': 'running', 'boundary':
              'Original800E1F7C through801639C8 and owned model/reset/node/table/registry helpers '
              'versus connected native player initialization. Reference resource views, inverse '
              'descriptor and original catalogs; allocation and complex resource/render services '
              'supplied. No fresh selected-player asset lookup or playable match claim.'}
    report.write_text(json.dumps(record, indent=2) + '\n')
    try:
        before = baseline_hashes()
        sources = source_hashes(root)
        inputs = [private(PRIVATE / f'research-temp/position-neutral-{c}.mem1') for c in (14, 15, 16)]
        hashes = {str(p): digest(p) for p in inputs}
        if not args.no_build:
            run(['bash', str(root / 'build_windows_player_startup_original_check.sh'), str(out)], out / 'build.log')
        exe = out / 'player-startup-original-check.exe'
        contexts = {}
        expected = [(str(mask), str(p), str(m), '89')
                    for mask in range(64) for p in range(2) for m in range(2)]
        for context in (14, 15, 16):
            traces = []
            for repeat in range(2):
                log = out / f'root-context{context}-{repeat}.log'
                text = run([str(exe), windows(BASELINES['original_dll'][0]),
                            windows(PRIVATE / f'research-temp/position-neutral-{context}.mem1')], log)
                rows = re.findall(r'^startup mask=(\d+) prefilled=(\d+) mutation=(\d+): actor1A90 \+\d+ model blocks \+two appearance allocations \+two registries \+(\d+) requests exact', text, re.M)
                if rows != expected:
                    raise RuntimeError(f'Incomplete joined startup comparison: context{context}')
                traces.append(digest(log))
            if traces[0] != traces[1]:
                raise RuntimeError('Fresh joined startup traces differ')
            contexts[str(context)] = {'cases': 256, 'fresh_processes': 2, 'log_sha256': traces}
        if before != baseline_hashes() or any(digest(Path(p)) != h for p, h in hashes.items()):
            raise RuntimeError('Protected originals or resource contexts changed')
        if sources != source_hashes(root):
            raise RuntimeError('Native or checker source changed during verification')
        record.update(status='passed', baseline=before, input_sha256=hashes,
                      contexts=contexts, executable_sha256=digest(exe), source_sha256=sources)
    except Exception as exc:
        record.update(status='failed', error=str(exc))
        raise
    finally:
        report.write_text(json.dumps(record, indent=2) + '\n')
    print('Connected player startup comparisons pass; private report ' + str(report))


if __name__ == '__main__':
    main()
