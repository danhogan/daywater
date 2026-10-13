#!/usr/bin/env python3
"""Verify native menu-to-GAME mapping and original lineup bit decisions."""
import argparse
import json
import re
import struct
from pathlib import Path
from verify_controller_checks import PRIVATE,BASELINES,baseline_hashes,digest,private,run,windows
from verify_player_startup import source_hashes
EXPECTED = ("Quick-match rosters: 387 menu selections, 5418 selected GAME rows, 10836 formation mappings; "
            "{rejections} rejection checks; 8192 lineup-bit cases. STAT unchanged; no actors created.\n"
            "Retained native menu snapshot: {membership}/14 ids match STAT team membership; {route}.")
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--selection',type=Path,default=PRIVATE/'research-temp/nflstreet-team-field-picker-join-20261011/field-picker-window-selection.json')
    parser.add_argument('--expected-membership',type=int,choices=(12,14),default=12)
    parser.add_argument('--output-dir',type=Path,default=PRIVATE/'research-temp/nflstreet-quick-match-roster-20261013')
    args=parser.parse_args();expected=EXPECTED.format(membership=args.expected_membership,rejections=392+(args.expected_membership!=14),route="both native GAME lineups created with distinct ids" if args.expected_membership==14 else "invalid membership rejects without creating a roster")
    here=Path(__file__).resolve().parent;root=here.parent
    out=private(args.output_dir);out.mkdir(parents=True,exist_ok=True)
    sources=source_hashes(root,tuple('native_assets/'+n for n in ('quick_match_roster.c','quick_match_roster_check.c',
      'quick_match_roster_original_check.c','game_roster.c','actor_model_init.c','gear_texture.c','headwear_state.c','archive.c','lzh1.c',
      'build_windows_quick_match_roster_check.sh','build_windows_quick_match_roster_original_check.sh','verify_quick_match_roster.py'))+
      ('menus/team_catalog.c','menus/quick_match_selection.c'))
    before=baseline_hashes();record={'status':'failed','boundary':'Native QuickMatchSelection to GAME row/id/order mapping. '
      'Original8014FD50 scalar bit operation executes with getter/writer services supplied, not an original menu database update. '
      'Synthetic menu inputs through existing selection builder; mirrored teams supported with distinct GAME ids; legacy wrong-team ids rejected. '
      'No visible menu-to-match startup, actor creation, field rendering or gameplay claimed.'}
    try:
        data=PRIVATE/'private/nflstreet/GNNE69-r0'
        stat=data/'unpacked/template/00002.bin';schema=data/'unpacked/template/00001.bin'
        selection=private(args.selection)
        s=json.loads(selection.read_text());assert s['schema_version']==1
        words=[*s['team_ids'],*(v for a in s['selected_players'] for v in a),*(v for a in s['formation_players'] for v in a),s['field_slot'],s['field_key']]
        if len(words)!=46 or any(type(v) is not int or not 0<=v<=0xffffffff for v in words):raise ValueError('Invalid selection')
        capture=private(out/'retained-menu.qms1');capture.write_bytes(b'QMS1'+struct.pack('>47I',1,*words))
        contexts=[PRIVATE/f'research-temp/position-neutral-{i}.mem1' for i in (14,15,16)]
        inputs={str(p):digest(p) for p in (stat,schema,selection,capture,*contexts)}
        stage=private(PRIVATE/'research-temp/nflstreet-game-roster-20261012/verification.json');r=json.loads(stage.read_text())
        if r['status']!='passed' or r['baseline']!=before:raise RuntimeError('Original GAME-roster evidence unavailable')
        for key,value in r['source_sha256'].items():
            path=PRIVATE/'tools/ModernGekko-Template/lib/ModernGekko/include'/key.removeprefix('ModernGekko/') if key.startswith('ModernGekko/') else root/key
            if digest(path)!=value:raise RuntimeError('Original GAME-roster source evidence stale')
        for name in ('quick_match_roster','quick_match_roster_original'):
            run(['bash',str(here/f'build_windows_{name}_check.sh'),str(out)],out/f'{name}-build.log')
        native=out/'quick-match-roster-check.exe';original=out/'quick-match-roster-original-check.exe';logs={}
        for name,exe,states in [('native',native,[None]),('original',original,contexts)]:
            for state in states:
                group=[]
                for repeat in range(2):
                    tag='native' if state is None else state.stem;log=out/f'{name}-{tag}-{repeat}.log'
                    paths=[stat,schema,*([] if state is None else [BASELINES['original_dll'][0],state]),capture]
                    if run([str(exe),*[windows(p) for p in paths],str(args.expected_membership)],log).strip()!=expected:raise RuntimeError('Incomplete roster comparison')
                    group.append(digest(log))
                if group[0]!=group[1]:raise RuntimeError('Fresh process traces differ')
                logs[name+'-'+tag]=group
        if before!=baseline_hashes() or any(digest(Path(p))!=v for p,v in inputs.items()):raise RuntimeError('Baseline/input changed')
        if any(digest(root/k if not k.startswith('ModernGekko/') else PRIVATE/'tools/ModernGekko-Template/lib/ModernGekko/include'/k.removeprefix('ModernGekko/'))!=v for k,v in sources.items()):raise RuntimeError('Source changed')
        record.update(status='passed',baseline=before,input_sha256=inputs,source_sha256=sources,
          game_roster_report_sha256=digest(stage),logs_sha256=logs,executable_sha256={p.name:digest(p) for p in (native,original)},
          selections_per_process=387,selected_rows_per_process=5418,formation_mappings_per_process=10836,
          rejection_checks_per_process=392+(args.expected_membership!=14),original_scalar_cases_per_process=8192,captured_membership_matches=args.expected_membership,captured_player_ids=14)
    except Exception as exc:record['error']=str(exc);raise
    finally:(out/'verification.json').write_text(json.dumps(record,indent=2)+'\n')
    print(expected);print('Original setter checks repeat across three contexts; protected baseline unchanged.')
if __name__=='__main__':main()
