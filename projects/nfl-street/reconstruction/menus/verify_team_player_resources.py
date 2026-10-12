"""Build and run the controlled original-DOL head/hair lookup oracle.

The original archives, generated queries, checker binary, and report remain on
the authorized private W: drive. The repository contains only this verifier
and the non-proprietary C harness.
"""
from __future__ import annotations

import hashlib
import json
import re
import subprocess
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[3]
PRIVATE = Path('/mnt/w/Daywater/private/nflstreet/GNNE69-r0')
WORK = Path('/mnt/w/Daywater/research-temp/nflstreet-team-player-loader')
MODULE = Path('/mnt/w/Daywater/research-temp/nflstreet-moderngekko-modules/GNNE69/a065ae2f4c192124266a56171aa5eed4e2dccb586d32670c5dab2e2ed3ad980c-c4dcfb815f5c2267/module-build/gGNNE69_recomp.so')
DOL = Path('/mnt/w/Daywater/research-temp/nflstreet-animation-20261008/original.dol')
DOL_SHA1 = '46e3de854998a0f66c9be8c72a1164d22cd29eb4'
SDK_INCLUDE = Path('/mnt/w/Daywater/tools/ModernGekko-Template/lib/ModernGekko/include')
CATALOGER = HERE / 'catalog_team_player_resources.py'
CHECKER_SOURCE = HERE / 'team_player_lookup_original_check.c'
BODY_CHECKER_SOURCE = HERE / 'team_player_body_route_original_check.c'
POOL_CHECKER_SOURCE = HERE / 'team_player_pool_constructor_original_check.c'
CATALOG = WORK / 'selected-resource-hash-catalog.json'
QUERIES = WORK / 'original-lookup-queries.tsv'
CHECKER = WORK / 'team_player_lookup_original_check'
TBYP_VALUES = WORK / 'play-tbyp-values.txt'
BODY_CHECKER = WORK / 'team_player_body_route_original_check'
POOL_CHECKER = WORK / 'team_player_pool_constructor_original_check'
REPORT = WORK / 'original-lookup-verification.json'


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> None:
    WORK.mkdir(parents=True, exist_ok=True)
    WORK.resolve().relative_to(Path('/mnt/w/Daywater').resolve())
    REPORT.write_text(json.dumps({'status': 'running', 'scope': 'controlled original lookup/body constructor; no rendering claim'}, indent=2) + '\n')
    for path in (PRIVATE, MODULE, SDK_INCLUDE, CATALOGER, CHECKER_SOURCE,
                 BODY_CHECKER_SOURCE, POOL_CHECKER_SOURCE, DOL):
        if not path.exists():
            raise FileNotFoundError(path)

    if hashlib.sha1(DOL.read_bytes()).hexdigest() != DOL_SHA1:
        raise ValueError(f'original DOL SHA1 differs: {DOL}')

    protected_sources = [MODULE, DOL, PRIVATE / 'assets/plaface.dat',
                         PRIVATE / 'assets/plarosterhair.dat',
                         PRIVATE / 'assets/uis_port.dat']
    protected_sources += [PRIVATE / 'unpacked/db_teams' / f'{entry:05}.bin'
                          for entry in range(1, 33)]
    protected_before = {str(path): sha256(path) for path in protected_sources}

    subprocess.run(['python', str(CATALOGER)], cwd=REPO, check=True,
                   stdout=subprocess.DEVNULL)
    catalog = json.loads(CATALOG.read_text())
    queries: list[tuple[str, str, str, int]] = []
    for record in catalog['records']:
        for pool, key_field, result_field in (
            ('face', 'head_key', 'head'),
            ('hair', 'hair_key', 'hair'),
            ('hair', 'hair_hat_key', 'hair_hat'),
        ):
            key = record[key_field]
            result = record[result_field]
            expected = result['index'] if result['index'] is not None else -1
            queries.append((pool, 'upper', key.upper(), expected))
            queries.append((pool, 'lower', key.lower(), expected))
    for pool in ('face', 'hair'):
        queries.append((pool, 'miss', 'NO_SUCH_MODEL_KEY_802F', -1))
        queries.append((pool, 'miss', 'no_such_model_key_802f', -1))
    QUERIES.write_text(''.join(f'{pool}\t{case}\t{key}\t{expected}\n'
                               for pool, case, key, expected in queries))

    # These are the unique TBYP values read from the actual PLAY rows. Include
    # selector 7 separately because 8004221C has a special 7 -> 2 override.
    tbyp_values = sorted({int(record['tbyp']) for record in catalog['records']})
    TBYP_VALUES.write_text(''.join(f'{value}\n' for value in tbyp_values) + '7\n')

    build_command = ['cc', '-O2', f'-I{SDK_INCLUDE}', str(CHECKER_SOURCE),
                     '-ldl', '-o', str(CHECKER)]
    subprocess.run(build_command, cwd=REPO, check=True)
    body_build_command = ['cc', '-O2', f'-I{SDK_INCLUDE}', str(BODY_CHECKER_SOURCE),
                          '-ldl', '-o', str(BODY_CHECKER)]
    subprocess.run(body_build_command, cwd=REPO, check=True)
    pool_build_command = ['cc', '-O2', f'-I{SDK_INCLUDE}', str(POOL_CHECKER_SOURCE),
                          str(HERE / 'team_preview.c'), str(HERE / 'team_catalog.c'),
                          '-ldl', '-o', str(POOL_CHECKER)]
    subprocess.run(pool_build_command, cwd=REPO, check=True)
    run_command = [str(CHECKER), str(MODULE), str(PRIVATE / 'assets/plaface.dat'),
                   str(PRIVATE / 'assets/plarosterhair.dat'), str(QUERIES)]
    body_run_command = [str(BODY_CHECKER), str(MODULE), str(TBYP_VALUES)]
    pool_run_command = [str(POOL_CHECKER), str(MODULE), str(DOL)]

    tracked = [*protected_sources, QUERIES, CATALOG,
               CATALOGER, CHECKER_SOURCE, BODY_CHECKER_SOURCE, Path(__file__),
               POOL_CHECKER_SOURCE, CHECKER, TBYP_VALUES, BODY_CHECKER,
               POOL_CHECKER]
    tracked += [HERE / name for name in ('team_preview.c', 'team_preview.h', 'team_catalog.c', 'team_catalog.h')]
    before = {str(path): sha256(path) for path in tracked}
    result = subprocess.run(run_command, cwd=REPO, check=True, text=True,
                            capture_output=True)
    body_result = subprocess.run(body_run_command, cwd=REPO, check=True, text=True,
                                 capture_output=True)
    pool_result = subprocess.run(pool_run_command, cwd=REPO, check=True, text=True,
                                 capture_output=True)
    after = {str(path): sha256(path) for path in tracked}
    protected_after = {str(path): sha256(path) for path in protected_sources}
    if before != after or protected_before != protected_after:
        raise RuntimeError('a protected oracle input changed while the check ran')
    match = re.search(r'passed: (\d+) cases \(upper=(\d+) lower=(\d+) misses=(\d+)\)',
                      result.stdout)
    if not match or tuple(map(int, match.groups())) != (2416, 1206, 1206, 4):
        raise RuntimeError(f'unexpected original lookup report: {result.stdout.strip()}')
    if 'passed: 4 supplied PLAY TBYP values' not in body_result.stdout:
        raise RuntimeError(f'unexpected original body-route report: {body_result.stdout.strip()}')
    if 'passed: 3 PLYRBODY groups, descriptor indices 1..9 grouped 1/4/7' not in pool_result.stdout:
        raise RuntimeError(f'unexpected original pool-constructor report: {pool_result.stdout.strip()}')

    report = {
        'status': 'passed',
        'scope': 'controlled original DOL 801A8088 lookup; no live menu/resource-acquisition claim',
        'game': 'NFL Street GameCube GNNE69-r0',
        'original_dol_sha1': '46e3de854998a0f66c9be8c72a1164d22cd29eb4',
        'original_dol_path': str(DOL),
        'original_module': str(MODULE),
        'build_command': build_command,
        'body_route_build_command': body_build_command,
        'pool_constructor_build_command': pool_build_command,
        'run_command': run_command,
        'body_route_run_command': body_run_command,
        'pool_constructor_run_command': pool_run_command,
        'query_generation': 'catalog actual PLAY rows; uppercase and lowercase each head/hair/hair-hat key; append uppercase/lowercase misses per pool',
        'queries': {'total': len(queries), 'uppercase': sum(q[1] == 'upper' for q in queries),
                    'lowercase': sum(q[1] == 'lower' for q in queries),
                    'misses': sum(q[1] == 'miss' for q in queries)},
        'controlled_ram_seed': '24 MiB zeroed MEM1, then supplied HSH1 descriptors/hash arrays, lookup keys, lowercase table, and readiness byte',
        'oracle_boundaries_stubbed': ['801AF944 pool mutex acquire', '801AF9CC pool mutex release',
                                      '801AF57C diagnostics/error reporting'],
        'result': result.stdout.strip(),
        'body_route_result': body_result.stdout.strip(),
        'pool_constructor_result': pool_result.stdout.strip(),
        'sha256_before_and_after': before,
        'protected_source_sha256_before_and_after': {
            'before': protected_before, 'after': protected_after},
        'protected_inputs_unchanged': before == after,
    }
    REPORT.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({'report': str(REPORT), 'result': report['result'],
                      'protected_inputs_unchanged': True}))


if __name__ == '__main__':
    try:
        main()
    except Exception as exc:
        REPORT.parent.mkdir(parents=True, exist_ok=True)
        REPORT.write_text(json.dumps({'status': 'failed', 'error': str(exc)}, indent=2) + '\n')
        raise
