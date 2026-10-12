#!/usr/bin/env python3
"""Repeat full native actor-model initialization against original helper bodies."""
import argparse
import json
import re
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent.parent/'menus'))
from verify_controller_checks import PRIVATE, BASELINES, baseline_hashes, digest, private, run, windows


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--no-build',action='store_true')
    args=parser.parse_args()
    root=Path(__file__).resolve().parent
    out=private(PRIVATE/'research-temp/nflstreet-actor-model-init-20261012')
    out.mkdir(parents=True,exist_ok=True)
    report=out/'verification.json'
    record={'status':'running','boundary':'Original800D4E0C and four channel helper bodies versus native owned base-model initialization. Source resource views from saved actors; allocator and zero-fill supplied. Full actor storage and allocated blocks compared; no complete player, appearance, or fresh playable match claim.'}
    report.write_text(json.dumps(record,indent=2)+'\n')
    try:
        before=baseline_hashes()
        inputs=[private(PRIVATE/f'research-temp/position-neutral-{c}.mem1') for c in (14,15,16)]
        hashes={str(p):digest(p) for p in inputs}
        if not args.no_build:
            run(['bash',str(root/'build_windows_actor_model_init_original_check.sh'),str(out)],out/'build.log')
        exe=out/'actor-model-init-original-check.exe'
        contexts={}
        for context in (14,15,16):
            traces=[]
            for repeat in range(2):
                log=out/f'root-context{context}-{repeat}.log'
                text=run([str(exe),windows(BASELINES['original_dll'][0]),windows(PRIVATE/f'research-temp/position-neutral-{context}.mem1')],log)
                rows=re.findall(r'^source-descriptor mask=(\d+): full actor1A90 \+(\d+) allocated blocks exact',text,re.M)
                if [int(row[0]) for row in rows]!=list(range(64)) or rows[-1][1]!='7' or 'Original800D4E0C + original channel helpers match' not in text:
                    raise RuntimeError(f'Incomplete model initialization evidence: context{context}')
                traces.append(digest(log))
            if traces[0]!=traces[1]:raise RuntimeError('Fresh model initialization traces differ')
            contexts[str(context)]={'cases':64,'fresh_processes':2,'log_sha256':traces}
        if before!=baseline_hashes() or any(digest(Path(p))!=h for p,h in hashes.items()):
            raise RuntimeError('Protected originals or resource contexts changed')
        record.update(status='passed',baseline=before,input_sha256=hashes,contexts=contexts,
                      executable_sha256=digest(exe),source_sha256={name:digest(root/name) for name in
                          ('actor_model_init.c','actor_model_init.h','actor_model_init_original_check.c')})
    except Exception as exc:
        record.update(status='failed',error=str(exc));raise
    finally:report.write_text(json.dumps(record,indent=2)+'\n')
    print('Full actor base-model initialization comparisons pass; private report '+str(report))


if __name__=='__main__':main()
