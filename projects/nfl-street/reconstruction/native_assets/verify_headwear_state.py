#!/usr/bin/env python3
"""Check selected roster resource assembly using only original local archives."""
import json
from pathlib import Path
from verify_player_startup import source_hashes
from verify_controller_checks import BASELINES, PRIVATE, baseline_hashes, digest, private, run, windows


def main():
    here = Path(__file__).resolve().parent
    root = here.parent
    out = private(PRIVATE / 'research-temp/nflstreet-headwear-state-20261012')
    out.mkdir(parents=True, exist_ok=True)
    report = out / 'verification.json'
    data = private(PRIVATE / 'private/nflstreet/GNNE69-r0')
    entries = tuple('native_assets/' + n for n in (
        'selected_player_resources.c', 'selected_hair_resources_check.c', 'hair_resources.c',
        'headwear_state_original_check.c', 'automatic_hair_resources_check.c',
        'headwear_state.c', 'selected_player_geometry.c', 'actor_model_init.c',
        'build_windows_headwear_state_original_check.sh', 'face_resource_pair.c',
        'face_postattach.c', 'model_postattach.c', 'skeleton_resource.c',
        'body_resource_pool.c', 'model_view.c', 'model_relocate.c', 'model_chunk_relocate.c',
        'archive.c', 'lzh1.c', 'build_windows_automatic_hair_resources_check.sh')) + tuple(
            'menus/' + n for n in ('team_player_body_binding.c', 'team_player_resolve.c',
                                   'team_player_manifest.c', 'team_catalog.c', 'team_identity.c'))
    record = {'status':'running','boundary':
              'Native exact PLAY/DIGT+DIGP selection, PLAY-to-GEAR headwear derivation and owned face/body/hair geometry. '
              'Original field extraction and800DDEBC/800DDE18 decisions execute; GEAR query services '
              'are supplied using fresh original-extracted records. Full original DB registration, '
              'hair postattachment, materials/rendering and fresh actor startup remain outside scope.'}
    try:
        before = baseline_hashes()
        sources = source_hashes(root, entries)
        archives = [data / 'assets' / f'{n}.dat' for n in ('plaface', 'plarosterhair', 'plamodel', 'animdata', 'plahair')]
        contexts = [PRIVATE / f'research-temp/position-neutral-{i}.mem1' for i in (14,15,16)]
        gear_db = data / 'unpacked/template/00002.bin'
        inputs = {str(p): digest(p) for p in [*archives, gear_db, *contexts, *[data / 'unpacked/db_teams' / f'{i:05}.bin' for i in range(1, 33)]]}
        run(['bash', str(here / 'build_windows_automatic_hair_resources_check.sh'), str(out)], out / 'build.log')
        exe = out / 'automatic-hair-resources-check.exe'
        traces = []
        for repeat in range(2):
            log = out / f'native-{repeat}.log'
            text = run([str(exe), *[windows(p) for p in archives[:4]], windows(data / 'unpacked'), windows(archives[4]), windows(gear_db)], log)
            expected = ('Selected native resources: 402 roster rows, 1206 explicit body-variant assemblies, 2412 bound face models, 3 ownership/input guards; fresh target and repeated binding, no snapshot/module input.\n'
                        'Automatic hair resources: 402 derived plans, 1206 processed models across 402 roster rows; rendering remains outside scope.')
            if text.replace('\r\n','\n').strip() != expected:
                raise RuntimeError('Incomplete native selected-resource checks')
            traces.append(digest(log))
        if traces[0] != traces[1]:
            raise RuntimeError('Fresh native resource traces differ')
        run(['bash', str(here / 'build_windows_headwear_state_original_check.sh'), str(out)], out / 'original-build.log')
        original_exe = out / 'headwear-state-original-check.exe'
        original_traces = {}
        for i,context in zip((14,15,16),contexts):
            logs=[]
            for repeat in range(2):
                log=out / f'original-{i}-{repeat}.log'
                text=run([str(original_exe), *[windows(p) for p in archives[:4]], windows(data / 'unpacked'),
                          windows(archives[4]), windows(gear_db), windows(BASELINES['original_dll'][0]), windows(context)],log)
                if text.replace('\r\n','\n').strip() != expected + '\nHeadwear original checks: 1650 extracted fields, 402 selectors, 64 mapped values; fresh GEAR query services supplied.':
                    raise RuntimeError('Incomplete original hair comparison')
                logs.append(digest(log))
            if logs[0]!=logs[1]:raise RuntimeError('Fresh original hair traces differ')
            original_traces[str(i)] = logs
        if before != baseline_hashes() or any(digest(Path(p)) != h for p, h in inputs.items()):
            raise RuntimeError('Protected originals or private inputs changed')
        if sources != source_hashes(root, entries):
            raise RuntimeError('Source changed during verification')
        record.update(status='passed', baseline=before, input_sha256=inputs, source_sha256=sources,
                      executable_sha256=digest(exe), fresh_processes=2, log_sha256=traces,
                      roster_rows=402, derived_hair_plans=402, processed_hair_models=1206, owned_geometry_assemblies=1206,
                      original_selectors_per_process=402, original_fields_per_process=1650, original_maps_per_process=64,
                      original_executable_sha256=digest(original_exe), original_log_sha256=original_traces)
    except Exception as exc:
        record.update(status='failed', error=str(exc))
        raise
    finally:
        report.write_text(json.dumps(record, indent=2) + '\n')
    print('Native automatic headwear/geometry and original decision checks pass; private report ' + str(report))


if __name__ == '__main__':
    main()
