#!/usr/bin/env python3
"""Verify owned body postattachment against the untouched original module."""
import json
import re
from pathlib import Path
from verify_controller_checks import PRIVATE,BASELINES,baseline_hashes,digest,private,run,windows
from verify_player_startup import source_hashes
EXPECTED=("Owned body bindings: 12 type/variant cases (TBYP0/1/2/7 x3); 24 original801C6B38 calls; "
 "whole files/views and target bytes exact; 36 temporary allocations, storage and release order exact; "
 "16 failure/reuse guards; target0 explicit, no actor creation.")
def main():
 here=Path(__file__).resolve().parent;root=here.parent;out=private(PRIVATE/'research-temp/nflstreet-body-postattach-20261013');out.mkdir(parents=True,exist_ok=True)
 record={'status':'failed','boundary':'Controlled original801C6B38 body binding with explicit fresh target resource0. '
 'All verified TBYP0/1/2/7 and variants0/1/2. Original and native model views start zero; attach preserves unused words. '
 'Original allocator/free services supplied; normal path compares full file/view, target and temporary storage/free order. '
 'Strict host rollback rejects all allocator failure points, including original first-map-allocation success fallback. '
 'No natural target selection, body skinning matrices, actor creation or live integration claimed.'}
 try:
  before=baseline_hashes();build=here/'build_windows_body_postattach_original_check.sh'
  compiled=re.findall(r'"\$root/([^"\n]+\.c)"',build.read_text())
  entries=tuple('native_assets/'+n for n in compiled)+('native_assets/build_windows_body_postattach_original_check.sh','native_assets/verify_body_postattach.py')
  sources=source_hashes(root,entries);data=PRIVATE/'private/nflstreet/GNNE69-r0'
  files=[data/'assets/plamodel.dat',data/'assets/animdata.dat',data/'native/skeleton-startup.nsc1']
  contexts=[PRIVATE/f'research-temp/position-neutral-{n}.mem1' for n in (14,15,16)]
  inputs={str(p):digest(p) for p in [*files,*contexts]};run(['bash',str(build),str(out)],out/'build.log')
  exe=out/'body-postattach-original-check.exe';logs={}
  for state in contexts:
   traces=[]
   for repeat in range(2):
    log=out/f'{state.stem}-{repeat}.log'
    if run([str(exe),windows(BASELINES['original_dll'][0]),windows(state),*[windows(p) for p in files]],log).strip()!=EXPECTED:raise RuntimeError('Incomplete body oracle')
    traces.append(digest(log))
   if traces[0]!=traces[1]:raise RuntimeError('Fresh body traces differ')
   logs[state.stem]=traces
  if before!=baseline_hashes() or sources!=source_hashes(root,entries) or any(digest(Path(p))!=h for p,h in inputs.items()):raise RuntimeError('Baseline/source/input changed')
  record.update(status='passed',baseline=before,source_sha256=sources,input_sha256=inputs,logs_sha256=logs,executable_sha256=digest(exe),body_cases=12,original_calls_per_process=24,temporary_allocations_per_process=36,guard_checks=16,explicit_target_index=0,initial_view_fill=0)
 except Exception as exc:record['error']=str(exc);raise
 finally:(out/'verification.json').write_text(json.dumps(record,indent=2)+'\n')
 print(EXPECTED);print('Three original saved contexts repeat twice; protected baseline unchanged.')
if __name__=='__main__':main()
