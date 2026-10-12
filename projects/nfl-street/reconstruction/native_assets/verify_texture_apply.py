#!/usr/bin/env python3
"""Verify selected face texture ownership and actual original lookup/cache/MMAP behavior."""
import json
from pathlib import Path
from verify_player_startup import source_hashes
from verify_controller_checks import PRIVATE,BASELINES,baseline_hashes,digest,private,run,windows

def main():
 here=Path(__file__).resolve().parent;root=here.parent
 out=private(PRIVATE/'research-temp/nflstreet-texture-apply-20261012');out.mkdir(parents=True,exist_ok=True)
 data=private(PRIVATE/'private/nflstreet/GNNE69-r0');report=out/'verification.json'
 entries=tuple('native_assets/'+name for name in (
  'texture_apply.c','texture_apply_check.c','texture_apply_original_check.c','applied_appearance_assets_check.c','face_texture.c','face_texture_check.c','face_texture_original_check.c','texture.c',
  'selected_appearance_assets.c','selected_appearance_assets_check.c',
  'skeleton_startup.c','skeleton_resource.c','selected_player_geometry.c','selected_player_resources.c',
  'headwear_state.c','hair_resources.c','hair_postattach.c','face_resource_pair.c','face_postattach.c',
  'model_postattach.c','body_resource_pool.c','model_view.c','model_relocate.c','model_chunk_relocate.c',
  'archive.c','lzh1.c','actor_model_init.c','build_windows_texture_apply_check.sh',
  'build_windows_texture_apply_original_check.sh','build_windows_applied_appearance_assets_check.sh','verify_texture_apply.py'))+tuple('menus/'+name for name in (
  'team_player_resolve.c','team_player_manifest.c','team_catalog.c','team_identity.c','team_player_body_binding.c'))
 record={'status':'running','boundary':'Original80164CE4 image/palette application into independently loaded templates; complete destination files, repeated application and ordered flush addresses/sizes compared. Original formatting/HSH1/cache/relocation/application/memcpy execute; synchronization and cache-flush effects supplied. Owned plane0 pixels are inspection output, not original material/palette selection or GPU behavior. Joined geometry/face-texture native ownership uses prepared skeleton targets.'}
 try:
  before=baseline_hashes();sources=source_hashes(root,entries)
  archives=[data/'assets'/f'{n}.dat' for n in ('plaface','plarosterhair','platex','plamodel','plahair','animdata')]
  contexts=[PRIVATE/f'research-temp/position-neutral-{i}.mem1' for i in (14,15,16)]
  gear=data/'unpacked/template/00002.bin';catalog=data/'native/skeleton-startup.nsc1'
  inputs={str(p):digest(p) for p in [*archives,*contexts,gear,catalog,*[data/'unpacked/db_teams'/f'{i:05}.bin' for i in range(1,33)]]}
  for name in ('texture_apply','texture_apply_original','applied_appearance_assets'):
   run(['bash',str(here/f'build_windows_{name}_check.sh'),str(out)],out/f'{name}-build.log')
  plain=out/'texture-apply-check.exe';original=out/'texture-apply-original-check.exe';joined=out/'applied-appearance-assets-check.exe'
  expected='Selected face textures: 402 roster rows, 15 template fallbacks, 13172736 inspection pixels; owned raw/processed MMAP, no original draw claim.'
  prefix=[*[windows(p) for p in archives[:3]],windows(data/'unpacked')]
  native_logs=[];joined_logs=[];original_logs={}
  for repeat in range(2):
   log=out/f'native-{repeat}.log';text=run([str(plain),*prefix],log)
   if text.replace('\r\n','\n').strip()!=expected+'\nFace texture application: 402 whole destination files/reapplies, 2412 ordered flush requests; original GPU effects excluded.':raise RuntimeError('Incomplete native face texture coverage')
   native_logs.append(digest(log))
   log=out/f'joined-{repeat}.log';text=run([str(joined),*prefix,*[windows(p) for p in archives[3:]],windows(gear),windows(catalog)],log)
   if text.replace('\r\n','\n').strip()!=expected+'\nJoined selected assets: 1206 geometry/face-texture assemblies, 3 texture namespace guards; prepared skeleton binding and request writes, no runtime DOL/module/snapshot.\nJoined face application: 1206 owned destinations, 3618 ordered flush requests, 2 replacement guards.':raise RuntimeError('Incomplete joined appearance asset coverage')
   joined_logs.append(digest(log))
  if native_logs[0]!=native_logs[1] or joined_logs[0]!=joined_logs[1]:raise RuntimeError('Fresh native asset traces differ')
  for i,context in zip((14,15,16),contexts):
   traces=[]
   for repeat in range(2):
    log=out/f'original-{i}-{repeat}.log';text=run([str(original),*prefix,windows(BASELINES['original_dll'][0]),windows(context)],log)
    if text.replace('\r\n','\n').strip()!=expected+'\nOriginal face texture checks: 1208 selections/cache cases, 388 whole MMAP relocations/repeats; actual HSH1 lookup, synchronization services supplied.\nFace texture application: 402 whole destination files/reapplies, 2412 ordered flush requests; original GPU effects excluded.':raise RuntimeError('Incomplete original texture comparison')
    traces.append(digest(log))
   if traces[0]!=traces[1]:raise RuntimeError('Fresh original texture traces differ')
   original_logs[str(i)]=traces
  if before!=baseline_hashes() or any(digest(Path(p))!=sha for p,sha in inputs.items()):raise RuntimeError('Protected originals or private inputs changed')
  if sources!=source_hashes(root,entries):raise RuntimeError('Source changed during verification')
  record.update(status='passed',baseline=before,input_sha256=inputs,source_sha256=sources,native_log_sha256=native_logs,joined_log_sha256=joined_logs,original_log_sha256=original_logs,
   executable_sha256={p.name:digest(p) for p in (plain,original,joined)},roster_rows=402,template_fallbacks=15,inspection_pixels=13172736,original_cache_cases_per_process=1208,
   original_mmap_files_per_process=388,joined_assemblies_per_process=1206,namespace_guards=3,application_cases_per_process=402,application_flushes_per_process=2412,joined_applied_destinations_per_process=1206,joined_flushes_per_process=3618,replacement_guards=2)
 except Exception as exc:record.update(status='failed',error=str(exc));raise
 finally:report.write_text(json.dumps(record,indent=2)+'\n')
 print('Original whole-face texture application and joined applied ownership pass; private report '+str(report))
if __name__=='__main__':main()
