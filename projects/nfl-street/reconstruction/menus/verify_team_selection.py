#!/usr/bin/env python3
"""Verify bounded team data/carousel reconstruction; keep evidence private on W:."""
from __future__ import annotations
import argparse
import json
from pathlib import Path
from verify_controller_checks import BASELINES, PRIVATE, baseline_hashes, digest, private, run, windows

def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=PRIVATE / 'research-temp/nflstreet-team-select-20261010')
    parser.add_argument('--no-build', action='store_true')
    args = parser.parse_args()
    out = private(args.output)
    out.mkdir(parents=True, exist_ok=True)
    report = out / 'verification.json'
    record = {'status': 'running', 'boundary': 'Original numeric/text primitives over real private records and original carousel with supplied database services and absent profiles; not original DB initialization or visible team-screen activation.'}
    report.write_text(json.dumps(record, indent=2) + '\n')
    try:
        before = baseline_hashes()
        root = Path(__file__).resolve().parent
        if not args.no_build:
            for kind in ('catalog', 'carousel'):
                run(['bash', str(root / f'build_windows_team_{kind}_checks.sh'), str(out)], out / f'{kind}-build.log')
        assets = private(PRIVATE / 'private/nflstreet/GNNE69-r0/unpacked/db_teams')
        hashes = {str(assets / f'{i:05d}.bin'): digest(assets / f'{i:05d}.bin') for i in range(1, 33)}
        results = {}
        for context in (14, 15, 16):
            snapshot = private(PRIVATE / f'research-temp/position-neutral-{context}.mem1')
            hashes[str(snapshot)] = digest(snapshot)
            checks = {}
            for kind, extra in [('catalog', [windows(assets)]), ('carousel', [windows(assets)])]:
                exe = private(out / f'team-{kind}-original-check.exe')
                hashes[exe.name] = digest(exe)
                result = run([str(exe), windows(BASELINES['original_dll'][0]), windows(snapshot), *extra], out / f'{kind}-context{context}.log')
                expected = 'Team catalog exact:32' if kind == 'catalog' else 'Original 8011CE64 controlled carousel exact:'
                if expected not in result:
                    raise RuntimeError(f'{kind} comparison did not finish in context{context}')
                if kind == 'carousel' and ('6 private MAET catalog checks' not in result or '780 differential comparisons of direct8011CE64 and callback8011E9D4/80000004' not in result):
                    raise RuntimeError('Actual team record carousel comparison did not finish')
                checks[kind] = result.strip()
            results[str(context)] = checks
        exe = private(out / 'team-selection-probe.exe')
        hashes[exe.name] = digest(exe)
        result = run([str(exe), windows(assets)], out / 'native-probe.log')
        if 'Independent native team selection data/carousel pass' not in result:
            raise RuntimeError('Independent native team record/selection probe did not finish')
        after = baseline_hashes()
        if before != after:
            raise RuntimeError('Original baselines changed during verification')
        for pattern in ('team_*.c', 'team_*.h', 'build_windows_team_*_checks.sh', 'verify_team_selection.py'):
            for source in sorted(root.glob(pattern)):
                hashes[str(source)] = digest(source)
        record.update(status='passed', baselines=after, hashes=hashes, original_contexts=results,
                      native_probe=result.strip(), limitations=[
                          'Version8 raw MAET table only; no proprietary assets are checked in.',
                          'Private blocks were previously unpacked; whole-archive and original database query initialization are not reconstructed.',
                          'Native probe uses explicit ID sorting, supplied starting IDs and no-profile SIVT eligibility.',
                          'Three fields retain raw tags FORT/EDRT/VORT; gameplay meanings have not been assigned.',
                          'Team source-screen startup, artwork/portraits, input/confirmation and field-screen handoff remain unfinished.',
                          'These controlled module comparisons do not establish fresh original UI or a playable selected match.'])
        print('Team record readers and bounded native carousel verified across contexts14/15/16; private evidence recorded.')
    except Exception as error:
        record.update(status='failed', error=str(error))
        raise
    finally:
        report.write_text(json.dumps(record, indent=2) + '\n')

if __name__ == '__main__':
    main()
