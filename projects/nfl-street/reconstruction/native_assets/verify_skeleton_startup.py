#!/usr/bin/env python3
"""Compare complete skeleton startup and independent selected geometry loading."""
import json
from pathlib import Path
from export_skeleton_startup_catalog import export
from verify_player_startup import source_hashes
from verify_controller_checks import PRIVATE,BASELINES,baseline_hashes,digest,private,run,windows

def main():
 here=Path(__file__).resolve().parent;root=here.parent
 out=private(PRIVATE/'research-temp/nflstreet-skeleton-startup-20261012');out.mkdir(parents=True,exist_ok=True)
 report=out/'verification.json';data=private(PRIVATE/'private/nflstreet/GNNE69-r0')
 entries=tuple('native_assets/'+n for n in (
  'skeleton_startup.c','skeleton_resource.c','skeleton_startup_original_check.c',
  'initialized_geometry_check.c','selected_hair_resources_check.c','selected_player_geometry.c',
  'selected_player_resources.c','headwear_state.c','hair_resources.c','hair_postattach.c',
  'face_resource_pair.c','face_postattach.c','model_postattach.c','body_resource_pool.c',
  'model_view.c','model_relocate.c','model_chunk_relocate.c','archive.c','lzh1.c','actor_model_init.c',
  'build_windows_skeleton_startup_original_check.sh','build_windows_initialized_geometry_check.sh',
  'export_skeleton_startup_catalog.py','verify_skeleton_startup.py'))+tuple('menus/'+n for n in (
  'team_player_body_binding.c','team_player_resolve.c','team_player_manifest.c','team_catalog.c','team_identity.c'))
 record={'status':'running','boundary':'Original80041108 complete six-resource relocation/position/scale/mirroring startup with only801A69C0 archive acquisition supplied. Whole files, references and vector outputs compared. Native selected geometry binds prepared face/hair targets using private NSC1 constants; no DOL/module/snapshot runtime inputs. Textures/materials and fresh actor startup remain open.'}
 try:
  before=baseline_hashes();sources=source_hashes(root,entries)
  catalog=private(data/'native/skeleton-startup.nsc1');catalog_sha=export(catalog)
  archives=[data/'assets'/f'{name}.dat' for name in ('plaface','plarosterhair','plamodel','animdata','plahair')]
  gear=data/'unpacked/template/00002.bin';contexts=[PRIVATE/f'research-temp/position-neutral-{i}.mem1' for i in (14,15,16)]
  inputs={str(p):digest(p) for p in [catalog,*archives,gear,*contexts,*[data/'unpacked/db_teams'/f'{i:05}.bin' for i in range(1,33)]]}
  run(['bash',str(here/'build_windows_skeleton_startup_original_check.sh'),str(out)],out/'original-build.log')
  exe=out/'skeleton-startup-original-check.exe';original_logs={}
  expected='Skeleton startup: 6 acquisitions, 6 complete initialized files, 6 published references, 6 vector words exact; partial-load rejection.'
  for i,context in zip((14,15,16),contexts):
   traces=[]
   for repeat in range(2):
    log=out/f'original-{i}-{repeat}.log'
    text=run([str(exe),windows(BASELINES['original_dll'][0]),windows(context),windows(archives[3]),windows(catalog)],log)
    if text.strip()!=expected:raise RuntimeError('Incomplete original startup comparison')
    traces.append(digest(log))
   if traces[0]!=traces[1]:raise RuntimeError('Fresh original startup traces differ')
   original_logs[str(i)]=traces
  run(['bash',str(here/'build_windows_initialized_geometry_check.sh'),str(out)],out/'native-build.log')
  native_exe=out/'initialized-geometry-check.exe';native_logs=[]
  expected=('Selected native resources: 402 roster rows, 1206 explicit body-variant assemblies, 2412 bound face models, 3 ownership/input guards; fresh target and repeated binding, no snapshot/module input.\n'
   'Automatic hair resources: 402 derived plans, 1206 processed models across 402 roster rows; rendering remains outside scope.\n'
   'Hair binding: 3618 selected models and repeated no-ops; fresh entry101 target.\n'
   'Selected geometry uses initialized face/hair targets from six-resource startup; no DOL/module/snapshot runtime input.')
  for repeat in range(2):
   log=out/f'native-{repeat}.log'
   text=run([str(native_exe),*[windows(p) for p in archives[:4]],windows(data/'unpacked'),windows(archives[4]),windows(gear),windows(catalog)],log)
   if text.replace('\r\n','\n').strip()!=expected:raise RuntimeError('Incomplete initialized geometry coverage')
   native_logs.append(digest(log))
  if native_logs[0]!=native_logs[1]:raise RuntimeError('Fresh native geometry traces differ')
  if before!=baseline_hashes() or any(digest(Path(p))!=v for p,v in inputs.items()):raise RuntimeError('Protected originals or private inputs changed')
  if sources!=source_hashes(root,entries):raise RuntimeError('Source changed during verification')
  catalog_data=catalog.read_bytes();position_offsets=(0x5a,0x5c,0x5e,0x7e,0x80,0x82)
  vectors=[f'{(int.from_bytes(catalog_data[16+offset:18+offset],"big",signed=True)*16)&0xffffffff:08x}' for offset in position_offsets]
  record.update(status='passed',observed_scale_word=catalog_data[-4:].hex(),observed_vector_words=vectors,baseline=before,source_sha256=sources,input_sha256=inputs,catalog_sha256=catalog_sha,
   original_executable_sha256=digest(exe),native_executable_sha256=digest(native_exe),original_log_sha256=original_logs,native_log_sha256=native_logs,
   original_initialized_files=36,original_vector_words=36,original_references=36,native_fresh_processes=2,geometry_assemblies_per_process=1206,bound_face_models_per_process=2412,bound_hair_models_per_process=3618)
 except Exception as exc:record.update(status='failed',error=str(exc));raise
 finally:report.write_text(json.dumps(record,indent=2)+'\n')
 print('Complete skeleton startup and initialized selected geometry pass; private report '+str(report))
if __name__=='__main__':main()
