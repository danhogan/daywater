"""Verify bounded PLYRBODY key/variant to actual plamodel bytes and original attach."""
from __future__ import annotations

import hashlib
import json
import subprocess
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[3]
ROOT = Path('/mnt/w/Daywater')
PRIVATE = ROOT / 'private/nflstreet/GNNE69-r0'
WORK = ROOT / 'research-temp/nflstreet-team-lineup-20261010'
MODULE = ROOT / ('research-temp/nflstreet-moderngekko-modules/GNNE69/'
                 'a065ae2f4c192124266a56171aa5eed4e2dccb586d32670c5dab2e2ed3ad980c-'
                 'c4dcfb815f5c2267/module-build/gGNNE69_recomp.so')
DOL = ROOT / 'research-temp/nflstreet-animation-20261008/original.dol'
DOL_SHA1 = '46e3de854998a0f66c9be8c72a1164d22cd29eb4'
SDK = ROOT / 'tools/ModernGekko-Template/lib/ModernGekko/include'
ARCHIVE = PRIVATE / 'assets/plamodel.dat'
CHECKER = HERE / 'team_player_body_binding_original_check.c'
BODY_ROUTE = HERE / 'team_player_body_route_original_check.c'
API = HERE / 'team_player_body_binding.c'
API_HEADER = HERE / 'team_player_body_binding.h'
ARCHIVE_C = HERE.parent / 'native_assets/archive.c'
ARCHIVE_H = HERE.parent / 'native_assets/archive.h'
LZH = HERE.parent / 'native_assets/lzh1.c'
BIN = WORK / 'team_player_body_binding_original_check'
ROUTE_BIN = WORK / 'team_player_body_route_original_check'
VALUES = WORK / 'tbyp-values.txt'
REPORT = WORK / 'body-binding-verification.json'


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> None:
    WORK.mkdir(parents=True, exist_ok=True)
    WORK.resolve().relative_to(ROOT.resolve())
    sources = [CHECKER, BODY_ROUTE, API, API_HEADER, ARCHIVE_C, ARCHIVE_H, LZH]
    protected = [MODULE, DOL, ARCHIVE]
    for path in [*sources, *protected, SDK]:
        if not path.exists():
            raise FileNotFoundError(path)
    if hashlib.sha1(DOL.read_bytes()).hexdigest() != DOL_SHA1:
        raise ValueError('original DOL SHA1 mismatch')
    before = {str(p): sha256(p) for p in [*sources, *protected]}
    VALUES.write_text('0\n1\n2\n7\n')
    command = ['cc', '-std=c11', '-Wall', '-Wextra', '-Werror', '-O2',
               f'-I{SDK}', str(CHECKER), str(API), str(ARCHIVE_C), str(LZH),
               '-ldl', '-o', str(BIN)]
    subprocess.run(command, cwd=REPO, check=True)
    # Existing body-route checker has a known indentation warning; it is
    # untouched and compiled without promoting that warning to an error.
    route_command = ['cc', '-std=c11', '-Wall', '-Wextra', '-O2',
                     f'-I{SDK}', str(BODY_ROUTE), '-ldl', '-o', str(ROUTE_BIN)]
    subprocess.run(route_command, cwd=REPO, check=True)
    run = subprocess.run([str(BIN), str(MODULE), str(DOL), str(ARCHIVE)],
                         cwd=REPO, check=True, text=True, capture_output=True)
    route_run = subprocess.run([str(ROUTE_BIN), str(MODULE), str(VALUES)],
                               cwd=REPO, check=True, text=True, capture_output=True)
    after = {str(p): sha256(p) for p in [*sources, *protected]}
    if before != after:
        raise RuntimeError('a protected input/source changed during the check')
    if 'Passed: 9 actual plamodel TERF entries decoded and Info attached' not in run.stdout:
        raise RuntimeError(f'unexpected original attach result: {run.stdout}')
    if 'passed: 4 supplied PLAY TBYP values' not in route_run.stdout:
        raise RuntimeError(f'unexpected TBYP route result: {route_run.stdout}')
    report = {
        'status': 'passed',
        'confidence': 'verified for the controlled archive-decoder boundary and original descriptor/attach path; not a claim of full runtime resource registration or rendering',
        'game': 'NFL Street GameCube GNNE69-r0',
        'original_dol_sha1': DOL_SHA1,
        'boundary': 'Native TERF/DIR1 decoder supplies decoded archive bytes at original 801A75B0; untouched original 800DFBE4/801EB3F4 and 801C65A0/801C67E4 execute. 801C5EC8 lock prelude is stubbed.',
        'mapping': 'TBYP 0,1,2 -> PLYRBODY keys 0,1,2; TBYP 7 -> key 2; descriptor variant 0..2 -> archive indices 1+key*3+variant.',
        'result': run.stdout.strip(),
        'route_result': route_run.stdout.strip(),
        'build_command': command,
        'route_build_command': route_command,
        'sha256_before_after': {'before': before, 'after': after},
        'protected_inputs_unchanged': before == after,
    }
    REPORT.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({'report': str(REPORT), 'status': report['status'],
                      'protected_inputs_unchanged': True}))


if __name__ == '__main__':
    try:
        main()
    except Exception as exc:
        WORK.mkdir(parents=True, exist_ok=True)
        REPORT.write_text(json.dumps({'status': 'failed', 'error': str(exc)}, indent=2) + '\n')
        raise
