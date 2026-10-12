#!/usr/bin/env python3
"""Exercise independent fresh entry7 startup; retain private input provenance."""
from __future__ import annotations
import argparse,json
from pathlib import Path
from verify_controller_checks import PRIVATE,BASELINES,baseline_hashes,digest,private,run,windows

def main():
 parser=argparse.ArgumentParser(description=__doc__)
 parser.add_argument('--output',type=Path,default=PRIVATE/'research-temp/nflstreet-team-startup-20261011')
 args=parser.parse_args();out=private(args.output);out.mkdir(parents=True,exist_ok=True)
 report=out/'verification.json';record={'status':'running','confidence':'verified for bounded native startup exercise',
  'boundary':'Independent Windows C executes complete original entry7 source creation/activation and passive timed scripts using fresh original DB/DOL inputs. Original engine and savestates are absent from native execution. TSTR selector0 is a supplied seed outcome; factory/audio/image resource effects are requests. Full original startup differential, focus/input and visible selected-athlete rendering are not claimed.'}
 report.write_text(json.dumps(record,indent=2)+'\n')
 try:
  before=baseline_hashes();root=Path(__file__).resolve().parent
  source=private(PRIVATE/'research-temp/nflstreet-original-menus-20261009/nested/uis_all/00000/00007.bin')
  unpacked=private(PRIVATE/'private/nflstreet/GNNE69-r0/unpacked')
  paths=[*[PRIVATE/f'research-temp/position-neutral-{c}.mem1' for c in (14,15,16)],source,BASELINES['dol'][0],unpacked/'template/00002.bin',*[unpacked/f'db_teams/{i:05}.bin' for i in range(1,33)],*[unpacked/f'uis_font/{i:05}.bin' for i in range(4)]]
  hashes={str(p):digest(p) for p in paths}
  hashes.update({str(p):digest(p) for pattern in ('*.c','*.h','*.sh','*.py') for p in root.glob(pattern)})
  run(['bash',str(root/'build_windows_team_screen_checks.sh'),str(out)],out/'build.log')
  exe=private(out/'team-screen-probe.exe');hashes[str(exe)]=digest(exe);results={}
  for frames in (160,1000):
   result=run([str(exe),windows(source),windows(unpacked),windows(BASELINES['dol'][0]),str(frames)],out/f'native-{frames}.log')
   if 'Fresh authentic-data lineup/formation checks pass: 320' not in result or f'creation/activation and{frames} updates pass' not in result or 'lineup sizes7/7, spawn requests14' not in result:raise RuntimeError('Native startup completion marker missing')
   results[str(frames)]=result.strip()
  run(['bash',str(root/'build_windows_team_screen_preview.sh'),str(out)],out/'preview-build.log')
  preview=private(out/'team-screen-preview.exe');hashes[str(preview)]=digest(preview)
  result=run([str(preview),windows(source),windows(unpacked),windows(BASELINES['dol'][0]),'1000','--capture',windows(out/'team-startup')],out/'preview.log')
  capture=private(out/'team-startup.bmp')
  if '2D team-menu startup capture:' not in result or not capture.is_file():raise RuntimeError('Diagnostic team startup capture missing')
  hashes[str(capture)]=digest(capture);record['diagnostic_capture']={'path':str(capture),'sha256':digest(capture),'boundary':'Original source artwork/font objects, host affine rasterizer; no original GX pixel or selected-athlete 3D comparison.'}
  run(['bash',str(root/'build_windows_team_bootstrap_weights_checks.sh'),str(out)],out/'bootstrap-build.log')
  original_exe=private(out/'team-bootstrap-weights-original-check.exe');hashes[str(original_exe)]=digest(original_exe)
  original_results={}
  for context in (14,15,16):
   result=run([str(original_exe),windows(BASELINES['original_dll'][0]),windows(PRIVATE/f'research-temp/position-neutral-{context}.mem1'),windows(unpacked/'db_teams'),windows(unpacked/'template/00002.bin')],out/f'bootstrap-context{context}.log')
   if 'weighted rows exact: 2010 authentic PLAY/mode cases' not in result or '402 PLAY rows / 4422 isolated query slots' not in result or '56 score cases' not in result:raise RuntimeError('Original bounded bootstrap completion marker missing')
   original_results[str(context)]=result.strip()
  record['controlled_original_bootstrap']=original_results
  record['original_bootstrap_boundary']='Actual registered 8014F2B0 query-engine contribution and ROBP curve comparisons; 80042568/8014FCCC execute across 2010 raw PLAY/mode cases per context with resolved DB getters and allocation supplied. The earlier uncontrolled saved-context 80042568 attempt mismatched and is not promoted to verification of original registration or full menu startup.'
  if baseline_hashes()!=before:raise RuntimeError('Protected original baseline changed')
  for p,expected in hashes.items():
   if digest(Path(p))!=expected:raise RuntimeError(f'Input/source/build changed during verification: {p}')
  record.update(status='passed',results=results,baseline=before,sha256_before_and_after=hashes)
 except Exception as exc:
  record.update(status='failed',error=str(exc));raise
 finally:report.write_text(json.dumps(record,indent=2)+'\n')
 print('Independent original-source team startup passes; report '+str(report))
if __name__=='__main__':main()
