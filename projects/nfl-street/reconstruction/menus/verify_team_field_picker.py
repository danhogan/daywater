#!/usr/bin/env python3
"""Repeat actual native team completion through source-driven field selection."""
import argparse
import json
import os
import re
import subprocess
from pathlib import Path
from verify_controller_checks import PRIVATE, baseline_hashes, digest, private, run, windows
import sys
sys.path.insert(0,str(Path(__file__).resolve().parent.parent / "native_assets"))
from verify_player_startup import source_hashes


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output-dir', type=Path, help='Private output directory; preserves earlier captures when set.')
    parser.add_argument('--no-build', action='store_true')
    parser.add_argument('--live-window', action='store_true',
                        help='Exercise the actual Win32 event/timer/paint loop with deterministic source inputs.')
    args = parser.parse_args()
    root = Path(__file__).resolve().parent
    out = private(args.output_dir or PRIVATE / 'research-temp/nflstreet-team-field-picker-join-20261011')
    out.mkdir(parents=True, exist_ok=True)
    record = {'status': 'running', 'boundary':
              'Actual native team completion routed to coordinator5 and source field8 recipient. '
              'Source inputs/timers select a field; native record retains both seven-player '
              'lineups and 14-word formations. Explicit no-profile, logical-device, statistic, '
              'clock and archive inputs. Confirm creates an owned native roster that survives menu teardown; '
              'occupied outputs and invalid selection/schema reject. No actors, field load or full original lifecycle.'}
    report = out / 'verification.json'
    try:
        before = baseline_hashes()
        build_source=(root/'build_windows_team_field_picker_join.sh').read_text()
        compiled=re.search(r'sources=\(([^)]+)\)',build_source).group(1).split()
        entries=tuple('menus/'+name+'.c' for name in compiled)+tuple('menus/'+name for name in (
          'team_field_picker_join_probe.c','field_screen_probe.c','field_screen_preview.c',
          'build_windows_team_field_picker_join.sh','verify_team_field_picker.py',
          'team_preview_check.c','build_windows_team_preview_checks.sh'))+('native_assets/texture.c','native_assets/quick_match_session.c','native_assets/quick_match_roster.c','native_assets/game_roster.c','native_assets/verify_player_startup.py',)
        include_dirs=(Path(os.environ.get('DAYWATER_MODERNGEKKO_HEADERS',str(PRIVATE/'tools/ModernGekko-Template/lib/ModernGekko/include'))),root,root.parent/'native_assets')
        sources=source_hashes(root.parent,entries,include_dirs)
        inputs = {
            'team_source': PRIVATE / 'research-temp/nflstreet-original-menus-20261009/nested/uis_all/00000/00007.bin',
            'field_source': PRIVATE / 'research-temp/nflstreet-original-menus-20261009/nested/uis_all/00000/00008.bin',
            'coordinator': PRIVATE / 'research-temp/nflstreet-original-menus-20261009/nested/uis_all/00000/00005.bin',
            'game_schema': PRIVATE / 'private/nflstreet/GNNE69-r0/unpacked/template/00001.bin',
            'template': PRIVATE / 'private/nflstreet/GNNE69-r0/unpacked/template/00002.bin',
        }
        unpacked=PRIVATE/'private/nflstreet/GNNE69-r0/unpacked'
        for i in range(1,33):inputs[f'team_catalog_{i}']=unpacked/f'db_teams/{i:05}.bin'
        for i in range(4):inputs[f'font_{i}']=unpacked/f'uis_font/{i:05}.bin'
        input_hashes = {name: digest(private(path)) for name, path in inputs.items()}
        run(['bash',str(root/'build_windows_team_preview_checks.sh'),str(out)],out/'fallback-build.log')
        fallback=out/'team-preview-check.exe';fallback_logs={}
        db=unpacked/'db_teams'
        text=run([str(fallback),windows(db)],out/'fallback-native.log')
        if '32 private team blocks, 402 PLAY/DIGP roster IDs' not in text:
            raise RuntimeError('Fallback roster parsing regressed')
        for context in (14,15,16):
            log=out/f'fallback-original-{context}.log'
            text=run([str(fallback),windows(PRIVATE/'research-temp/nflstreet-native-windows-local/gGNNE69_recomp.dll'),
                      windows(PRIVATE/f'research-temp/position-neutral-{context}.mem1'),windows(db)],log)
            if 'Original8011D628 roster projection exact: 64' not in text:
                raise RuntimeError('Original fallback projection regressed')
            fallback_logs[str(context)]=digest(log)
        record['fallback_original_logs_sha256']=fallback_logs
        if not args.no_build:
            run(['bash', str(root / 'build_windows_team_field_picker_join.sh'), str(out)], out / 'build.log')
        exe = out / 'team-field-picker-join.exe'
        base = [str(exe), windows(inputs['team_source']),
                windows(PRIVATE / 'private/nflstreet/GNNE69-r0/unpacked'),
                windows(PRIVATE / 'research-temp/nflstreet-native-windows-local/game/sys/main.dol'), '1000',
                windows(inputs['field_source']), windows(inputs['template']), '0', '5,4,5,7']
        cases = []
        for name, sequence, team in [('default', '7,7,7,7', 1),
                                     ('reverse', '4,4,7,4,4,7,7,7', 31),
                                     ('forward', '5,5,7,5,5,7,7,7', 3)]:
            logs = []
            for iteration in range(2):
                log = out / f'{name}-{iteration}.log'
                text = run(base + [sequence], log)
                expected = f'teams={team}/{team}; selected roster counts=7/7; formation arrays retained; source field UI slot=1 raw DIVE key=1;'
                if expected not in text or 'Quick-match input guards pass:' not in text:
                    raise RuntimeError(f'Incomplete team/field selection: {name}')
                retained=f'Native match roster retained after menu teardown: teams={team}/{team}; 14 selected rows; 28 formation mappings; owned STAT; field slot=1 key=1; no actors or field loaded.'
                if retained not in text:raise RuntimeError('Menu-owned match roster did not survive teardown')
                observed = re.findall(r'Field source numeric event([45]) accepted; [^\n]*?root\+0x90 cursor=(\d+)', text)
                if observed != [('5', '1'), ('4', '0'), ('5', '1')]:
                    raise RuntimeError(f'Actual forward/reverse cursor trace differs: {name}: {observed}')
                if 'transition requests=1 completions=1 link effects=1' not in text:
                    raise RuntimeError(f'Field confirmation did not complete its transition: {name}')
                if 'Joined native field source commit request: slot=1 raw DIVE key=1 owner=1; field cache release requests=1; FFFD completed.' not in text:
                    raise RuntimeError(f'Source shutdown did not retain the confirmed field: {name}')
                logs.append(digest(log))
            if logs[0] != logs[1]:
                raise RuntimeError(f'Fresh native selection traces differ: {name}')
            cases.append({'name': name, 'team_events': sequence, 'teams': [team, team],
                          'field_events': '5,4,5,7', 'field_slots': [1, 0, 1], 'field_key': 1,
                          'source_shutdown_commit': {'slot': 1, 'key': 1, 'supplied_owner': 1},
                          'repeated_log_sha256': logs})
        back_prefix = out / 'field-picker-back'
        back_selection = Path(str(back_prefix) + '-selection.json')
        if back_selection.exists():
            raise RuntimeError('Back check prefix already contains a selection snapshot')
        env = dict(os.environ, DAYWATER_FIELD_JOIN_CAPTURE_PREFIX=windows(back_prefix))
        env['WSLENV'] = env.get('WSLENV', '') + ':DAYWATER_FIELD_JOIN_CAPTURE_PREFIX'
        back_log = out / 'back.log'
        with back_log.open('w') as output:
            subprocess.run(base[:8] + ['5,4,5,6', '7,7,7,7'], env=env, stdout=output,
                           stderr=subprocess.STDOUT, timeout=25, check=True)
        if 'Field source back event6 processed; no native field commit or selection snapshot was created.' not in back_log.read_text() or back_selection.exists():
            raise RuntimeError('Back was treated as a confirmed selection')
        record['back_without_commit'] = True
        if args.live_window:
            prefix = out / 'field-picker-window'
            env = dict(os.environ, DAYWATER_FIELD_JOIN_LIVE_SELF_TEST='1',
                       DAYWATER_FIELD_JOIN_CAPTURE_PREFIX=windows(prefix))
            env['WSLENV'] = env.get('WSLENV', '') + ':DAYWATER_FIELD_JOIN_LIVE_SELF_TEST:DAYWATER_FIELD_JOIN_CAPTURE_PREFIX'
            log = out / 'live-window.log'
            with log.open('w') as output:
                subprocess.run(base + ['7,7,7,7'], env=env, stdout=output,
                               stderr=subprocess.STDOUT, timeout=25, check=True)
            text = log.read_text()
            if 'Win32 self-test source FFFD commit completed before window exit: slot=1 key=1 owner=1.' not in text:
                raise RuntimeError('Actual Win32 loop did not complete source shutdown')
            status_file = Path(str(prefix) + '-live-status.txt')
            status = dict(line.split('=', 1) for line in status_file.read_text().splitlines())
            expected_status = {'self_test': '1', 'opened': '1', 'runtime_events': '4',
                               'last_event': '7', 'source_cursor': '1', 'finish_completed': '1',
                               'commit_count': '1', 'commit_slot': '1', 'commit_key': '1'}
            if any(status.get(k) != v for k, v in expected_status.items()) or int(status.get('successful_paint_callbacks', '0')) < 4:
                raise RuntimeError('Actual Win32 status does not demonstrate complete event/paint/finish flow')
            queues = ('resource_queue', 'button_resource_queue', 'audio_queue',
                      'broadcast_queue', 'link_queue', 'focus_queue')
            if any(status.get(k) != '0' for k in queues):
                raise RuntimeError('Win32 host retained consumed effect notifications')
            selection = Path(str(prefix) + '-selection.json')
            saved = json.loads(selection.read_text())
            if saved['field_slot'] != 1 or saved['field_key'] != 1 or saved['team_ids'] != [1, 1]:
                raise RuntimeError('Win32 loop selection record differs')
            if list(map(len, saved['selected_players'])) != [7, 7] or list(map(len, saved['formation_players'])) != [14, 14]:
                raise RuntimeError('Win32 loop lost selected lineups or formations')
            for selected, formation in zip(saved['selected_players'], saved['formation_players']):
                if len(set(selected)) != 7 or set(formation[:7]) != set(selected) or set(formation[7:]) != set(selected):
                    raise RuntimeError('Win32 loop formation membership differs')
            bridge_out=private(out/'roster-bridge')
            run([sys.executable,str(root.parent/'native_assets/verify_quick_match_roster.py'),
                 '--selection',str(selection),'--expected-membership','14','--output-dir',str(bridge_out)],
                out/'roster-bridge.log')
            bridge=json.loads((bridge_out/'verification.json').read_text())
            if bridge['status']!='passed' or bridge['captured_membership_matches']!=14:
                raise RuntimeError('Corrected picker lost STAT membership')
            record['roster_bridge_report_sha256']=digest(bridge_out/'verification.json')
            record['stat_membership_matches']=14
            if 'Native match roster retained after menu teardown: teams=1/1; 14 selected rows; 28 formation mappings; owned STAT; field slot=1 key=1; no actors or field loaded.' not in text:raise RuntimeError('Win32 commit did not retain its native roster')
            record['owned_match_roster']={'status':'passed','selected_rows':14,'formation_mappings':28,'stat_lifetime':'owned beyond menu teardown','actors_created':0,'failure_guards':['occupied output','duplicate selection','invalid schema'],'close_repeats':True}
            record['win32_window'] = {'status': 'passed', 'source_inputs': [5, 4, 5, 7],
                                      'slot': 1, 'raw_key': 1, 'selection_sha256': digest(selection),
                                      'log_sha256': digest(log), 'manual_input_tested': False}
        if before != baseline_hashes() or input_hashes != {name: digest(path) for name, path in inputs.items()}:
            raise RuntimeError('Protected originals or source inputs changed')
        if sources != source_hashes(root.parent,entries,include_dirs):
            raise RuntimeError('Picker sources changed during verification')
        record.update(source_sha256=sources,status='passed', cases=cases, baseline=before, input_sha256=input_hashes,
                      executable_sha256=digest(exe))
    except Exception as exc:
        record.update(status='failed', error=str(exc))
        raise
    finally:
        report.write_text(json.dumps(record, indent=2) + '\n')
    print('Native team/field picker join passes; private report ' + str(report))


if __name__ == '__main__':
    main()
