#!/usr/bin/env python3
"""Repeat original session and joined startup comparisons with explicit owners."""
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
    out = private(PRIVATE / 'research-temp/nflstreet-match-session-20261012')
    out.mkdir(parents=True, exist_ok=True)
    report = out / 'verification.json'
    record = {'status': 'running', 'boundary':
              'Original801E577C body and joined801EA480 caller versus native '
              'orchestration. Explicit controlled settings/query/resource/stage services, '
              'synthetic distinct rosters, no actor or playable match creation.'}
    report.write_text(json.dumps(record, indent=2) + '\n')
    try:
        before = baseline_hashes()
        inputs = [private(PRIVATE / f'research-temp/position-neutral-{c}.mem1') for c in (14, 15, 16)]
        hashes = {str(p): digest(p) for p in inputs}
        if not args.no_build:
            run(['bash', str(root / 'build_windows_match_session_original_check.sh'), str(out)], out / 'build.log')
        exe = out / 'match-session-original-check.exe'
        contexts = {}
        for context in (14, 15, 16):
            traces = []
            for repeat in range(2):
                log = out / f'root-context{context}-{repeat}.log'
                text = run([str(exe), windows(BASELINES['original_dll'][0]),
                            windows(PRIVATE / f'research-temp/position-neutral-{context}.mem1')], log)
                rows = re.findall(r'^([^:\n]+): exact status=([0-9a-f]+) continuation=(\d+) initialized=(\d+) trace=(\d+) assigned=(\d+) stages=(\d+) joined=(\d+)', text, re.M)
                if len(rows) != 39:
                    raise RuntimeError(f'Incomplete session differential: context{context}')
                by_name = {row[0]: row[1:] for row in rows}
                if by_name.get('joined-success') != ('00000000','1','1','51','1','4','1'):
                    raise RuntimeError('Joined success did not complete the original sequence')
                if by_name.get('joined-cancel-update') != ('00000000','0','1','41','0','0','1'):
                    raise RuntimeError('Original cancellation/update behavior differs')
                if by_name.get('fresh-service-error-801e5628') != ('000000b4','0','1','5','1','0','0'):
                    raise RuntimeError('Preparation error failed to preserve assignment and initialized flag')
                if 'Bounded original801E577C and joined801EA480 differential passed' not in text:
                    raise RuntimeError('Session differential did not report completion')
                traces.append(digest(log))
            if traces[0] != traces[1]:
                raise RuntimeError(f'Fresh session traces differ: context{context}')
            contexts[str(context)] = {'cases': 39, 'fresh_processes': 2, 'log_sha256': traces}
        if before != baseline_hashes() or any(digest(Path(p)) != h for p, h in hashes.items()):
            raise RuntimeError('Protected originals or source contexts changed')
        record.update(status='passed', baseline=before, input_sha256=hashes,
                      executable_sha256=digest(exe), contexts=contexts,
                      source_sha256={name: digest(root / name) for name in
                          ('match_session.c','match_session.h','match_start.c','match_session_original_check.c')})
    except Exception as exc:
        record.update(status='failed', error=str(exc))
        raise
    finally:
        report.write_text(json.dumps(record, indent=2) + '\n')
    print('Native session and joined startup comparisons pass; private report ' + str(report))


if __name__ == '__main__':
    main()
