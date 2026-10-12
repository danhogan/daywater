#!/usr/bin/env python3
"""Verify the native source team window and its separate athlete inspection pane."""
import json,re,sys
from pathlib import Path
from verify_controller_checks import PRIVATE,baseline_hashes,digest,private,run,windows
from verify_team_visible_input import PASSIVE_SHA256

def main():
 root=Path(__file__).resolve().parent;out=private(PRIVATE/'research-temp/nflstreet-team-live-20261011');out.mkdir(parents=True,exist_ok=True)
 report=out/'verification.json';record={'status':'running','boundary':'Fresh native source UI and Windows paints with separate selected head/body inspection panes. Resource joins are checked; combined stored pose is candidate, variant 0 is supplied. Hair/material/attachment and full navigation remain unfinished.'};report.write_text(json.dumps(record,indent=2)+'\n')
 try:
  before=baseline_hashes();unpacked=private(PRIVATE/'private/nflstreet/GNNE69-r0/unpacked');source=private(PRIVATE/'research-temp/nflstreet-original-menus-20261009/nested/uis_all/00000/00007.bin')
  paths=[source,unpacked/'template/00002.bin',unpacked.parent/'assets/plaface.dat',unpacked.parent/'assets/plarosterhair.dat',unpacked.parent/'assets/plamodel.dat',*[unpacked/f'db_teams/{i:05}.bin' for i in range(1,33)],*[unpacked/f'uis_font/{i:05}.bin' for i in range(4)]]
  # Hash the actual build dependencies, not unrelated research harnesses.
  build=root/'build_windows_team_screen_preview.sh'
  sources=re.search(r'sources=\(([^)]+)\)',build.read_text()).group(1).split()
  paths += [build,root/'team_screen_preview.c',root/'team_screen_probe.c',Path(__file__)]
  for name in sources:paths += [root/(name+'.c'),root/(name+'.h')]
  for name in ('model.c','model.h','mesh_render.c','mesh_render.h','texture.c','texture.h','archive.c','archive.h','lzh1.c','lzh1.h'):paths.append(root.parent/'native_assets'/name)
  for folder in ('uis_comn','uis_butt','uis_tmlo','uis_bgmp','uis_bgpl','uis_ctrl','uis_fron','uis_fsel','uis_port','uis_gabr','uis_ingm','uis_crtm','uis_chal','uis_movi'):paths += list((unpacked/folder).glob('*.bin'))
  hashes={str(p):digest(p) for p in paths}
  run(['bash',str(root/'build_windows_team_screen_preview.sh'),str(out)],out/'build.log');exe=private(out/'team-screen-preview.exe');hashes[str(exe)]=digest(exe)
  cases={}
  for mode,pane in (('startup',None),('roster','head'),('confirmation','head'),('roster','player'),('confirmation','player')):
   repeats=[]
   for iteration in range(2):
    case=mode+('-'+pane if pane else '')
    prefix=private(out/(case+'-window'+('-repeat' if iteration else '')))
    cmd=[str(exe),windows(source),windows(unpacked),windows(PRIVATE/'research-temp/nflstreet-native-windows-local/game/sys/main.dol'),'1000','--capture',windows(prefix),'--live-check']
    if mode!='startup':cmd += ['--inspect-'+mode]
    if pane:cmd += ['--'+pane+'-inspection']
    text=run(cmd,out/(prefix.name+'.log'))
    paints=re.search(r'Native team window: 60 source updates; (\d+) successful paints;',text)
    if not paints or int(paints.group(1))<1:raise RuntimeError('Window lifecycle check failed')
    image=private(prefix.with_suffix('.bmp'));value=digest(image);hashes[str(image)]=value;values={'source_pixels':value}
    if mode=='startup' and value!=PASSIVE_SHA256:raise RuntimeError('Passive menu pixels changed')
    if pane:
     if 'Separate selected-'+pane+' inspection:' not in text:raise RuntimeError('Selected inspection owner missing')
     image=private(Path(str(prefix)+'-selected-'+pane+'.bmp'));value=digest(image);hashes[str(image)]=value;values['selected_'+pane+'_pixels']=value
     if len(re.findall(r'Source preview slot\d+ PLAY\d+ head index',text))>25:raise RuntimeError('Consumed factory descriptions replayed during passive updates')
    repeats.append(values)
   if repeats[0]!=repeats[1]:raise RuntimeError('Independent repeated captures differ: '+mode)
   cases[case]=repeats[0]
  run(['bash',str(root/'build_windows_team_effects_checks.sh'),str(out)],out/'effects-build.log')
  effect_exe=private(out/'team-effects-check.exe');hashes[str(effect_exe)]=digest(effect_exe)
  run([str(effect_exe)],out/'effects.log')
  if baseline_hashes()!=before:raise RuntimeError('Protected baseline changed')
  for name,value in hashes.items():
   if digest(Path(name))!=value:raise RuntimeError('Tracked source/input changed: '+name)
  record.update(status='passed',cases=cases,baseline=before,sha256_before_and_after=hashes)
 except Exception as exc:record.update(status='failed',error=str(exc));raise
 finally:report.write_text(json.dumps(record,indent=2)+'\n')
 print('Native source team window and selected athlete pane checks pass; report '+str(report))
if __name__=='__main__':main()
