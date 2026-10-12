#!/usr/bin/env python3
"""Repeat controller source screen checks, retaining all diagnostics on W:."""
from __future__ import annotations

import argparse
import json
from pathlib import Path

from verify_controller_checks import BASELINES, PRIVATE, baseline_hashes, digest, private, run, windows


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=PRIVATE / 'research-temp/nflstreet-controller-start-20261010')
    parser.add_argument('--no-build', action='store_true')
    args = parser.parse_args()
    out = private(args.output)
    out.mkdir(parents=True, exist_ok=True)
    report = out / 'screen-verification.json'
    record = {'status': 'running', 'boundary': 'Controlled source screen checks with supplied font resources, static archive flags, profiles and devices; no live menu or visible runtime claim.'}
    report.write_text(json.dumps(record, indent=2) + '\n')
    try:
        before = baseline_hashes()
        root = Path(__file__).resolve().parent
        if not args.no_build:
            run(['bash', str(root / 'build_windows_controller_screen_checks.sh'), str(out)], out / 'screen-build.log')
        coordinator = private(PRIVATE / "research-temp/nflstreet-original-menus-20261009/nested/uis_all/00000/00005.bin")
        source = private(PRIVATE / 'research-temp/nflstreet-original-menus-20261009/nested/uis_all/00000/00006.bin')
        assets = private(PRIVATE / 'private/nflstreet/GNNE69-r0/unpacked')
        hashes = {str(source): digest(source), str(coordinator): digest(coordinator)}
        for entry in range(4):
            font = private(assets / f'uis_font/{entry:05d}.bin')
            hashes[str(font)] = digest(font)
        original = private(out / 'controller-start-original-check.exe')
        preview = private(out / "controller-preview.exe")
        probe = private(out / 'controller-start-probe.exe')
        hashes[original.name], hashes[probe.name], hashes[preview.name] = digest(original), digest(probe), digest(preview)
        results = {}
        for context in (14, 15, 16):
            snapshot = private(PRIVATE / f'research-temp/position-neutral-{context}.mem1')
            hashes[str(snapshot)] = digest(snapshot)
            result = run([str(original), windows(BASELINES['original_dll'][0]), windows(snapshot), windows(source), windows(assets)], out / f'screen-context{context}.log')
            if 'Controller construction and child-first FFFE exact:' not in result or 'Selected FFFC scripts=1;160 timedupdates exact;device requests=2601;broadcasts=1.' not in result:
                raise RuntimeError(f'Context{context} returned no source screen comparison summary')
            if "Controller source inputs exact:5 translated press events and100 timedupdates;" not in result:
                raise RuntimeError(f"Context{context} did not finish source input and delayed completion checks")
            results[str(context)] = result.strip()
        result = run([str(probe), windows(source), windows(assets), windows(coordinator)], out / 'probe.log')
        if 'Native controller160 scheduler updates pass' not in result or 'Independent source controller completion -> Quick Game coordinator5 -> team7' not in result:
            raise RuntimeError('Independent source probe did not finish its scheduler checks')
        rendered = run([str(preview), windows(source), windows(assets), '--captures', windows(out / 'controller')], out / 'preview.log')
        if 'Native controller source preview:startup,5 translated inputs,6 diagnostic rendered captures,delayed completion/link5;' not in rendered:
            raise RuntimeError('Controller preview did not finish its source capture sequence')
        for phase in range(6):
            capture = private(out / f'controller-{phase}.bmp')
            hashes[str(capture)] = digest(capture)
        for folder, count in (('uis_comn', 16), ('uis_butt', 13), ('uis_ctrl', 4), ('uis_fron', 11)):
            for entry in range(count):
                resource = private(assets / folder / f'{entry:05d}.bin')
                hashes[str(resource)] = digest(resource)
        after = baseline_hashes()
        if before != after:
            raise RuntimeError('Original baselines changed during the screen checks')
        # Include shared VM/classes and build/driver provenance, not only game callbacks.
        for pattern in ('*.c', '*.h', 'build_windows_controller_screen_checks.sh', 'verify_controller_screen.py'):
            for path in sorted(root.glob(pattern)):
                hashes[str(path)] = digest(path)
        record.update(status='passed', baselines=after, hashes=hashes, original_contexts=results,
                      independent_probe=result.strip(), independent_rendered_captures=rendered.strip(),
                      limitations=['Resource I/O, profiles and device responses are supplied or retained at explicit boundaries.',
                                   'Independent probe execution alone does not establish original matching behavior.',
                                   'Translated source input uses supplied logical port0, float1 and query responses; hardware discovery/translation and live UI are unverified.',
                                   'Controller captures use the diagnostic host rasterizer and supplied profiles/device; interactive desktop input and original GX rendering are unverified.',
                                   'Main-window integration, visible team/field selectors and selected-match startup remain unfinished.'])
        print('Controller source screen verification passed across contexts14/15/16; private evidence recorded.')
    except Exception as error:
        record.update(status='failed', error=str(error))
        raise
    finally:
        report.write_text(json.dumps(record, indent=2) + '\n')


if __name__ == '__main__':
    main()
