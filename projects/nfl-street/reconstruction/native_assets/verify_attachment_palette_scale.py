#!/usr/bin/env python3
"""Verify original attachment scaling blocks with supplied matrices."""
import json
import re
from pathlib import Path
from verify_controller_checks import PRIVATE,BASELINES,baseline_hashes,digest,private,run,windows
from verify_player_startup import source_hashes
EXPECTED=("Attachment scale: 480 original caller-block runs, 27648 matrices exact in scratch and destination; "
 "4 model slots x4 LOD inputs x5 cluster counts x6 scales; 2 atomic guards; supplied matrices and copy service, no full caller.")
def main():
 here=Path(__file__).resolve().parent;root=here.parent;out=private(PRIVATE/'research-temp/nflstreet-attachment-palette-scale-20261013');out.mkdir(parents=True,exist_ok=True)
 record={'status':'failed','boundary':'Controlled blocks of original80162760 with supplied Clst rows, matrices, actor scale and LOD. '
 'All four exact model-slot scaling blocks execute in the untouched DLL, including slot5A8 LOD skip. '
 'Model generation returns to supplied matrices and scratch copy is performed by the fixture. '
 'Scratch E0000000 has private external-memory callbacks. Distinct scratch and staged areas are tested, '
 'including the slot788 write followed by overwrite. No inferred face/hair slot mapping, complete actor '
 'caller, original matrix generation here, natural frame state or original GPU pixels claimed.'}

 try:
  before=baseline_hashes();build=here/'build_windows_attachment_palette_scale_original_check.sh'
  compiled=re.findall(r'"\$root/([^"\n]+\.c)"',build.read_text())
  entries=tuple('native_assets/'+n for n in compiled)+('native_assets/build_windows_attachment_palette_scale_original_check.sh','native_assets/verify_attachment_palette_scale.py')
  sources=source_hashes(root,entries);data=PRIVATE/'private/nflstreet/GNNE69-r0'
  files=[]
  contexts=[PRIVATE/f'research-temp/position-neutral-{n}.mem1' for n in (14,15,16)]
  inputs={str(p):digest(p) for p in [*files,*contexts]};run(['bash',str(build),str(out)],out/'build.log')
  exe=out/'attachment-palette-scale-original-check.exe';logs={}
  for state in contexts:
   traces=[]
   for repeat in range(2):
    log=out/f'{state.stem}-{repeat}.log'
    if run([str(exe),windows(BASELINES['original_dll'][0]),windows(state),*[windows(p) for p in files]],log).strip()!=EXPECTED:raise RuntimeError('Incomplete body oracle')
    traces.append(digest(log))
   if traces[0]!=traces[1]:raise RuntimeError('Fresh body traces differ')
   logs[state.stem]=traces
  if before!=baseline_hashes() or sources!=source_hashes(root,entries) or any(digest(Path(p))!=h for p,h in inputs.items()):raise RuntimeError('Baseline/source/input changed')
  record.update(status='passed',baseline=before,source_sha256=sources,input_sha256=inputs,logs_sha256=logs,executable_sha256=digest(exe),model_slots=4,lod_inputs=4,cluster_counts=[1,2,7,23,255],scale_inputs=6,original_blocks_per_process=480,matrices_per_process=27648,guard_checks=2)
 except Exception as exc:record['error']=str(exc);raise
 finally:(out/'verification.json').write_text(json.dumps(record,indent=2)+'\n')
 print(EXPECTED);print('Three original saved contexts repeat twice; protected baseline unchanged.')
if __name__=='__main__':main()
