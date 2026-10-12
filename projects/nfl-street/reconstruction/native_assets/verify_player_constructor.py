#!/usr/bin/env python3
"""Repeat the joined player model/tail constructor against original bodies."""
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
    out=private(PRIVATE/'research-temp/nflstreet-player-constructor-20261012')
    out.mkdir(parents=True,exist_ok=True)
    report=out/'verification.json'
    record={'status':'running','boundary':'Original800E1F7C, model/channel helpers and800D0448/800D1330 versus native constructor. Actual reference resources and constants; remaining input descriptor fields controlled. Appearance callback and allocations supplied. No complete appearance or fresh playable match claim.'}
    report.write_text(json.dumps(record,indent=2)+'\n')
    try:
        before=baseline_hashes()
        inputs=[private(PRIVATE/f'research-temp/position-neutral-{c}.mem1') for c in (14,15,16)]
        hashes={str(p):digest(p) for p in inputs}
        if not args.no_build:
            run(['bash',str(root/'build_windows_player_constructor_original_check.sh'),str(out)],out/'build.log')
        exe=out/'player-constructor-original-check.exe'
        contexts={}
        expected=[(str(mask),str(prefilled),str(mutation)) for mask in range(64) for prefilled in range(2) for mutation in range(2)]
        for context in (14,15,16):
            traces=[]
            for repeat in range(2):
                log=out/f'root-context{context}-{repeat}.log'
                text=run([str(exe),windows(BASELINES['original_dll'][0]),windows(PRIVATE/f'research-temp/position-neutral-{context}.mem1')],log)
                rows=re.findall(r'^constructor mask=(\d+) prefilled=(\d+) mutation=(\d+): full actor1A90 \+\d+ blocks \+appearance45 exact',text,re.M)
                if rows!=expected:raise RuntimeError(f'Incomplete constructor comparison: context{context}')
                traces.append(digest(log))
            if traces[0]!=traces[1]:raise RuntimeError('Fresh constructor traces differ')
            contexts[str(context)]={'cases':256,'fresh_processes':2,'log_sha256':traces}
        if before!=baseline_hashes() or any(digest(Path(p))!=h for p,h in hashes.items()):
            raise RuntimeError('Protected originals or resource contexts changed')
        record.update(status='passed',baseline=before,input_sha256=hashes,contexts=contexts,
                      executable_sha256=digest(exe),source_sha256={name:digest(root/name) for name in
                          ('player_constructor.c','player_constructor.h','actor_tail_init.c','actor_tail_init.h','actor_model_init.c','player_constructor_original_check.c','actor_model_init_original_check.c')})
    except Exception as exc:
        record.update(status='failed',error=str(exc));raise
    finally:report.write_text(json.dumps(record,indent=2)+'\n')
    print('Joined native player constructor comparisons pass; private report '+str(report))


if __name__=='__main__':main()
