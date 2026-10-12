#!/usr/bin/env python3
"""Build/check controller reconstruction; keep all diagnostics private on W:."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

PRIVATE = Path('/mnt/w/Daywater')
BASELINES = {
    'dol': (PRIVATE / 'research-temp/nflstreet-native-windows-local/game/sys/main.dol',
            'sha1', '46e3de854998a0f66c9be8c72a1164d22cd29eb4'),
    'original_dll': (PRIVATE / 'research-temp/nflstreet-native-windows-local/gGNNE69_recomp.dll',
                     'sha1', 'dd521cc38b0db6bca98a0bb261570a2be5e6e0ed'),
    'original_runner_source': (PRIVATE / 'tools/ModernGekko-Template/lib/ModernGekko/tools/moderngekko_run.cpp',
                               'sha256', '3f9e3a3eb4b5728d9036e9552c01e8c9dc084a1f53805fef95dcf3b1b86f73ea'),
}
PATTERNS = {
    'selection': re.compile(r'exact:(\d+) cycles,(\d+) remaps,(\d+) assignments,(\d+) reads,(\d+) commits with ordered effects,(\d+) actual screen callback comparisons; (\d+) source nontermination cases'),
    'devices': re.compile(r'exact:(\d+) refreshes,(\d+) local initializations,(\d+) actual screen callbacks'),
}
FIELDS = {
    'selection': ('cycles', 'remaps', 'assignments', 'reads', 'commits', 'callbacks', 'bounded_nontermination_guards'),
    'devices': ('refreshes', 'initializations', 'callbacks'),
}


def digest(path: Path, algorithm: str = 'sha256') -> str:
    h = hashlib.new(algorithm)
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            h.update(block)
    return h.hexdigest()


def baseline_hashes() -> dict:
    result = {}
    for name, (path, algorithm, expected) in BASELINES.items():
        actual = digest(path, algorithm)
        if actual != expected:
            raise RuntimeError(f'{name} differs from the preserved baseline')
        result[name] = {'path': str(path), algorithm: actual}
    return result


def windows(path: Path) -> str:
    return 'W:\\' + str(path.resolve().relative_to(Path('/mnt/w'))).replace('/', '\\')


def private(path: Path) -> Path:
    resolved = path.resolve()
    if not resolved.is_relative_to(PRIVATE.resolve()):
        raise ValueError('Inputs and outputs must stay beneath /mnt/w/Daywater')
    return resolved


def run(command: list[str], log: Path) -> str:
    with log.open('w', encoding='utf-8') as output:
        subprocess.run(command, stdout=output, stderr=subprocess.STDOUT, check=True)
    return log.read_text(encoding='utf-8')


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=PRIVATE / 'research-temp/nflstreet-controller-devices-20261010')
    parser.add_argument('--no-build', action='store_true', help='Reuse existing private executables; their hashes are recorded.')
    args = parser.parse_args()
    out = private(args.output)
    out.mkdir(parents=True, exist_ok=True)
    record_path = out / 'verification.json'
    # Invalidate a previous success before doing any work, including baseline checks.
    record = {'status': 'running', 'boundary': 'Controlled original-module comparisons with supplied device/profile responses; no visible source screen activation or hardware discovery.'}
    record_path.write_text(json.dumps(record, indent=2) + '\n')
    try:
        before = baseline_hashes()
        root = Path(__file__).resolve().parent
        if not args.no_build:
            run(['bash', str(root / 'build_windows_quick_game_checks.sh'), str(out)], out / 'build.log')
        results, hashes = {}, {}
        for kind, executable in [('selection', 'controller-select-original-check.exe'), ('devices', 'controller-devices-original-check.exe')]:
            exe = private(out / executable)
            hashes[executable] = digest(exe)
            results[kind] = {'contexts': {}, 'totals': dict.fromkeys(FIELDS[kind], 0)}
            for context in (14, 15, 16):
                snapshot = private(PRIVATE / f'research-temp/position-neutral-{context}.mem1')
                hashes[str(snapshot)] = digest(snapshot)
                text = run([str(exe), windows(BASELINES['original_dll'][0]), windows(snapshot)], out / f'{kind}-context{context}.log')
                match = PATTERNS[kind].search(text)
                if not match:
                    raise RuntimeError(f'{kind} context{context} did not return its verification summary')
                counts = dict(zip(FIELDS[kind], map(int, match.groups())))
                results[kind]['contexts'][str(context)] = counts
                for key, value in counts.items():
                    results[kind]['totals'][key] += value
        screens = PRIVATE / 'research-temp/nflstreet-original-menus-20261009/nested/uis_all/00000'
        inputs = [screens / f'{entry:05d}.bin' for entry in (1, 2, 5)]
        inputs.append(PRIVATE / 'private/nflstreet/GNNE69-r0/unpacked/uis_font/00003.bin')
        for path in inputs:
            hashes[str(private(path))] = digest(path)
        exe = out / 'quick-route-check.exe'
        hashes[exe.name] = digest(exe)
        text = run([str(exe), *map(windows, inputs)], out / 'quick-route-regression.log')
        if 'All source ordering requests applied' not in text:
            raise RuntimeError('Independent Quick Game routing did not return its completion summary')
        after = baseline_hashes()
        if before != after:
            raise RuntimeError('Original baselines changed during verification')
        for pattern in ('controller_*.c', 'controller_*.h'):
            for path in sorted(root.glob(pattern)):
                hashes[str(path)] = digest(path)
        hashes[str(root / 'build_windows_quick_game_checks.sh')] = digest(root / 'build_windows_quick_game_checks.sh')
        hashes[str(Path(__file__).resolve())] = digest(Path(__file__).resolve())
        record.update(status='passed', baselines=after, hashes=hashes, results=results,
                      independent_routing='passed; selector completions remain supplied',
                      limitations=['Local query branch only; profile/device inputs supplied.',
                                   'Binding, polling, profile and audio effects are retained; no physical device or profile database I/O.',
                                   'Native bounded rejection differs from non-returning supplied original states.',
                                   'Source screen construction/activation/input/completion and selected-match creation remain unfinished.'])
        print(json.dumps({'status': 'passed', 'totals': {k: v['totals'] for k, v in results.items()}, 'independent_routing': 'passed'}))
    except Exception as error:
        record.update(status='failed', error=str(error))
        raise
    finally:
        record_path.write_text(json.dumps(record, indent=2) + '\n')


if __name__ == '__main__':
    main()
