#!/usr/bin/env python3
"""Repeat owned appearance initialization against the original Windows module."""
import argparse
import json
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / 'menus'))
from verify_controller_checks import PRIVATE, BASELINES, baseline_hashes, digest, private, run, windows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--no-build', action='store_true')
    args = parser.parse_args()
    root = Path(__file__).resolve().parent
    out = private(PRIVATE / 'research-temp/nflstreet-player-appearance-20261012')
    out.mkdir(parents=True, exist_ok=True)
    report = out / 'verification.json'
    record = {'status': 'running', 'boundary':
              'Original801639C8 and owned node/vector/table/registry helpers. '
              'Descriptor inverses and catalogs from reference state; resource '
              'lookup and complex binding owners supplied. No fresh asset loading or rendering claim.'}
    report.write_text(json.dumps(record, indent=2) + '\n')
    try:
        before = baseline_hashes()
        inputs = [private(PRIVATE / f'research-temp/position-neutral-{c}.mem1') for c in (14, 15, 16)]
        hashes = {str(p): digest(p) for p in inputs}
        if not args.no_build:
            run(['bash', str(root / 'build_windows_player_appearance_original_check.sh'), str(out)], out / 'build.log')
        exe = out / 'player-appearance-original-check.exe'
        expected = [(str(p), str(m), str(v), '89') for p in range(2) for m in range(2) for v in range(2)]
        contexts = {}
        for context in (14, 15, 16):
            traces = []
            for repeat in range(2):
                log = out / f'root-context{context}-{repeat}.log'
                text = run([str(exe), windows(BASELINES['original_dll'][0]),
                            windows(PRIVATE / f'research-temp/position-neutral-{context}.mem1')], log)
                rows = re.findall(r'^appearance prefilled=(\d+) mutation=(\d+) variant=(\d+): actor1A90 \+two allocations \+two registries \+(\d+) requests exact', text, re.M)
                if rows != expected:
                    raise RuntimeError(f'Incomplete appearance comparison: context{context}')
                traces.append(digest(log))
            if traces[0] != traces[1]:
                raise RuntimeError('Fresh appearance traces differ')
            contexts[str(context)] = {'cases': 8, 'fresh_processes': 2, 'log_sha256': traces}
        if before != baseline_hashes() or any(digest(Path(p)) != h for p, h in hashes.items()):
            raise RuntimeError('Protected originals or resource contexts changed')
        record.update(status='passed', baseline=before, input_sha256=hashes,
                      contexts=contexts, executable_sha256=digest(exe),
                      source_sha256={str(p.relative_to(root)): digest(p) for p in root.glob('*.c')
                                     if p.name in ('player_appearance_init.c', 'player_appearance_original_check.c',
                                                   'actor_model_init_original_check.c')})
        record['source_sha256'].update({name: digest(root / name) for name in
                                      ('player_appearance_init.h', 'player_appearance_check_fixture.h',
                                       'build_windows_player_appearance_original_check.sh')})
    except Exception as exc:
        record.update(status='failed', error=str(exc))
        raise
    finally:
        report.write_text(json.dumps(record, indent=2) + '\n')
    print('Owned appearance comparisons pass; private report ' + str(report))


if __name__ == '__main__':
    main()
