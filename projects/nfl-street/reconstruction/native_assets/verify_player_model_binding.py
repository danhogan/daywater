#!/usr/bin/env python3
"""Repeat owned player model binding against original801630B0."""
import json
import re
from pathlib import Path
from verify_player_startup import source_hashes
from verify_controller_checks import PRIVATE, BASELINES, baseline_hashes, digest, private, run, windows


def main():
    root = Path(__file__).resolve().parent
    out = private(PRIVATE / 'research-temp/nflstreet-player-model-binding-20261012')
    out.mkdir(parents=True, exist_ok=True)
    report = out / 'verification.json'
    entries = ('player_model_binding.c', 'player_model_binding_original_check.c',
               'actor_model_init.c', 'build_windows_player_model_binding_original_check.sh')
    record = {'status': 'running', 'boundary':
              'Original801630B0 and owned model-copy/command/index-remap helpers. '
              'Reference model views supplied; cache/resource/name/render services supplied. '
              'No selected-resource loading or original rendering fidelity claim.'}
    try:
        before = baseline_hashes()
        sources = source_hashes(root, entries)
        inputs = {str(private(PRIVATE / f'research-temp/position-neutral-{c}.mem1')):
                  digest(private(PRIVATE / f'research-temp/position-neutral-{c}.mem1')) for c in (14, 15, 16)}
        run(['bash', str(root / 'build_windows_player_model_binding_original_check.sh'), str(out)], out / 'build.log')
        exe = out / 'player-model-binding-original-check.exe'
        expected = [(str(p), str(z), str(s), str(t), '2084')
                    for p in range(2) for z in range(2) for s in range(2) for t in range(2)]
        contexts = {}
        for context in (14, 15, 16):
            traces = []
            for repeat in range(2):
                log = out / f'root-context{context}-{repeat}.log'
                text = run([str(exe), windows(BASELINES['original_dll'][0]),
                            windows(PRIVATE / f'research-temp/position-neutral-{context}.mem1')], log)
                rows = re.findall(r'^model-binding prefilled=(\d+) null=(\d+) shared=(\d+) mutation=(\d+): actor1A90 \+\d+ allocations \+(\d+) ordered requests \+catalog/shared/source mutations exact', text, re.M)
                if rows != expected:
                    raise RuntimeError(f'Incomplete binding comparisons: context{context}')
                traces.append(digest(log))
            if traces[0] != traces[1]:
                raise RuntimeError('Fresh binding logs differ')
            contexts[str(context)] = {'cases': 16, 'fresh_processes': 2, 'log_sha256': traces}
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
    print('Owned player model binding comparisons pass; private report ' + str(report))


if __name__ == '__main__':
    main()
