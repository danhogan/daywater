#!/usr/bin/env python3
"""Verify private bound body ownership for both selected native lineups."""
import json
import re
import struct
from pathlib import Path
from verify_controller_checks import PRIVATE, baseline_hashes, digest, private, run, windows
from verify_player_startup import source_hashes

VISUAL_EXPECTED = ("Selected match visuals: 14 simultaneous players, 350 owned slots, 630 material instances; "
            "2162 deferred flushes; source menu buffers released; standalone owners byte-equal. "
            "6 ownership/failure guards; no actors or drawing.")

SKELETON_EXPECTED = ("Selected match skeletons: 14 players, 84 fresh resources, 28 face views and 42 hair views; "
            "210 temporary allocations released; 5 transactional guards; matches independent binding stages; "
            "body skinning and actors remain separate.\n" + VISUAL_EXPECTED)

EXPECTED=("Selected match bodies: 14 private relocated/bound bodies; 42 temporary allocations released; "
 "5 transactional guards; stable body links published; independent owners byte-equal; target0 explicit, no matrices or actors.\n"+SKELETON_EXPECTED)

def main():
    here=Path(__file__).resolve().parent;root=here.parent
    out=private(PRIVATE/'research-temp/nflstreet-match-bodies-20261013');out.mkdir(parents=True,exist_ok=True)
    report={'status':'failed','boundary':'Native aggregate ownership from the actual retained menu selection. '
      'Both selected seven-player lineups load simultaneously, in menu order, with complete GAME records. '
      'Logical host namespaces are reserved per player; callbacks are deferred host notifications, '
      'not verified original cache operations or GPU uploads. Constructor side values 0/1 are explicit '
      'fixture inputs, not an inferred controller mapping. Body variant 0 is explicit. '
      'Borrowed catalogs/archives/body pools remain alive; original menu buffers are released. '
      'Fresh face/hair attachment is now composed for14 players; each owns six initialized source '
      'resources. Face target0/hair target5 and binding token ranges are explicit fixture inputs, '
      'not inferred original actor selection/ownership. Body skinning, actor creation, field load, '
      'drawing and fresh gameplay remain separate.'}
    try:
        before=baseline_hashes();build=here/'build_windows_match_bodies_check.sh'
        compiled=re.findall(r'"\$root/([^"\n]+\.c)"',build.read_text())
        entries=tuple('native_assets/'+name for name in compiled)+('native_assets/build_windows_match_bodies_check.sh','native_assets/verify_match_bodies.py')
        sources=source_hashes(root,entries);refs={}
        for directory in ('nflstreet-body-postattach-20261013','nflstreet-match-skeletons-20261013'):
            path=private(PRIVATE/'research-temp'/directory/'verification.json');r=json.loads(path.read_text())
            if r['status']!='passed' or r['baseline']!=before:raise RuntimeError('Missing baseline-matched stage: '+directory)
            for k,h in r['source_sha256'].items():
                source=PRIVATE/'tools/ModernGekko-Template/lib/ModernGekko/include'/k.removeprefix('ModernGekko/') if k.startswith('ModernGekko/') else root/k
                if digest(source)!=h:raise RuntimeError('Stale stage source: '+str(source))
            refs[str(path)]=digest(path)
        data=PRIVATE/'private/nflstreet/GNNE69-r0'
        assets=[data/'assets'/f'{n}.dat' for n in ('plaface','plarosterhair','platex','plamodel','plahair','animdata','pladyncl')]
        stat=data/'unpacked/template/00002.bin';schema=data/'unpacked/template/00001.bin'
        catalogs=[data/'native'/name for name in ('skeleton-startup.nsc1','gear-texture.gtc4','slot-materials.smc1')]
        selection=private(PRIVATE/'research-temp/nflstreet-menu-match-owner-20261013/field-picker-window-selection.json')
        s=json.loads(selection.read_text());assert s['schema_version']==1
        words=[*s['team_ids'],*(v for a in s['selected_players'] for v in a),*(v for a in s['formation_players'] for v in a),s['field_slot'],s['field_key']]
        if len(words)!=46 or any(type(v)is not int or not 0<=v<=0xffffffff for v in words):raise ValueError('Invalid menu selection')
        capture=private(out/'retained-menu.qms1');capture.write_bytes(b'QMS1'+struct.pack('>47I',1,*words))
        files=[*assets,stat,schema,*catalogs,selection,capture]
        inputs={str(p):digest(p) for p in files}
        run(['bash',str(build),str(out)],out/'build.log');exe=out/'match-bodies-check.exe'
        args=[*assets[:3],data/'unpacked',*assets[3:6],stat,*catalogs,assets[6],schema,capture]
        logs=[]
        for repeat in range(2):
            log=out/f'native-{repeat}.log'
            if run([str(exe),*(windows(p) for p in args)],log).strip()!=EXPECTED:raise RuntimeError('Incomplete aggregate visual check')
            logs.append(digest(log))
        if logs[0]!=logs[1] or sources!=source_hashes(root,entries) or before!=baseline_hashes():raise RuntimeError('Trace/source/baseline changed')
        if any(digest(Path(p))!=h for p,h in inputs.items()) or any(digest(Path(p))!=h for p,h in refs.items()):raise RuntimeError('Private input/stage changed')
        report.update(status='passed',baseline=before,source_sha256=sources,input_sha256=inputs,stage_report_sha256=refs,
          logs_sha256=logs,executable_sha256=digest(exe),simultaneous_players=14,owned_slots=350,material_instances=630,
          deferred_flushes=2162,guard_checks=6,fresh_skeleton_resources=84,bound_face_views=28,bound_hair_views=42,temporary_allocations=210,temporary_releases=210,attachment_guard_checks=5,private_bodies=14,body_temporary_allocations=42,body_temporary_releases=42,body_guard_checks=5,explicit_body_target_index=0,explicit_target_indices=[0,5],teams=s['team_ids'],constructor_sides=[0,1],body_variant=0)
    except Exception as exc:report['error']=str(exc);raise
    finally:(out/'verification.json').write_text(json.dumps(report,indent=2)+'\n')
    print(EXPECTED);print('Fresh Windows processes agree; stage evidence current; protected baseline unchanged.')

if __name__=='__main__':main()
