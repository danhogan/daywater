#!/usr/bin/env python3
"""Compare native postattach binding with original 801C6B38 on private contexts."""
import json
from pathlib import Path

from verify_player_startup import source_hashes
from verify_controller_checks import PRIVATE, BASELINES, baseline_hashes, digest, private, run, windows


def main() -> None:
    here = Path(__file__).resolve().parent
    root = here.parent
    out = private(PRIVATE / 'research-temp/nflstreet-model-postattach-20261012')
    out.mkdir(parents=True, exist_ok=True)
    report = out / 'verification.json'
    record = {'status': 'running', 'boundary': 'Original 801C6B38 on relocated, '
              'attached plaface.dat entries 12/13 and freshly decoded animdata95 SKL1 target. '
              'The source face resources are acquired by the native verified pair.'}
    report.write_text(json.dumps(record, indent=2) + '\n')
    entries = ('native_assets/model_postattach.h', 'native_assets/model_postattach.c',
               'native_assets/model_postattach_original_check.c',
               'native_assets/build_windows_model_postattach_original_check.sh',
               'native_assets/face_resource_pair.c', 'native_assets/face_resource_pair.h',
               'native_assets/model_relocate.c', 'native_assets/model_chunk_relocate.c',
               'native_assets/model_view.c', 'native_assets/archive.c',
               'native_assets/lzh1.c', 'native_assets/skeleton_resource.c')
    try:
        before = baseline_hashes()
        sources = source_hashes(root, entries)
        archive = private(PRIVATE / 'private/nflstreet/GNNE69-r0/assets/plaface.dat')
        target_archive = private(PRIVATE / 'private/nflstreet/GNNE69-r0/assets/animdata.dat')
        contexts = [private(PRIVATE / f'research-temp/position-neutral-{i}.mem1')
                    for i in (14, 15, 16)]
        inputs = {str(p): digest(p) for p in (archive, target_archive, *contexts)}
        run(['bash', str(here / 'build_windows_model_postattach_original_check.sh'), str(out)],
            out / 'build.log')
        exe = out / 'model-postattach-original-check.exe'
        results = {}
        for index, context in zip((14, 15, 16), contexts):
            logs = []
            expected = ''.join(f'context{index} face{i}: whole mutated file/view, '
                               '3 allocations, storage, free order and repeated no-op exact\n'
                               for i in range(2))
            for repeat in range(2):
                log = out / f'context{index}-{repeat}.log'
                output = run([str(exe), windows(BASELINES['original_dll'][0]),
                              windows(context), windows(archive), windows(target_archive)], log)
                if output.replace('\r\n', '\n') != expected:
                    raise RuntimeError(f'Incomplete postattach comparison in context {index}')
                logs.append(digest(log))
            if logs[0] != logs[1]:
                raise RuntimeError(f'Fresh postattach traces differ in context {index}')
            results[str(index)] = {'fresh_processes': 2, 'face_entries': [12, 13],
                                   'whole_mutated_files': 2, 'whole_views': 2,
                                   'allocations_per_face': 3, 'log_sha256': logs}
        if before != baseline_hashes() or any(digest(Path(p)) != h for p, h in inputs.items()):
            raise RuntimeError('Protected baseline or asset input changed')
        if sources != source_hashes(root, entries):
            raise RuntimeError('Checker source changed during verification')
        record.update(status='passed', baseline=before, input_sha256=inputs,
                      source_sha256=sources, executable_sha256=digest(exe), contexts=results)
    except Exception as exc:
        record.update(status='failed', error=str(exc))
        raise
    finally:
        report.write_text(json.dumps(record, indent=2) + '\n')
    print('Original postattach binding checks pass; private report ' + str(report))


if __name__ == '__main__':
    main()
