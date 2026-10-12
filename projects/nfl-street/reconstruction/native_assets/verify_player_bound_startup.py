#!/usr/bin/env python3
"""Check actual factory-to-model-binding creation against connected native owners."""
import json
import re
from pathlib import Path
from verify_player_startup import source_hashes
from verify_controller_checks import PRIVATE, BASELINES, baseline_hashes, digest, private, run, windows


def main():
    root = Path(__file__).resolve().parent
    out = private(PRIVATE / 'research-temp/nflstreet-player-bound-startup-20261012')
    out.mkdir(parents=True, exist_ok=True)
    report = out / 'verification.json'
    entries = ('player_bound_startup.c', 'player_bound_startup_original_check.c',
               'player_model_binding.c', 'player_startup.c', 'actor_factory.c',
               'player_constructor.c', 'actor_model_init.c', 'actor_tail_init.c',
               'player_appearance_init.c', 'build_windows_player_bound_startup_original_check.sh')
    record = {'status': 'running', 'boundary':
              'Actual801956A4→800E1F7C→801639C8→801630B0 and owned helpers. '
              'Reference descriptor/catalog/resource views; allocation, cache/query, '
              'name comparison, node/texture and renderer services supplied. '
              'No fresh selected-resource loading or playable match claim. '
              'Whole RAM comparison excludes isolated CPU stack scratch816F0000..8170FFFF.'}
    try:
        before = baseline_hashes()
        sources = source_hashes(root, entries)
        inputs = {str(private(PRIVATE / f'research-temp/position-neutral-{c}.mem1')):
                  digest(private(PRIVATE / f'research-temp/position-neutral-{c}.mem1')) for c in (14, 15, 16)}
        run(['bash', str(root / 'build_windows_player_bound_startup_original_check.sh'), str(out)], out / 'build.log')
        exe = out / 'player-bound-startup-original-check.exe'
        expected = [(str(mask), str(p), str(m), str(s), '88', '2084', '2172')
                    for mask in range(64) for p in range(2) for m in range(2) for s in range(2)]
        contexts = {}
        for context in (14, 15, 16):
            traces = []
            for repeat in range(2):
                log = out / f'root-context{context}-{repeat}.log'
                text = run([str(exe), windows(BASELINES['original_dll'][0]),
                            windows(PRIVATE / f'research-temp/position-neutral-{context}.mem1')], log)
                rows = re.findall(r'^bound-startup mask=(\d+) prefilled=(\d+) mutation=(\d+) shared=(\d+): actor1A90 \+\d+ model blocks \+two appearance allocations \+\d+ binding allocations \+two registries \+(\d+)/(\d+) requests exact ordered=(\d+)', text, re.M)
                if rows != expected:
                    raise RuntimeError(f'Incomplete bound startup comparison: context{context}')
                traces.append(digest(log))
            if traces[0] != traces[1]:
                raise RuntimeError('Fresh bound startup traces differ')
            contexts[str(context)] = {'cases': 512, 'fresh_processes': 2, 'log_sha256': traces}
        if before != baseline_hashes() or any(digest(Path(p)) != h for p, h in inputs.items()):
            raise RuntimeError('Protected originals or inputs changed')
        if sources != source_hashes(root, entries):
            raise RuntimeError('Source changed during verification')
        record.update(status='passed', baseline=before, input_sha256=inputs,
                      source_sha256=sources, executable_sha256=digest(exe), contexts=contexts)
    except Exception as exc:
        record.update(status='failed', error=str(exc))
        raise
    finally:
        report.write_text(json.dumps(record, indent=2) + '\n')
    print('Connected factory/model-binding comparisons pass; private report ' + str(report))


if __name__ == '__main__':
    main()
