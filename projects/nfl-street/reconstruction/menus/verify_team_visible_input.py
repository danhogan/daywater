#!/usr/bin/env python3
"""Capture original entry7 artwork after bounded native roster source input."""
from __future__ import annotations
import argparse,json,sys
from pathlib import Path
from verify_controller_checks import PRIVATE,BASELINES,baseline_hashes,digest,private,run,windows

PASSIVE_SHA256='7f4e482da300c397c40ccda7780d0d60845a20ac8d3b6fe5a99e2ecd7e9ee78a'

def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--output',type=Path,default=PRIVATE/'research-temp/nflstreet-team-visible-input-20261011')
 args=p.parse_args();out=private(args.output);out.mkdir(parents=True,exist_ok=True)
 report=out/'verification.json'
 record={'status':'running','confidence':'verified native repeatability and preserved passive capture; candidate original pixels',
  'boundary':'Complete original entry7 source runs independently using supplied logical device records and fresh private files. Artwork/fonts are original; the affine rasterizer is diagnostic. Captures do not prove original GX pixels, hardware input, selected-athlete 3D appearance or full menu-to-match startup.'}
 report.write_text(json.dumps(record,indent=2)+'\n')
 try:
  before=baseline_hashes();root=Path(__file__).resolve().parent
  source=private(PRIVATE/'research-temp/nflstreet-original-menus-20261009/nested/uis_all/00000/00007.bin')
  unpacked=private(PRIVATE/'private/nflstreet/GNNE69-r0/unpacked')
  hashes={str(path):digest(path) for pattern in ('*.c','*.h','*.sh','*.py') for path in root.glob(pattern)}
  paths=[source,BASELINES['dol'][0],unpacked/'template/00002.bin',*[unpacked/f'db_teams/{i:05}.bin' for i in range(1,33)],*[unpacked/f'uis_font/{i:05}.bin' for i in range(4)]]
  # Hash each archive supported by the diagnostic source renderer.
  for folder in ('uis_comn','uis_butt','uis_tmlo','uis_bgmp','uis_bgpl','uis_ctrl','uis_fron','uis_fsel','uis_port','uis_gabr','uis_ingm','uis_crtm','uis_chal','uis_movi'):
   paths += list((unpacked/folder).glob('*.bin'))
  hashes.update({str(path):digest(path) for path in paths})
  run(['bash',str(root/'build_windows_team_screen_preview.sh'),str(out)],out/'build.log')
  exe=private(out/'team-screen-preview.exe');hashes[str(exe)]=digest(exe)
  run(['bash',str(root/'build_windows_team_player_manifest_checks.sh'),str(out)],out/'manifest-build.log')
  manifest_exe=private(out/'team-player-manifest-check.exe');hashes[str(manifest_exe)]=digest(manifest_exe)
  manifest_text=run([str(manifest_exe)],out/'manifest-check.log')
  if 'Selected PLAY manifest synthetic checks passed:' not in manifest_text:raise RuntimeError('Manifest fixture gate marker missing')
  record['manifest_check']={'scope':'Native synthetic fixture contract; injected lookup responses, not joined original resource loading','log':str(out/'manifest-check.log')}
  captures={}
  for mode,rows,spawns in (('startup',None,14),('roster','6/7',16),('confirmation','7/7',25)):
   repeats=[]
   for iteration in range(2):
    prefix=private(out/(mode if iteration==0 else mode+'-repeat'))
    command=[str(exe),windows(source),windows(unpacked),windows(BASELINES['dol'][0]),'1000']
    if mode!='startup':command += ['--inspect-'+mode]
    command += ['--capture',windows(prefix)]
    text=run(command,out/(prefix.name+'.log'))
    if '2D team-menu startup capture:' not in text or f'spawn requests{spawns}.' not in text:raise RuntimeError(mode+' capture marker missing')
    if rows and f'selected rows{rows}; preview requests{spawns};' not in text:raise RuntimeError(mode+' roster marker missing')
    bmp=private(prefix.with_suffix('.bmp'));value=digest(bmp);hashes[str(bmp)]=value;repeats.append(value)
   if repeats[0]!=repeats[1]:raise RuntimeError(mode+' independent captures differ')
   if mode=='startup' and repeats[0]!=PASSIVE_SHA256:raise RuntimeError('Passive screenshot changed from the preserved startup milestone')
   captures[mode]={'path':str(out/(mode+'.bmp')),'sha256':repeats[0],'selected_rows':rows,'preview_requests':spawns}
  # Recheck joined source input plus all controlled original components.
  input_out=private(out/'input')
  text=run([sys.executable,str(root/'verify_team_screen_input.py'),'--output',str(input_out)],out/'input-verification.log')
  if 'Bounded native team source input passes;' not in text:raise RuntimeError('Joined input gate marker missing')
  input_report=private(input_out/'verification.json')
  if json.loads(input_report.read_text())['status']!='passed':raise RuntimeError('Joined input report failed')
  hashes[str(input_report)]=digest(input_report)
  if baseline_hashes()!=before:raise RuntimeError('Protected original baseline changed')
  for path,expected in hashes.items():
   if digest(Path(path))!=expected:raise RuntimeError('Input/source/build changed during verification: '+path)
  record.update(status='passed',captures=captures,controlled_original_report=str(input_report),baseline=before,sha256_before_and_after=hashes)
 except Exception as exc:
  record.update(status='failed',error=str(exc));raise
 finally:report.write_text(json.dumps(record,indent=2)+'\n')
 print('Native team roster/confirmation captures pass; report '+str(report))
if __name__=='__main__':main()
