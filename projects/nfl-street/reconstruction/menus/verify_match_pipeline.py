#!/usr/bin/env python3
"""Compare the joined native startup/session/configuration with original bodies."""
import argparse
import json
import re
from pathlib import Path
from verify_controller_checks import PRIVATE, BASELINES, baseline_hashes, digest, private, run, windows


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--no-build',action='store_true')
    args=parser.parse_args()
    root=Path(__file__).resolve().parent
    out=private(PRIVATE/'research-temp/nflstreet-match-pipeline-20261012')
    out.mkdir(parents=True,exist_ok=True)
    report=out/'verification.json'
    record={'status':'running','boundary':'Original801EA480→801E577C→801E51D4 bodies against connected native helpers. Actual Win32 formation input and private PLAY resolution; configuration query outputs, callbacks, table/resource/actor stages supplied. No fresh playable match or original query-engine ownership claim.'}
    report.write_text(json.dumps(record,indent=2)+'\n')
    try:
        before=baseline_hashes()
        selection=private(PRIVATE/'research-temp/nflstreet-team-field-picker-join-20261011/field-picker-window-selection.json')
        database=private(PRIVATE/'private/nflstreet/GNNE69-r0/unpacked/db_teams/00001.bin')
        inputs=[selection,database]+[private(PRIVATE/f'research-temp/position-neutral-{c}.mem1') for c in (14,15,16)]
        hashes={str(p):digest(p) for p in inputs}
        if not args.no_build:
            run(['bash',str(root/'build_windows_match_pipeline_original_check.sh'),str(out)],out/'build.log')
        exe=out/'match-pipeline-original-check.exe'
        contexts={}
        for context in (14,15,16):
            traces=[]
            for repeat in range(2):
                log=out/f'root-context{context}-{repeat}.log'
                text=run([str(exe),windows(BASELINES['original_dll'][0]),windows(PRIVATE/f'research-temp/position-neutral-{context}.mem1'),windows(selection),windows(database)],log)
                rows=re.findall(r'^([^:\n]+): pipeline exact status=([0-9a-f]+) continuation=(\d+) initialized=(\d+) trace=(\d+) settings=(\d+) insert-float=([^\s]+)',text,re.M)
                by_name={row[0]:row[1:] for row in rows}
                expected={'actual-selection-success':('00000000','1','1','55','5','10'),
                          'pipeline-error-80266f44':('00000071','0','1','39','1','0'),
                          'pipeline-error-802671a0':('00000074','0','1','43','5','10'),
                          'pipeline-cancel-update':('00000000','0','1','45','5','10')}
                if len(rows)!=9 or any(by_name.get(name)!=values for name,values in expected.items()) or 'Joined original801EA480-801E577C-801E51D4 differential passed' not in text:
                    raise RuntimeError(f'Incomplete joined pipeline evidence: context{context}')
                traces.append(digest(log))
            if traces[0]!=traces[1]:raise RuntimeError('Fresh pipeline traces differ')
            contexts[str(context)]={'cases':9,'fresh_processes':2,'log_sha256':traces}
        if before!=baseline_hashes() or any(digest(Path(p))!=h for p,h in hashes.items()):
            raise RuntimeError('Protected originals or supplied inputs changed')
        record.update(status='passed',baseline=before,input_sha256=hashes,contexts=contexts,
                      executable_sha256=digest(exe),source_sha256={name:digest(root/name) for name in
                      ('match_pipeline.c','match_pipeline.h','match_settings.c','match_session.c','match_start.c','match_player_query.c','match_pipeline_original_check.c','match_session_original_check.c')})
    except Exception as exc:
        record.update(status='failed',error=str(exc));raise
    finally:report.write_text(json.dumps(record,indent=2)+'\n')
    print('Joined native startup/session/configuration comparisons pass; private report '+str(report))


if __name__=='__main__':main()
