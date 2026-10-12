#!/usr/bin/env python3
"""Verify native team startup and source class construction on private W: inputs."""
from __future__ import annotations
import argparse
import json
from pathlib import Path
from verify_controller_checks import BASELINES, PRIVATE, baseline_hashes, digest, private, run, windows

def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=PRIVATE / 'research-temp/nflstreet-team-start-20261010')
    parser.add_argument('--no-build', action='store_true')
    args = parser.parse_args()
    out = private(args.output)
    out.mkdir(parents=True, exist_ok=True)
    report = out / 'verification.json'
    record = {'status': 'running', 'boundary': 'Controlled no-profile original startup and source image/text construction; database I/O, random/profile queries and actor previews supplied or omitted. Not full visible menu activation.'}
    report.write_text(json.dumps(record, indent=2) + '\n')
    try:
        before = baseline_hashes()
        root = Path(__file__).resolve().parent
        if not args.no_build:
            for kind in ('start', 'classes'):
                run(['bash', str(root / f'build_windows_team_{kind}_checks.sh'), str(out)], out / f'{kind}-build.log')
        assets = private(PRIVATE / 'private/nflstreet/GNNE69-r0/unpacked')
        db = private(assets / 'db_teams')
        screen = private(PRIVATE / 'research-temp/nflstreet-original-menus-20261009/nested/uis_all/00000/00007.bin')
        hashes = {str(screen): digest(screen)}
        for i in range(1, 33):
            path = db / f'{i:05d}.bin'
            hashes[str(path)] = digest(path)
        for i in range(4):
            path = assets / f'uis_font/{i:05d}.bin'
            hashes[str(path)] = digest(path)
        results = {}
        for context in (14, 15, 16):
            snapshot = private(PRIVATE / f'research-temp/position-neutral-{context}.mem1')
            hashes[str(snapshot)] = digest(snapshot)
            checks = {}
            for kind, extra, marker in [
                ('start-original', [windows(db)], 'Team startup original8011C414 exact:48'),
                ('classes', [windows(screen), windows(assets)], 'Team image/text original construction exact:')]:
                exe = private(out / f'team-{kind}-check.exe')
                hashes[exe.name] = digest(exe)
                result = run([str(exe), windows(BASELINES['original_dll'][0]), windows(snapshot), *extra], out / f'{kind}-context{context}.log')
                if marker not in result:
                    raise RuntimeError(f'{kind} check did not finish in context{context}')
                if kind == 'start-original' and 'plus48 original8011E9D4/80000001 callback comparisons' not in result:
                    raise RuntimeError('Original startup callback dispatch did not finish')
                checks[kind] = result.strip()
            results[str(context)] = checks
        exe = private(out / 'team-start-probe.exe')
        hashes[exe.name] = digest(exe)
        probe = run([str(exe), windows(db)], out / 'native-start-probe.log')
        if 'Independent native original team-start/carousel join passes' not in probe or 'Native team callback bridge passes:' not in probe:
            raise RuntimeError('Independent startup/carousel join did not finish')
        classes = private(out / 'team-classes-check.exe')
        native_classes = run([str(classes), windows(screen), windows(assets)], out / 'native-classes.log')
        if 'Independent native team image/text construction passes:' not in native_classes:
            raise RuntimeError('Independent class construction did not finish')
        after = baseline_hashes()
        if before != after:
            raise RuntimeError('Original baselines changed during verification')
        for pattern in ('team_*.c', 'team_*.h', 'build_windows_team_*_checks.sh', 'verify_team_start.py'):
            for source in sorted(root.glob(pattern)):
                hashes[str(source)] = digest(source)
        for source in ('construct.c', 'screen.c', 'font.c', 'text.c', 'text_construct.c', 'image_construct.c',
                       'text_lines.c', 'construct.h', 'screen.h', 'font.h', 'text.h', 'text_construct.h',
                       'image_construct.h', 'text_lines.h'):
            hashes[str(root / source)] = digest(root / source)
        for source in ('texture.c', 'texture.h'):
            path = root.parent / 'native_assets' / source
            hashes[str(path)] = digest(path)
        record.update(status='passed', baselines=after, hashes=hashes, original_contexts=results,
                      native_start=probe.strip(), native_classes=native_classes.strip(), limitations=[
                          'Startup branch: absent profiles, profile mode6, supplied 80149AF0=true, 80150A00=false, random0, actor gatefalse.',
                          'Catalogs are supplied; original whole-archive/query initialization and profile/unlock database bootstrap remain separate.',
                          'Class construction uses a supplied registry containing image0/text8; class3 actor previews and original resource I/O are omitted. Only line-table allocator addresses are normalized; every allocated range is compared.',
                          'Source FFFE/FFFC scripts, timers, rendered portraits, source input/confirmation and field-screen handoff are not yet connected.',
                          'Controlled original-code comparisons are not fresh original menu captures or selected-match startup.'])
        print('Team startup and image/text construction verified across contexts14/15/16; independent native joins pass and baselines are unchanged.')
    except Exception as error:
        record.update(status='failed', error=str(error))
        raise
    finally:
        report.write_text(json.dumps(record, indent=2) + '\n')

if __name__ == '__main__':
    main()
