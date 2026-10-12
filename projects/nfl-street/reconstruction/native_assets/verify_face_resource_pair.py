#!/usr/bin/env python3
"""Verify selected face acquisition and processed pair against the original DLL."""
import json
from pathlib import Path

from verify_player_startup import source_hashes
from verify_controller_checks import PRIVATE, BASELINES, baseline_hashes, digest, private, run, windows


def main() -> None:
    here = Path(__file__).resolve().parent
    root = here.parent
    out = private(PRIVATE / 'research-temp/nflstreet-face-resource-pair-20261012')
    out.mkdir(parents=True, exist_ok=True)
    report = out / 'verification.json'
    record = {
        'status': 'running',
        'boundary': 'Original 800E0AC4/801EB3F4 acquisition, 801C5EC8 relocation, '
                    '801C67E4 attach, and 800E0734 pool4 selection. Archive decoder '
                    '801A75B0 is supplied from independently decoded plaface.dat. '
                    'Optional postattach binding input r13-7220 is controlled to zero.',
    }
    report.write_text(json.dumps(record, indent=2) + '\n')
    entries = ('native_assets/face_resource_pair_original_check.c',
               'native_assets/build_windows_face_resource_pair_original_check.sh',
               'native_assets/face_resource_pair.c', 'native_assets/face_resource_pair.h',
               'native_assets/actor_model_init_original_check.c', 'native_assets/actor_model_init.c',
               'native_assets/model_relocate.c', 'native_assets/model_chunk_relocate.c',
               'native_assets/model_view.c', 'native_assets/archive.c', 'native_assets/lzh1.c')
    try:
        before = baseline_hashes()
        sources = source_hashes(root, entries)
        archive = private(PRIVATE / 'private/nflstreet/GNNE69-r0/assets/plaface.dat')
        contexts = [private(PRIVATE / f'research-temp/position-neutral-{i}.mem1')
                    for i in (14, 15, 16)]
        inputs = {str(p): digest(p) for p in (archive, *contexts)}
        run(['bash', str(here / 'build_windows_face_resource_pair_original_check.sh'), str(out)],
            out / 'build.log')
        exe = out / 'face-resource-pair-original-check.exe'
        expected = ('face entry12: original acquisition buffer80f64660 size51744 view80272614 exact\n'
                    'face entry13: original acquisition buffer80f5c360 size33504 view80272664 exact\n')
        results = {}
        for index, context in zip((14, 15, 16), contexts):
            logs = []
            for repeat in range(2):
                log = out / f'context{index}-{repeat}.log'
                output = run([str(exe), windows(BASELINES['original_dll'][0]),
                              windows(context), windows(archive)], log)
                if output.replace('\r\n', '\n') != expected:
                    raise RuntimeError(f'Incomplete face comparison in context {index}')
                logs.append(digest(log))
            if logs[0] != logs[1]:
                raise RuntimeError(f'Fresh face traces differ in context {index}')
            results[str(index)] = {'fresh_processes': 2, 'acquired_entries': [12, 13],
                                   'whole_files': 2, 'attached_views': 2,
                                   'pool4_queries': 2, 'log_sha256': logs}
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
    print('Original face resource pair checks pass; private report ' + str(report))


if __name__ == '__main__':
    main()
