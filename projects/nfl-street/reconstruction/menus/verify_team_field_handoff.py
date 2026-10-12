#!/usr/bin/env python3
"""Check actual native team-source completion, with field requests retained."""
import json
import re
from pathlib import Path
from verify_controller_checks import PRIVATE, baseline_hashes, digest, private, run, windows


def main():
    root = Path(__file__).resolve().parent
    out = private(PRIVATE / 'research-temp/nflstreet-team-field-handoff-20261011')
    out.mkdir(parents=True, exist_ok=True)
    report = out / 'verification.json'
    record = {'status': 'running', 'boundary':
              'Actual team source input/timers produce completion forwarded to coordinator5. '
              'Preceding controller completion, logical device, clock12345/audio0/context7 supplied. '
              'Field8 load/activation retained; field recipient and match creation absent. '
              'No full original startup or physical input claim.'}
    report.write_text(json.dumps(record, indent=2) + '\n')
    try:
        before = baseline_hashes()
        run(['bash', str(root / 'build_windows_team_field_handoff_probe.sh'), str(out)], out / 'build.log')
        exe = out / 'team-field-handoff-probe.exe'
        base = [str(exe), windows(PRIVATE / 'research-temp/nflstreet-original-menus-20261009/nested/uis_all/00000/00007.bin'),
                windows(PRIVATE / 'private/nflstreet/GNNE69-r0/unpacked'),
                windows(PRIVATE / 'research-temp/nflstreet-native-windows-local/game/sys/main.dol'), '1000']
        cases = {}
        for name, sequence, team in [('default', '7,7,7,7', 1),
                                     ('reverse', '4,4,7,4,4,7,7,7', 31),
                                     ('forward', '5,5,7,5,5,7,7,7', 3)]:
            logs = []
            formations = None
            for iteration in range(2):
                log = out / (name + '-' + str(iteration) + '.log')
                text = run(base + [sequence], log)
                expected = f'teams{team}/{team} lineups7/7 transitions1 broadcasts1 links1'
                if expected not in text or 'Actual native team source completion -> coordinator5 -> field8 load/activation requests pass;' not in text:
                    raise RuntimeError('Missing actual completion or field route: ' + name)
                if 'spawn requests14.' not in text:
                    raise RuntimeError('Completed lineup preview requests differ: ' + name)
                found = re.findall(r'Native handoff side([01]) team(\d+) formation14((?: \d+){14})\n', text)
                current = {int(side): [int(word) for word in words.split()]
                           for side, selected_team, words in found if int(selected_team) == team}
                if set(current) != {0, 1} or any(len(set(words)) != 7 for words in current.values()):
                    raise RuntimeError('Native selected formation handoff differs: ' + name)
                if formations is not None and current != formations:
                    raise RuntimeError('Fresh native formation words differ: ' + name)
                formations = current
                logs.append(digest(log))
            if logs[0] != logs[1]:
                raise RuntimeError('Fresh completed source traces differ: ' + name)
            cases[name] = {'events': sequence, 'teams': [team, team], 'lineups': [7, 7],
                           'transition_requests': 1, 'broadcasts': 1, 'linked_owner': 5,
                           'native_formation_words': formations,
                           'field_load': 8, 'field_activate': 8, 'repeated_log_sha256': logs}
        if baseline_hashes() != before:
            raise RuntimeError('Protected baseline changed')
        record.update(status='passed', cases=cases, baseline=before, executable_sha256=digest(exe))
    except Exception as exc:
        record.update(status='failed', error=str(exc))
        raise
    finally:
        report.write_text(json.dumps(record, indent=2) + '\n')
    print('Actual native team-to-field requests pass; report ' + str(report))


if __name__ == '__main__':
    main()
