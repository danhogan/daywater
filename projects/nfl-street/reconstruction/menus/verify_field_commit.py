#!/usr/bin/env python3
"""Repeat bounded original selected-field transfer; never execute match creation."""
import argparse
import json
from pathlib import Path
from verify_controller_checks import PRIVATE, BASELINES, baseline_hashes, digest, private, run, windows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--no-build', action='store_true')
    parser.add_argument('--selection', type=Path,
                        default=PRIVATE / 'research-temp/nflstreet-team-field-picker-join-20261011/field-picker-selection.json')
    args = parser.parse_args()
    root = Path(__file__).resolve().parent
    out = private(PRIVATE / 'research-temp/nflstreet-field-commit-original-20261011')
    out.mkdir(parents=True, exist_ok=True)
    report = out / 'verification.json'
    record = {'status': 'running', 'boundary':
              'Original8011F170 selected-input transfer with native formation arrays seeded '
              'into saved RAM and owner mode1 supplied. Cleanup/settings/init/free boundaries '
              'captured. 801EA480 captured without execution; no fresh match creation.'}
    report.write_text(json.dumps(record, indent=2) + '\n')
    try:
        before = baseline_hashes()
        selection = private(args.selection)
        hashes = {str(selection): digest(selection)}
        selected = json.loads(selection.read_text())
        slot, key = selected['field_slot'], selected['field_key']
        if not 0 <= slot < 8 or not 0 <= key < 8 or list(map(len, selected['formation_players'])) != [14, 14]:
            raise RuntimeError('Native snapshot is not a complete bounded selection')
        if not args.no_build:
            run(['bash', str(root / 'build_windows_field_commit_original_check.sh'), str(out)], out / 'build.log')
        exe = out / 'field-commit-original-check.exe'
        contexts = {}
        for context in (14, 15, 16):
            snapshot = private(PRIVATE / f'research-temp/position-neutral-{context}.mem1')
            hashes[str(snapshot)] = digest(snapshot)
            text = run([str(exe), windows(BASELINES['original_dll'][0]), windows(snapshot), windows(selection)],
                       out / f'context{context}.log')
            expected = f'Original8011F170 selected-field commit matched native handoff: slot={slot} DOL key={key}; copied formation arrays=2x14 words exact;'
            if expected not in text or '801EA480 was captured and not executed' not in text:
                raise RuntimeError(f'Original transfer comparison incomplete: context{context}')
            contexts[str(context)] = {'slot': slot, 'raw_key': key, 'formation_words': 28}
        if before != baseline_hashes() or any(digest(Path(p)) != h for p, h in hashes.items()):
            raise RuntimeError('Protected originals or supplied inputs changed')
        record.update(status='passed', baseline=before, input_sha256=hashes,
                      executable_sha256=digest(exe), contexts=contexts,
                      checker_sha256=digest(root / 'field_commit_original_check.c'))
    except Exception as exc:
        record.update(status='failed', error=str(exc))
        raise
    finally:
        report.write_text(json.dumps(record, indent=2) + '\n')
    print('Original selected-field transfer passes in three contexts; private report ' + str(report))


if __name__ == '__main__':
    main()
