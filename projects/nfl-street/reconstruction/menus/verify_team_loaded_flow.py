#!/usr/bin/env python3
"""Repeat the native loaded team flow; this is not an original startup oracle."""
import json
import re
from pathlib import Path
from verify_controller_checks import PRIVATE, baseline_hashes, digest, private, run, windows


def main():
    root = Path(__file__).resolve().parent
    out = private(PRIVATE / 'research-temp/nflstreet-team-loaded-flow-20261011')
    out.mkdir(parents=True, exist_ok=True)
    record = {'status': 'running', 'boundary':
              'Fresh independent Windows source startup with original forward-load arguments, '
              'supplied logical device/profile services, source events and separate untextured '
              'athlete inspection. No original full-startup, physical button mapping, field '
              'handoff or complete match claim.'}
    report = out / 'verification.json'
    report.write_text(json.dumps(record, indent=2) + '\n')
    try:
        before = baseline_hashes()
        run(['bash', str(root / 'build_windows_team_screen_preview.sh'), str(out)], out / 'build.log')
        exe = private(out / 'team-screen-preview.exe')
        base = [str(exe), windows(private(PRIVATE / 'research-temp/nflstreet-original-menus-20261009/nested/uis_all/00000/00007.bin')),
                windows(private(PRIVATE / 'private/nflstreet/GNNE69-r0/unpacked')),
                windows(private(PRIVATE / 'research-temp/nflstreet-native-windows-local/game/sys/main.dol')),
                '1000', '--quick-game-start', '--player-inspection', '--source-controls', '--live-check']
        cases = {}
        for name, events, team, play, key, entry, triangles in [
                ('defaults', '7,7,7', 1, 15, 2, 7, 15524),
                ('changed', '4,4,7,4,4,7,7', 31, 318, 0, 1, 15823),
                ('opposite', '5,5,7,5,5,7,7', 3, 39, 0, 1, 15823)]:
            repeats = []
            for iteration in range(2):
                prefix = private(out / (name + '-' + str(iteration)))
                text = run(base + ['--source-events', events, '--capture', windows(prefix)], out / (prefix.name + '.log'))
                expected = f'Source replay after updates: selected team IDs{team}/{team}; lineup sizes7/7; athlete requests14.'
                if expected not in text:
                    raise RuntimeError('Team/lineup source state mismatch: ' + name)
                if not re.search(r'Separate selected-player inspection: slot12 PLAY' + str(play) + r'\b', text):
                    raise RuntimeError('Selected athlete identity mismatch: ' + name)
                if f'Source preview slot12 PLAY{play} body key{key} entry{entry} triangles{triangles};' not in text:
                    raise RuntimeError('Selected athlete geometry binding mismatch: ' + name)
                paints = re.search(r'Native team window: 60 source updates; (\d+) successful paints;', text)
                if not paints or int(paints.group(1)) < 1:
                    raise RuntimeError('Native window update/paint failed: ' + name)
                repeats.append({'menu': digest(prefix.with_suffix('.bmp')),
                                'player': digest(Path(str(prefix) + '-selected-player.bmp'))})
            if repeats[0] != repeats[1]:
                raise RuntimeError('Fresh repeated captures differ: ' + name)
            cases[name] = {'teams': [team, team], 'lineups': [7, 7], 'athlete_requests': 14,
                           'selected_play': play, 'body_key': key, 'body_entry': entry,
                           'triangles': triangles, 'events': events, 'pixels': repeats[0]}
        # PLAY39 and PLAY318 share body key0/entry1 in this flat inspection;
        # different identities need not produce different untextured pixels.
        if cases['defaults']['pixels']['player'] == cases['changed']['pixels']['player']:
            raise RuntimeError('Changed athlete rendered identical inspection pixels')
        if baseline_hashes() != before:
            raise RuntimeError('Protected original baseline changed')
        record.update(status='passed', cases=cases, baseline=before, executable_sha256=digest(exe))
    except Exception as exc:
        record.update(status='failed', error=str(exc))
        raise
    finally:
        report.write_text(json.dumps(record, indent=2) + '\n')
    print('Native loaded team flow passes; report ' + str(report))


if __name__ == '__main__':
    main()
