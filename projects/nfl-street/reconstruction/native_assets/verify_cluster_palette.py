#!/usr/bin/env python3
"""Verify owned body postattachment against the untouched original module."""
import json
import re
from pathlib import Path
from verify_controller_checks import PRIVATE,BASELINES,baseline_hashes,digest,private,run,windows
from verify_player_startup import source_hashes
EXPECTED=("Cluster palettes: 720 original helper calls, 88320 complete matrices bit-exact; "
 "12 body type/variant models x20 root/pose cases x3 output paths; 3 atomic guards; polynomial and root supplied; no actor caller.")
def main():
 here=Path(__file__).resolve().parent;root=here.parent;out=private(PRIVATE/'research-temp/nflstreet-cluster-palette-20261013');out.mkdir(parents=True,exist_ok=True)
 record={'status':'failed','boundary':'Exact processed-model weighted cluster palette arithmetic, original801CA4E8/801CA720. '
 'Explicit current-root matrix stack and original saved-context polynomial. Twelve body type/variant files,20 root/pose '
 'cases and three direct/scratch-copy/scratch-only output paths. Includes stored and perturbed/edge short rotations, '
 'u16 weights beyond4096, translated/rotated/scaled/sheared roots. Cache flush returns and scratch copy service supplied; '
 'every valid matrix word compared, no whole-RAM/copy-cache behavior claim. No original80162760 actor caller, '
 'head/attachment scaling, frame scheduling, actor creation or GPU pixels claimed.'}
 try:
  before=baseline_hashes();build=here/'build_windows_cluster_palette_original_check.sh'
  compiled=re.findall(r'"\$root/([^"\n]+\.c)"',build.read_text())
  entries=tuple('native_assets/'+n for n in compiled)+('native_assets/build_windows_cluster_palette_original_check.sh','native_assets/verify_cluster_palette.py')
  sources=source_hashes(root,entries);data=PRIVATE/'private/nflstreet/GNNE69-r0'
  files=[data/'assets/plamodel.dat']
  contexts=[PRIVATE/f'research-temp/position-neutral-{n}.mem1' for n in (14,15,16)]
  inputs={str(p):digest(p) for p in [*files,*contexts]};run(['bash',str(build),str(out)],out/'build.log')
  exe=out/'cluster-palette-original-check.exe';logs={}
  for state in contexts:
   traces=[]
   for repeat in range(2):
    log=out/f'{state.stem}-{repeat}.log'
    if run([str(exe),windows(BASELINES['original_dll'][0]),windows(state),*[windows(p) for p in files]],log).strip()!=EXPECTED:raise RuntimeError('Incomplete body oracle')
    traces.append(digest(log))
   if traces[0]!=traces[1]:raise RuntimeError('Fresh body traces differ')
   logs[state.stem]=traces
  if before!=baseline_hashes() or sources!=source_hashes(root,entries) or any(digest(Path(p))!=h for p,h in inputs.items()):raise RuntimeError('Baseline/source/input changed')
  record.update(status='passed',baseline=before,source_sha256=sources,input_sha256=inputs,logs_sha256=logs,executable_sha256=digest(exe),body_models=12,root_pose_cases_per_model=20,output_paths=3,original_calls_per_process=720,matrices_per_process=88320,guard_checks=3)
 except Exception as exc:record['error']=str(exc);raise
 finally:(out/'verification.json').write_text(json.dumps(record,indent=2)+'\n')
 print(EXPECTED);print('Three original saved contexts repeat twice; protected baseline unchanged.')
if __name__=='__main__':main()
