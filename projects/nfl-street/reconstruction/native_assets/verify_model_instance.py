#!/usr/bin/env python3
"""Verify original model instance initializer and model setter."""
import json
import re
from pathlib import Path
from verify_controller_checks import PRIVATE,BASELINES,baseline_hashes,digest,private,run,windows
from verify_player_startup import source_hashes
EXPECTED=("Model instances: 32 original initialize/select sequences, all240 bytes exact; "
 "4 allocator fills x8 model/palette/material masks; original helpers executed; no full actor or draw.")
def main():
 here=Path(__file__).resolve().parent;root=here.parent;out=private(PRIVATE/'research-temp/nflstreet-model-instance-20261013');out.mkdir(parents=True,exist_ok=True)
 record={'status':'failed','boundary':'Original80162FC4 initialization with all called helpers executing, '
 'followed by801EBFE0 model setter. Compare all240 instance bytes across four allocator fills and eight '
 'model/palette/material token masks. Resource identities are supplied, no table dereferences required. '
 'No full actor creation, natural group selectors or rendering claim.'}

 try:
  before=baseline_hashes();build=here/'build_windows_model_instance_original_check.sh'
  compiled=re.findall(r'"\$root/([^"\n]+\.c)"',build.read_text())
  entries=tuple('native_assets/'+n for n in compiled)+('native_assets/build_windows_model_instance_original_check.sh','native_assets/verify_model_instance.py')
  sources=source_hashes(root,entries);data=PRIVATE/'private/nflstreet/GNNE69-r0'
  files=[]
  contexts=[PRIVATE/f'research-temp/position-neutral-{n}.mem1' for n in (14,15,16)]
  inputs={str(p):digest(p) for p in [*files,*contexts]};run(['bash',str(build),str(out)],out/'build.log')
  exe=out/'model-instance-original-check.exe';logs={}
  for state in contexts:
   traces=[]
   for repeat in range(2):
    log=out/f'{state.stem}-{repeat}.log'
    if run([str(exe),windows(BASELINES['original_dll'][0]),windows(state),*[windows(p) for p in files]],log).strip()!=EXPECTED:raise RuntimeError('Incomplete body oracle')
    traces.append(digest(log))
   if traces[0]!=traces[1]:raise RuntimeError('Fresh body traces differ')
   logs[state.stem]=traces
  if before!=baseline_hashes() or sources!=source_hashes(root,entries) or any(digest(Path(p))!=h for p,h in inputs.items()):raise RuntimeError('Baseline/source/input changed')
  record.update(status='passed',baseline=before,source_sha256=sources,input_sha256=inputs,logs_sha256=logs,executable_sha256=digest(exe),original_sequences_per_process=32,instance_bytes=240,allocator_fills=4,token_masks=8)
 except Exception as exc:record['error']=str(exc);raise
 finally:(out/'verification.json').write_text(json.dumps(record,indent=2)+'\n')
 print(EXPECTED);print('Three original saved contexts repeat twice; protected baseline unchanged.')
if __name__=='__main__':main()
