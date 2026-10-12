#!/usr/bin/env python3
"""Compare fresh actor factory headers and allocation behavior to original801956A4."""
import json
from pathlib import Path
from verify_player_startup import source_hashes
from verify_controller_checks import PRIVATE, BASELINES, baseline_hashes, digest, private, run, windows


def main():
    root = Path(__file__).resolve().parent
    out = private(PRIVATE / 'research-temp/nflstreet-actor-factory-20261012')
    out.mkdir(parents=True, exist_ok=True)
    report = out / 'verification.json'
    record = {'status': 'running', 'boundary':
              'Original801956A4 allocation/zeroing/header/constructor ordering. '
              'Original type0 size/owner supplied from saved registry; alternate records '
              'and constructor behavior controlled. Allocation service supplied; no pool allocator or fresh match claim.'}
    try:
        before = baseline_hashes()
        sources = source_hashes(root, ('actor_factory.c', 'actor_factory_original_check.c',
                               'actor_model_init.c', 'build_windows_actor_factory_original_check.sh'))
        inputs = {str(private(PRIVATE / f'research-temp/position-neutral-{c}.mem1')):
                  digest(private(PRIVATE / f'research-temp/position-neutral-{c}.mem1')) for c in (14, 15, 16)}
        run(['bash', str(root / 'build_windows_actor_factory_original_check.sh'), str(out)], out / 'build.log')
        exe = out / 'actor-factory-original-check.exe'
        contexts = {}
        for context in (14, 15, 16):
            traces = []
            for repeat in range(2):
                log = out / f'context{context}-{repeat}.log'
                text = run([str(exe), windows(BASELINES['original_dll'][0]),
                            windows(PRIVATE / f'research-temp/position-neutral-{context}.mem1')], log)
                if text.strip() != 'Factory original801956A4: 96 cases exact; full storage, allocation and constructor entry.':
                    raise RuntimeError('Incomplete factory comparison')
                traces.append(digest(log))
            if traces[0] != traces[1]:
                raise RuntimeError('Fresh factory comparison logs differ')
            contexts[str(context)] = {'cases': 96, 'fresh_processes': 2, 'log_sha256': traces}
        if before != baseline_hashes() or any(digest(Path(p)) != h for p, h in inputs.items()):
            raise RuntimeError('Protected original or input changed')
        if sources != source_hashes(root, ('actor_factory.c', 'actor_factory_original_check.c',
                                   'actor_model_init.c', 'build_windows_actor_factory_original_check.sh')):
            raise RuntimeError('Source changed during verification')
        record.update(status='passed', baseline=before, input_sha256=inputs, source_sha256=sources,
                      executable_sha256=digest(exe), contexts=contexts)
    except Exception as exc:
        record.update(status='failed', error=str(exc))
        raise
    finally:
        report.write_text(json.dumps(record, indent=2) + '\n')
    print('Fresh actor factory comparisons pass; private report ' + str(report))


if __name__ == '__main__':
    main()
