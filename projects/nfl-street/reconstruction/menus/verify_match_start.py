#!/usr/bin/env python3
"""Compare native startup orchestration with original code and supplied services."""
import argparse
import json
import re
from pathlib import Path
from verify_controller_checks import PRIVATE, BASELINES, baseline_hashes, digest, private, run, windows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--no-build', action='store_true')
    args = parser.parse_args()
    root = Path(__file__).resolve().parent
    out = private(PRIVATE / 'research-temp/nflstreet-match-start-20261012')
    out.mkdir(parents=True, exist_ok=True)
    report = out / 'verification.json'
    record = {'status': 'running', 'boundary':
              'Original801EA480 body versus native orchestration, using the Windows menu '
              'selection. Query/setup/session services supplied; session recipient not '
              'executed. One case resolves player queries from actual private PLAY records '
              'with side contexts1/1 explicitly supplied. No fresh match creation.'}
    report.write_text(json.dumps(record, indent=2) + '\n')
    try:
        before = baseline_hashes()
        selection = private(PRIVATE / 'research-temp/nflstreet-team-field-picker-join-20261011/field-picker-window-selection.json')
        database = private(PRIVATE / 'private/nflstreet/GNNE69-r0/unpacked/db_teams/00001.bin')
        inputs = [selection, database] + [private(PRIVATE / f'research-temp/position-neutral-{c}.mem1') for c in (14, 15, 16)]
        hashes = {str(p): digest(p) for p in inputs}
        if not args.no_build:
            run(['bash', str(root / 'build_windows_match_start_original_check.sh'), str(out)], out / 'build.log')
        exe = out / 'match-start-original-check.exe'
        contexts = {}
        for context in (14, 15, 16):
            traces = []
            for repeat in range(2):
                log = out / f'context{context}-{repeat}.log'
                text = run([str(exe), windows(BASELINES['original_dll'][0]),
                            windows(PRIVATE / f'research-temp/position-neutral-{context}.mem1'),
                            windows(selection), windows(database)], log)
                rows = re.findall(r'^([^:\n]+): original/native status=([0-9a-f]+) trace=(\d+) player-queries=(\d+) session=(\d+) update=(\d+); two output arrays exact\.', text, re.M)
                if len(rows) != 17:
                    raise RuntimeError(f'Incomplete startup differential: context{context}: {len(rows)} cases')
                required = [('success', '00000000', '40', '28', '1', '1'),
                            ('initial-query-error', '00000031', '2', '0', '0', '0'),
                            ('nonzero-mode-bypass', '00000000', '3', '0', '0', '1'),
                            ('mode255-bypass', '00000000', '3', '0', '0', '1'),
                            ('actual-DB-player-resolution', '00000000', '40', '28', '1', '1')]
                if any(row not in rows for row in required) or 'Bounded original801EA480 differential passed.' not in text:
                    raise RuntimeError(f'Startup differential missing a required branch: context{context}')
                traces.append(digest(log))
            if traces[0] != traces[1]:
                raise RuntimeError(f'Fresh startup traces differ: context{context}')
            contexts[str(context)] = {'cases': 17, 'fresh_processes': 2, 'log_sha256': traces}
        if before != baseline_hashes() or any(digest(Path(p)) != h for p, h in hashes.items()):
            raise RuntimeError('Protected originals or supplied inputs changed')
        record.update(status='passed', baseline=before, input_sha256=hashes,
                      executable_sha256=digest(exe), contexts=contexts,
                      source_sha256={name: digest(root / name) for name in
                          ('match_start.c', 'match_start.h', 'match_player_query.c',
                           'match_player_query.h', 'match_start_original_check.c', 'team_catalog.c')})
    except Exception as exc:
        record.update(status='failed', error=str(exc))
        raise
    finally:
        report.write_text(json.dumps(record, indent=2) + '\n')
    print('Native startup orchestration passes original comparisons; private report ' + str(report))


if __name__ == '__main__':
    main()
