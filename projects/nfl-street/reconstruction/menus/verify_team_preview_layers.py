#!/usr/bin/env python3
"""Repeat team label/roster/actor-routing checks with private provenance."""
from __future__ import annotations
import argparse,json
from pathlib import Path
from verify_controller_checks import PRIVATE,BASELINES,baseline_hashes,digest,private,run,windows

def main():
 parser=argparse.ArgumentParser(description=__doc__)
 parser.add_argument('--output',type=Path,default=PRIVATE/'research-temp/nflstreet-team-screen-20261010/layers')
 args=parser.parse_args();out=private(args.output);out.mkdir(parents=True,exist_ok=True)
 report=out/'verification.json';record={'status':'running','boundary':'Controlled original callback comparisons with supplied DB numeric fields, fonts and callback trampoline. No full screen activation or visible athlete rendering claim.'}
 report.write_text(json.dumps(record,indent=2)+'\n')
 try:
  before=baseline_hashes();root=Path(__file__).resolve().parent
  db=PRIVATE/'private/nflstreet/GNNE69-r0/unpacked/db_teams'
  specs=[('activation','Team activation/rebuild original comparison passes: 6',False),('activation_callback','Team activation callback exact: 24',False),('formation','Team formation original assignment exact: 3',False),('identity','Team identity original name exact: 402',True),('logo','Team logo callback original comparison passes: 32','logo'),('text_update','Team text update original comparison passes: 4', 'text'),('format','Team formatter original exact: 72',False),('labels','Team label original callback exact: 224',True),('preview','Original8011D628 roster projection exact: 64',True),('actor','Team class3 original callback routing exact: 8',False),('image','Team class0 image property7 original exact: 6',False),('player_assets','Team player asset selection exact: 32 actual team DB blocks, 402',True)]
  hashes={str(p):digest(p) for pattern in ('*.c','*.h','*.sh','*.py') for p in root.glob(pattern)};results={}
  unpacked=PRIVATE/'private/nflstreet/GNNE69-r0/unpacked'
  source=PRIVATE/'research-temp/nflstreet-original-menus-20261009/nested/uis_all/00000/00007.bin'
  for path in [source,unpacked/'template/00002.bin',*[unpacked/f'uis_font/{i:05d}.bin' for i in range(4)]]:
   path=private(path);hashes[str(path)]=digest(path)
  for i in range(1,33):
   path=private(db/f'{i:05d}.bin');hashes[str(path)]=digest(path)
  for kind,marker,uses_db in specs:
   run(['bash',str(root/f'build_windows_team_{kind}_checks.sh'),str(out)],out/f'{kind}-build.log')
   name=kind.replace('_','-');exe=private(out/f'team-{name}-check.exe');hashes[str(exe)]=digest(exe);results[kind]={}
   for context in (14,15,16):
    snapshot=private(PRIVATE/f'research-temp/position-neutral-{context}.mem1');hashes[str(snapshot)]=digest(snapshot)
    command=[str(exe),windows(BASELINES['original_dll'][0])]
    if uses_db!='logo':command.append(windows(snapshot))
    if uses_db=='text':
     command.extend([windows(PRIVATE/'research-temp/nflstreet-original-menus-20261009/nested/uis_all/00000/00007.bin'),windows(PRIVATE/'private/nflstreet/GNNE69-r0/unpacked')])
    elif uses_db:command.append(windows(db))
    result=run(command,out/f'{kind}-context{context}.log')
    if marker not in result:raise RuntimeError(f'{kind} completion marker missing')
    results[kind][str(context)]=result.strip()
  for kind,name,marker,extra in [
   ('lineup','team-lineup-original-check.exe','Lineup bounded weighted-row checks passed',[]),
   ('weights','team-weights-original-check.exe','Raw PLAY extraction, GEAR/GRIP table join, supplied-response 8014F504 arithmetic, and 801E0D00 score conversion pass.',[windows(db),windows(unpacked/'template/00002.bin')]),
   ('player_window','team-player-window-check.exe','Team player-window callbacks exact on 3 saved contexts',[]),
   ('player_window_callback','team-player-window-callback-check.exe','Original 8011E9D4 properties8000000F/80000010 framing exact across three saved contexts',[]),
   ('player_window_screen','team-player-window-screen-check.exe','Joined team-screen player-window routing exact',[])]:
   run(['bash',str(root/f'build_windows_team_{kind}_checks.sh'),str(out)],out/f'{kind}-build.log')
   exe=private(out/name);hashes[str(exe)]=digest(exe)
   result=run([str(exe),windows(BASELINES['original_dll'][0]),windows(PRIVATE/'research-temp'),*extra],out/f'{kind}-original.log')
   if marker not in result:raise RuntimeError(f'{kind} completion marker missing')
   results[kind]=result.strip()
  after=baseline_hashes()
  if after!=before:raise RuntimeError('Original baseline changed')
  for path,expected in hashes.items():
   if digest(Path(path))!=expected:raise RuntimeError(f'Input/build changed: {path}')
  record.update(status='passed',results=results,baseline=after,private_hashes=hashes)
 except Exception as exc:
  record.update(status='failed',error=str(exc));raise
 finally:report.write_text(json.dumps(record,indent=2)+'\n')
 print('Team labels, rosters, rating arithmetic, player windows and supplied activation callbacks pass in three contexts; full startup and selected-athlete rendering remain unfinished.')
if __name__=='__main__':main()
