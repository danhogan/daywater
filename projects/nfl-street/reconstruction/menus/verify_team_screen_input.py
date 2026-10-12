#!/usr/bin/env python3
"""Repeat bounded team input checks with private artifacts and original provenance."""
from __future__ import annotations
import argparse,json
from pathlib import Path
from verify_controller_checks import PRIVATE,BASELINES,baseline_hashes,digest,private,run,windows

def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--output',type=Path,default=PRIVATE/'research-temp/nflstreet-team-input-20261011')
 args=p.parse_args();out=private(args.output);out.mkdir(parents=True,exist_ok=True)
 report=out/'verification.json'
 record={'status':'running','boundary':'Native complete entry7 source input with supplied logical devices and owned transition services. Controlled original helper comparisons do not establish full original menu input, hardware discovery, athlete rendering or field recipient activation.'}
 report.write_text(json.dumps(record,indent=2)+'\n')
 try:
  before=baseline_hashes();root=Path(__file__).resolve().parent
  source=private(PRIVATE/'research-temp/nflstreet-original-menus-20261009/nested/uis_all/00000/00007.bin')
  title_source=private(source.parent/'00003.bin')
  unpacked=private(PRIVATE/'private/nflstreet/GNNE69-r0/unpacked')
  paths=[source,title_source,BASELINES['dol'][0],unpacked/'template/00002.bin',*[unpacked/f'db_teams/{i:05}.bin' for i in range(1,33)],*[unpacked/f'uis_font/{i:05}.bin' for i in range(4)],*[PRIVATE/f'research-temp/position-neutral-{c}.mem1' for c in (14,15,16)]]
  hashes={str(path):digest(path) for path in paths}
  hashes.update({str(path):digest(path) for pattern in ('*.c','*.h','*.sh','*.py') for path in root.glob(pattern)})
  stages=(('devices','507 supported supplied iterator/mapping cases'),('confirm','confirmation gate exact: 64'),('transition','exact for 1000 caller-owned object cases'))
  results={}
  run(['bash',str(root/'build_windows_script_checks.sh'),str(out)],out/'script-build.log')
  script_exe=private(out/'script-original-check.exe');hashes[str(script_exe)]=digest(script_exe)
  script_results={}
  for context in (14,15,16):
   text=run([str(script_exe),windows(BASELINES['original_dll'][0]),windows(PRIVATE/f'research-temp/position-neutral-{context}.mem1'),windows(title_source)],out/f'script-context{context}.log')
   if '12 opcode 6E cases' not in text:raise RuntimeError('Original array-address opcode marker missing')
   script_results[str(context)]=text.strip()
  results['script']=script_results
  run(['bash',str(root/'build_windows_team_player_assets_checks.sh'),str(out)],out/'player-assets-build.log')
  assets_exe=private(out/'team-player-assets-check.exe');hashes[str(assets_exe)]=digest(assets_exe)
  assets_results={}
  for context in (14,15,16):
   text=run([str(assets_exe),windows(BASELINES['original_dll'][0]),windows(PRIVATE/f'research-temp/position-neutral-{context}.mem1'),windows(unpacked/'db_teams')],out/f'player-assets-context{context}.log')
   if '402 PLAY rows across original8014EAC4' not in text:raise RuntimeError('Original portrait selector marker missing')
   assets_results[str(context)]=text.strip()
  results['portrait_selector']=assets_results
  for stage,marker in stages:
   run(['bash',str(root/f'build_windows_team_{stage}_checks.sh'),str(out)],out/f'{stage}-build.log')
   exe=private(out/f'team-{stage}-original-check.exe');hashes[str(exe)]=digest(exe)
   checks={}
   for context in (14,15,16):
    text=run([str(exe),windows(BASELINES['original_dll'][0]),windows(PRIVATE/f'research-temp/position-neutral-{context}.mem1')],out/f'{stage}-context{context}.log')
    if marker not in text:raise RuntimeError(f'{stage} completion marker missing')
    checks[str(context)]=text.strip()
   results[stage]=checks
  run(['bash',str(root/'build_windows_team_input_checks.sh'),str(out)],out/'input-build.log')
  exe=private(out/'team-input-probe.exe');hashes[str(exe)]=digest(exe)
  text=run([str(exe),windows(source),windows(unpacked),windows(BASELINES['dol'][0]),'1000'],out/'native-input.log')
  if 'Team source input checks pass:' not in text:raise RuntimeError('Native source input completion marker missing')
  record['native_source_input']=text.strip()
  # Retain the existing no-device passive startup regression separately.
  run(['bash',str(root/'build_windows_team_screen_checks.sh'),str(out)],out/'passive-build.log')
  exe=private(out/'team-screen-probe.exe');hashes[str(exe)]=digest(exe)
  text=run([str(exe),windows(source),windows(unpacked),windows(BASELINES['dol'][0]),'1000'],out/'native-passive.log')
  if 'creation/activation and1000 updates pass' not in text or 'lineup sizes7/7, spawn requests14' not in text:raise RuntimeError('Passive startup regression marker missing')
  record['native_passive']=text.strip()
  if baseline_hashes()!=before:raise RuntimeError('Protected original baseline changed')
  for path,expected in hashes.items():
   if digest(Path(path))!=expected:raise RuntimeError('Input/source/build changed during verification: '+path)
  record.update(status='passed',controlled_original=results,baseline=before,sha256_before_and_after=hashes)
 except Exception as exc:
  record.update(status='failed',error=str(exc));raise
 finally:report.write_text(json.dumps(record,indent=2)+'\n')
 print('Bounded native team source input passes; report '+str(report))
if __name__=='__main__':main()
