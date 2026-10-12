#!/usr/bin/env python3
"""Bounded native sequential-event/preview check; not full original navigation."""
import json
from pathlib import Path
from verify_controller_checks import PRIVATE, baseline_hashes, digest, private, run, windows


def main():
    root = Path(__file__).resolve().parent
    out = private(PRIVATE / 'research-temp/nflstreet-team-source-replay-20261011')
    out.mkdir(parents=True, exist_ok=True)
    report = out / 'verification.json'
    record = {'status': 'running', 'boundary': 'Sequential translated events on supplied logical port 0; native image cache and selected geometry pane. Physical mapping, original pixels and full navigation unverified.'}
    try:
        before = baseline_hashes()
        run(['bash', str(root / 'build_windows_team_screen_preview.sh'), str(out)], out / 'build.log')
        exe = private(out / 'team-screen-preview.exe')
        source = private(PRIVATE / 'research-temp/nflstreet-original-menus-20261009/nested/uis_all/00000/00007.bin')
        unpacked = private(PRIVATE / 'private/nflstreet/GNNE69-r0/unpacked')
        base = [str(exe), windows(source), windows(unpacked), windows(PRIVATE / 'research-temp/nflstreet-native-windows-local/game/sys/main.dol'), '1000']
        pixels = []
        for iteration in range(2):
            prefix = private(out / ('sequential-' + str(iteration)))
            text = run(base + ['--capture', windows(prefix), '--inspect-roster', '--player-inspection', '--source-controls', '--source-events', '2,3,4,5', '--live-check'], out / (prefix.name + '.log'))
            for event in (2, 3, 4, 5):
                if 'Source event' + str(event) + ' accepted;' not in text:
                    raise RuntimeError('Missing sequential source event ' + str(event))
            if 'Native team window: 60 source updates;' not in text or 'Separate selected-player inspection:' not in text:
                raise RuntimeError('Window/selected athlete replay failed')
            if 'Host UI lifetime cache: consumed' not in text:
                raise RuntimeError('Runtime image requests were not consumed')
            pixels.append({'menu': digest(prefix.with_suffix('.bmp')), 'player': digest(Path(str(prefix) + '-selected-player.bmp'))})
        if pixels[0] != pixels[1]:
            raise RuntimeError('Fresh sequential input captures differ')
        if baseline_hashes() != before:
            raise RuntimeError('Protected original baseline changed')
        record.update(status='passed', repeated_pixels=pixels, baseline=before, executable_sha256=digest(exe))
    except Exception as exc:
        record.update(status='failed', error=str(exc))
        raise
    finally:
        report.write_text(json.dumps(record, indent=2) + '\n')
    print('Sequential native source replay passes; report ' + str(report))


if __name__ == '__main__':
    main()
