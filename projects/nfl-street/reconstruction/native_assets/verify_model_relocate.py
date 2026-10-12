#!/usr/bin/env python3
"""Verify whole-body relocation and processed resource pools against original helpers."""
import json
from pathlib import Path
from verify_player_startup import source_hashes
from verify_controller_checks import PRIVATE, BASELINES, baseline_hashes, digest, private, run, windows


def main():
    here = Path(__file__).resolve().parent
    root = here.parent
    out = private(PRIVATE / 'research-temp/nflstreet-model-relocate-20261012')
    out.mkdir(parents=True, exist_ok=True)
    report = out / 'verification.json'
    entries = ('native_assets/model_view.c', 'native_assets/body_resource_pool.c',
               'native_assets/model_relocate_original_check.c', 'native_assets/actor_model_init.c',
               'native_assets/archive.c', 'native_assets/lzh1.c',
               'native_assets/model_relocate.c', 'native_assets/model_chunk_relocate.c',
               'menus/team_player_body_binding.c', 'native_assets/build_windows_model_relocate_original_check.sh')
    record = {'status': 'running', 'boundary':
              'Original801C5EC8 dispatcher, registered chunk handlers and801C67E4 with actual decoded '
              'plamodel entries. Whole files, repeat no-op calls and attached/pooled views compared. '
              'No full original rendering, complete selected-player appearance or fresh match claim.'}
    try:
        before = baseline_hashes()
        sources = source_hashes(root, entries)
        archive = private(PRIVATE / 'private/nflstreet/GNNE69-r0/assets/plamodel.dat')
        inputs = {str(p): digest(p) for p in
                  [archive, *[private(PRIVATE / f'research-temp/position-neutral-{c}.mem1') for c in (14, 15, 16)]]}
        run(['bash', str(here / 'build_windows_model_relocate_original_check.sh'), str(out)], out / 'build.log')
        exe = out / 'model-relocate-original-check.exe'
        contexts = {}
        for context in (14, 15, 16):
            traces = []
            for repeat in range(2):
                log = out / f'context{context}-{repeat}.log'
                text = run([str(exe), windows(BASELINES['original_dll'][0]),
                            windows(PRIVATE / f'research-temp/position-neutral-{context}.mem1'), windows(archive)], log)
                if text.strip() != 'Relocated body models: 36 whole files +36 repeated no-op calls +36 attached views +9 processed pooled assets exact.':
                    raise RuntimeError('Incomplete attachment comparison')
                traces.append(digest(log))
            if traces[0] != traces[1]:
                raise RuntimeError('Fresh attachment traces differ')
            contexts[str(context)] = {'relocated_files': 36, 'repeat_calls': 36, 'attached_views': 36, 'pooled_assets': 9,
                                      'fresh_processes': 2, 'log_sha256': traces}
        if before != baseline_hashes() or any(digest(Path(p)) != h for p, h in inputs.items()):
            raise RuntimeError('Protected originals or input archive changed')
        if sources != source_hashes(root, entries):
            raise RuntimeError('Source changed during verification')
        record.update(status='passed', baseline=before, input_sha256=inputs, source_sha256=sources,
                      executable_sha256=digest(exe), contexts=contexts)
    except Exception as exc:
        record.update(status='failed', error=str(exc))
        raise
    finally:
        report.write_text(json.dumps(record, indent=2) + '\n')
    print('Relocated body resource checks pass; private report ' + str(report))


if __name__ == '__main__':
    main()
