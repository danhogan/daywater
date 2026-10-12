#!/usr/bin/env python3
"""Verify full field image/text construction against three original contexts."""
import argparse
import json
from pathlib import Path
from verify_controller_checks import PRIVATE, baseline_hashes, digest, private, run, windows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--no-build', action='store_true')
    args = parser.parse_args()
    root = Path(__file__).resolve().parent
    out = private(PRIVATE / 'research-temp/nflstreet-field-classes-20261011')
    out.mkdir(parents=True, exist_ok=True)
    report = out / 'verification.json'
    record = {'status': 'running', 'boundary':
              'Complete source bytes and class0/class8 constructor effects compared with original. '
              'Fonts/resource services and line allocation are supplied. Original line ranges checked; '
              'line pointers normalized. Class3 actors, activation, navigation and rendering excluded.'}
    report.write_text(json.dumps(record, indent=2) + '\n')
    try:
        before = baseline_hashes()
        source = private(PRIVATE / 'research-temp/nflstreet-original-menus-20261009/nested/uis_all/00000/00008.bin')
        source_hash = digest(source)
        if not args.no_build:
            run(['bash', str(root / 'build_windows_field_classes_checks.sh'), str(out)], out / 'build.log')
        exe = out / 'field-classes-original-check.exe'
        contexts = {}
        for context in (14, 15, 16):
            snapshot = private(PRIVATE / f'research-temp/position-neutral-{context}.mem1')
            log = out / f'windows-context{context}.log'
            text = run([str(exe), windows(PRIVATE / 'research-temp/nflstreet-native-windows-local/gGNNE69_recomp.dll'),
                        windows(snapshot), windows(source), windows(PRIVATE / 'private/nflstreet/GNNE69-r0/unpacked')], log)
            if '32914-byte complete source file' not in text or '170 ordered class callbacks' not in text or '47 ordered' not in text:
                raise RuntimeError('Missing complete field constructor comparison')
            contexts[str(context)] = {'snapshot_sha256': digest(snapshot), 'log_sha256': digest(log)}
        if baseline_hashes() != before or digest(source) != source_hash:
            raise RuntimeError('Protected original/source baseline changed')
        record.update(status='passed', contexts=contexts, source_sha256=source_hash,
                      baseline=before, executable_sha256=digest(exe))
    except Exception as exc:
        record.update(status='failed', error=str(exc))
        raise
    finally:
        report.write_text(json.dumps(record, indent=2) + '\n')
    print('Field image/text construction passes; private report ' + str(report))


if __name__ == '__main__':
    main()
