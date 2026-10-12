#!/usr/bin/env python3
"""Check selected roster resource assembly using only original local archives."""
import json
from pathlib import Path
from verify_player_startup import source_hashes
from verify_controller_checks import PRIVATE, baseline_hashes, digest, private, run, windows


def main():
    here = Path(__file__).resolve().parent
    root = here.parent
    out = private(PRIVATE / 'research-temp/nflstreet-selected-resources-20261012')
    out.mkdir(parents=True, exist_ok=True)
    report = out / 'verification.json'
    data = private(PRIVATE / 'private/nflstreet/GNNE69-r0')
    entries = tuple('native_assets/' + n for n in (
        'selected_player_resources.c', 'selected_player_resources_check.c', 'face_resource_pair.c',
        'body_resource_pool.c', 'model_view.c', 'model_relocate.c', 'model_chunk_relocate.c',
        'archive.c', 'lzh1.c', 'build_windows_selected_player_resources_check.sh')) + tuple(
            'menus/' + n for n in ('team_player_body_binding.c', 'team_player_resolve.c',
                                   'team_player_manifest.c', 'team_catalog.c', 'team_identity.c'))
    record = {'status': 'running', 'boundary':
              'Native roster→face-pair and explicit processed body-variant resource assembly. '
              'No emulator/module/snapshot/catalog supplied at runtime. All regular-team rows; '
              'hair selection, textures/material bindings, optional original postattachment '
              'processing and actor creation remain unfinished. This is native data coverage, '
              'not full original-gameplay equivalence.'}
    try:
        before = baseline_hashes()
        sources = source_hashes(root, entries)
        archives = [data / 'assets' / f'{n}.dat' for n in ('plaface', 'plarosterhair', 'plamodel')]
        inputs = {str(p): digest(p) for p in [*archives, *[data / 'unpacked/db_teams' / f'{i:05}.bin' for i in range(1, 33)]]}
        run(['bash', str(here / 'build_windows_selected_player_resources_check.sh'), str(out)], out / 'build.log')
        exe = out / 'selected-player-resources-check.exe'
        traces = []
        for repeat in range(2):
            log = out / f'native-{repeat}.log'
            text = run([str(exe), *[windows(p) for p in archives], windows(data / 'unpacked')], log)
            if text.strip() != 'Selected native resources: 402 roster rows, 1206 explicit body-variant assemblies, 2412 processed face models, 3 ownership/input guards; no snapshot/module input.':
                raise RuntimeError('Incomplete native selected-resource checks')
            traces.append(digest(log))
        if traces[0] != traces[1]:
            raise RuntimeError('Fresh native resource traces differ')
        if before != baseline_hashes() or any(digest(Path(p)) != h for p, h in inputs.items()):
            raise RuntimeError('Protected originals or private inputs changed')
        if sources != source_hashes(root, entries):
            raise RuntimeError('Source changed during verification')
        record.update(status='passed', baseline=before, input_sha256=inputs, source_sha256=sources,
                      executable_sha256=digest(exe), fresh_processes=2, log_sha256=traces,
                      roster_rows=402, explicit_variant_assemblies=1206, processed_face_models=2412)
    except Exception as exc:
        record.update(status='failed', error=str(exc))
        raise
    finally:
        report.write_text(json.dumps(record, indent=2) + '\n')
    print('Selected native resource assembly passes; private report ' + str(report))


if __name__ == '__main__':
    main()
