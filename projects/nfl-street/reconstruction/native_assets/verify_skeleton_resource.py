#!/usr/bin/env python3
"""Verify fresh SKL1 resource relocation against the original helper."""
import json
from pathlib import Path

from verify_player_startup import source_hashes
from verify_controller_checks import PRIVATE, BASELINES, baseline_hashes, digest, private, run, windows


def main() -> None:
    here = Path(__file__).resolve().parent
    root = here.parent
    out = private(PRIVATE / 'research-temp/nflstreet-skeleton-resource-20261012')
    out.mkdir(parents=True, exist_ok=True)
    report = out / 'verification.json'
    record = {'status': 'running', 'boundary': 'Original8017EB24 and tree callback on six actual decoded animdata SKL1 entries; whole resources and repeat calls. Additional80041108 game edits are not applied.'}
    report.write_text(json.dumps(record, indent=2) + '\n')
    entries = ('native_assets/skeleton_resource_original_check.c', 'native_assets/skeleton_resource.c',
               'native_assets/build_windows_skeleton_resource_original_check.sh',
               'native_assets/archive.c', 'native_assets/lzh1.c', 'native_assets/actor_model_init.c')
    try:
        before = baseline_hashes()
        sources = source_hashes(root, entries)
        archive = private(PRIVATE / 'private/nflstreet/GNNE69-r0/assets/animdata.dat')
        contexts = [private(PRIVATE / f'research-temp/position-neutral-{i}.mem1')
                    for i in (14, 15, 16)]
        inputs = {str(p): digest(p) for p in (archive, *contexts)}
        run(['bash', str(here / 'build_windows_skeleton_resource_original_check.sh'), str(out)],
            out / 'build.log')
        exe = out / 'skeleton-resource-original-check.exe'
        expected = ''.join(f'SKL1 entry{i}: whole relocated resource and repeated no-op exact\n' for i in (95,96,97,99,100,101))
        results = {}
        for index, context in zip((14, 15, 16), contexts):
            logs = []
            for repeat in range(2):
                log = out / f'context{index}-{repeat}.log'
                output = run([str(exe), windows(BASELINES['original_dll'][0]),
                              windows(context), windows(archive)], log)
                if output.replace('\r\n', '\n') != expected:
                    raise RuntimeError(f'Incomplete skeleton comparison in context {index}')
                logs.append(digest(log))
            if logs[0] != logs[1]:
                raise RuntimeError(f'Fresh skeleton traces differ in context {index}')
            results[str(index)] = {'fresh_processes': 2, 'resource_indices': [95,96,97,99,100,101],
                                   'whole_resources': 6, 'repeat_calls': 6, 'log_sha256': logs}
        if before != baseline_hashes() or any(digest(Path(p)) != value for p, value in inputs.items()):
            raise RuntimeError('Protected baseline or archive input changed')
        if sources != source_hashes(root, entries):
            raise RuntimeError('Checker source changed during verification')
        record.update(status='passed', baseline=before, input_sha256=inputs,
                      source_sha256=sources, executable_sha256=digest(exe), contexts=results)
    except Exception as exc:
        record.update(status='failed', error=str(exc))
        raise
    finally:
        report.write_text(json.dumps(record, indent=2) + '\n')
    print('Fresh SKL1 resource checks pass; private report ' + str(report))


if __name__ == '__main__':
    main()
