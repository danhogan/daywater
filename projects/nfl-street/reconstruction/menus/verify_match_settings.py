#!/usr/bin/env python3
"""Build/repeat original configuration normalization comparisons privately."""
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
    out=private(PRIVATE/'research-temp/nflstreet-match-settings-20261012')
    out.mkdir(parents=True,exist_ok=True)
    report=out/'verification.json'
    record={'status':'running','boundary':'Original801E51D4 query ordering, normalized insert values, floating argument and output contexts compared with native configuration. Query outputs and insertion status supplied; no query-engine or actor creation claim.'}
    report.write_text(json.dumps(record,indent=2)+'\n')
    try:
        before=baseline_hashes()
        inputs=[private(PRIVATE/f'research-temp/position-neutral-{c}.mem1') for c in (14,15,16)]
        hashes={str(p):digest(p) for p in inputs}
        if not args.no_build:
            run(['bash',str(root/'build_windows_match_settings_original_check.sh'),str(out)],out/'build.log')
        exe=out/'match-settings-original-check.exe'
        traces=[]
        for repeat in range(2):
            log=out/f'root-{repeat}.log'
            text=run([str(exe),windows(BASELINES['original_dll'][0]),*[windows(p) for p in inputs]],log)
            success=re.findall(r'^context(?:14|15|16) mode [012]: untouched 801E51D4 queries=',text,re.M)
            errors=re.findall(r'^error query [0-9a-f]+: original/native status=',text,re.M)
            if len(success)!=9 or len(errors)!=8 or 'Original 801E51D4 settings differential passed' not in text:
                raise RuntimeError('Settings differential lacks required modes/error branches')
            traces.append(digest(log))
        if traces[0]!=traces[1] or before!=baseline_hashes() or any(digest(Path(p))!=h for p,h in hashes.items()):
            raise RuntimeError('Trace repetition or protected input checks failed')
        record.update(status='passed',baseline=before,input_sha256=hashes,
                      executable_sha256=digest(exe),repeated_log_sha256=traces,
                      cases_per_process=17,fresh_processes=2,
                      source_sha256={name:digest(root/name) for name in ('match_settings.c','match_settings.h','match_settings_original_check.c')})
    except Exception as exc:
        record.update(status='failed',error=str(exc));raise
    finally:
        report.write_text(json.dumps(record,indent=2)+'\n')
    print('Native configuration original comparisons pass; private report '+str(report))


if __name__=='__main__':main()
