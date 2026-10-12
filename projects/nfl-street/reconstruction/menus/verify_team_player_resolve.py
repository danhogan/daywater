#!/usr/bin/env python3
"""Check fresh native player resolution against the original lookup oracle."""
from pathlib import Path
import json,sys
from verify_controller_checks import PRIVATE,baseline_hashes,digest,private,run,windows

def main():
 root=Path(__file__).resolve().parent;out=private(PRIVATE/'research-temp/nflstreet-team-resolve-20261011');out.mkdir(parents=True,exist_ok=True)
 report=out/'verification.json';record={'status':'running','boundary':'All regular-team PLAY rows and head TMdl payloads; original bounded HSH1 lookup oracle. Hair choice, body assembly, original pixels and full athlete appearance remain unresolved.'}
 report.write_text(json.dumps(record,indent=2)+'\n')
 try:
  before=baseline_hashes();assets=private(PRIVATE/'private/nflstreet/GNNE69-r0/assets');unpacked=private(assets.parent/'unpacked')
  paths=[assets/'plaface.dat',assets/'plarosterhair.dat',*[unpacked/f'db_teams/{i:05}.bin' for i in range(1,33)]]
  paths += [root/name for name in ('team_player_resolve.c','team_player_resolve.h','team_player_resolve_check.c','team_player_manifest.c','team_player_manifest.h','team_catalog.c','team_identity.c','build_windows_team_player_resolve_checks.sh','verify_team_player_resolve.py')]
  paths += [root.parent/'native_assets'/name for name in ('archive.c','archive.h','lzh1.c','lzh1.h')]
  hashes={str(p):digest(p) for p in paths}
  text=run([sys.executable,str(root/'verify_team_player_resources.py')],out/'original-lookup.log')
  original=private(PRIVATE/'research-temp/nflstreet-team-player-loader/original-lookup-verification.json')
  if json.loads(original.read_text())['status']!='passed':raise RuntimeError('Original lookup gate failed')
  queries=private(original.parent/'original-lookup-queries.tsv');hashes[str(queries)]=digest(queries);hashes[str(original)]=digest(original)
  run(['bash',str(root/'build_windows_team_player_resolve_checks.sh'),str(out)],out/'build.log')
  exe=private(out/'team-player-resolve-check.exe');hashes[str(exe)]=digest(exe)
  text=run([str(exe),windows(assets/'plaface.dat'),windows(assets/'plarosterhair.dat'),windows(unpacked),windows(queries)],out/'native.log')
  if '402 actual PLAY rows, 402 custom head TMdl payloads, 2416 original-oracle query results' not in text:raise RuntimeError('Native actual-data marker missing')
  run(['bash',str(root/'build_windows_team_player_manifest_checks.sh'),str(out)],out/'manifest-build.log')
  manifest=private(out/'team-player-manifest-check.exe');hashes[str(manifest)]=digest(manifest)
  text=run([str(manifest)],out/'manifest.log')
  if 'Selected PLAY manifest synthetic checks passed:' not in text:raise RuntimeError('Manifest guards failed')
  if baseline_hashes()!=before:raise RuntimeError('Protected baselines changed')
  for path,value in hashes.items():
   if digest(Path(path))!=value:raise RuntimeError('Tracked input changed: '+path)
  record.update(status='passed',actual_play_rows=402,decoded_head_payloads=402,original_lookup_queries=2416,original_report=str(original),baseline=before,sha256_before_and_after=hashes)
 except Exception as exc:record.update(status='failed',error=str(exc));raise
 finally:report.write_text(json.dumps(record,indent=2)+'\n')
 print('Fresh native selected-player resolution verified; report '+str(report))
if __name__=='__main__':main()
