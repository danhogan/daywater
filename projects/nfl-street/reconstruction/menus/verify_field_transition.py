#!/usr/bin/env python3
"""Check bounded original transition apply/completion in three saved contexts."""
import argparse
import json
from pathlib import Path
from verify_controller_checks import PRIVATE, baseline_hashes, digest, private, run, windows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--no-build', action='store_true')
    args = parser.parse_args()
    root = Path(__file__).resolve().parent
    out = private(PRIVATE / 'research-temp/nflstreet-field-transition-20261011')
    out.mkdir(parents=True, exist_ok=True)
    report = out / 'verification.json'
    record = {'status': 'running', 'boundary':
              'Original 80137B84 apply and 80137854 phase3 unsigned deadline branch / '
              '80137C80 completion writes. Clock/audio/global audio update supplied; '
              'other owner phases, live original timing and match creation excluded.'}
    try:
        before = baseline_hashes()
        if not args.no_build:
            run(['bash', str(root / 'build_windows_team_transition_checks.sh'), str(out)], out / 'build.log')
        exe = out / 'team-transition-original-check.exe'
        cases = []
        for context in (14, 15, 16):
            snapshot = private(PRIVATE / f'research-temp/position-neutral-{context}.mem1')
            original_hash = digest(snapshot)
            log = out / f'original-{context}.log'
            text = run([str(exe), windows(PRIVATE / 'research-temp/nflstreet-native-windows-local/gGNNE69_recomp.dll'),
                        windows(snapshot)], log)
            if 'exact for 1000 caller-owned' not in text or 'completion exact for256' not in text:
                raise RuntimeError(f'Incomplete original transition check context{context}')
            if digest(snapshot) != original_hash:
                raise RuntimeError(f'Original context{context} changed')
            cases.append({'context': context, 'snapshot_sha256': original_hash,
                          'log_sha256': digest(log), 'apply_cases': 1000, 'completion_cases': 256})
        if baseline_hashes() != before:
            raise RuntimeError('Protected original baseline changed')
        record.update(status='passed', cases=cases, baseline=before,
                      executable_sha256=digest(exe),
                      native_source_sha256=digest(root / 'team_transition.c'),
                      oracle_source_sha256=digest(root / 'team_transition_original_check.c'))
    except Exception as exc:
        record.update(status='failed', error=str(exc))
        raise
    finally:
        report.write_text(json.dumps(record, indent=2) + '\n')
    print('Original transition apply/completion checks pass; private report ' + str(report))


if __name__ == '__main__':
    main()
