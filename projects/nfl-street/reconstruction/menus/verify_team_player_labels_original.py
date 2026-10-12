"""Generate private PLAY label queries and run the original 8014E9A0 oracle."""
from __future__ import annotations

import hashlib
import importlib.util
import json
import subprocess
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[3]
PRIVATE = Path('/mnt/w/Daywater/private/nflstreet/GNNE69-r0')
WORK = Path('/mnt/w/Daywater/research-temp/nflstreet-team-player-loader')
MODULE = Path('/mnt/w/Daywater/research-temp/nflstreet-moderngekko-modules/GNNE69/a065ae2f4c192124266a56171aa5eed4e2dccb586d32670c5dab2e2ed3ad980c-c4dcfb815f5c2267/module-build/gGNNE69_recomp.so')
MODULE_SHA256 = 'ce97e16c6bb176b11598b3e816b9d6b43734943bcbc52d66195bf5020bc4b5ce'
SDK_INCLUDE = Path('/mnt/w/Daywater/tools/ModernGekko-Template/lib/ModernGekko/include')
CATALOGER = HERE / 'catalog_team_player_resources.py'
CHECKER_SOURCE = HERE / 'team_player_labels_original_check.c'
QUERIES = WORK / 'team-player-label-original-queries.tsv'
CHECKER = WORK / 'team_player_labels_original_check'
REPORT = WORK / 'team-player-label-original-verification.json'

PLAY_TAG = 0x59414c50
DIGT_TAG = 0x44494754
PFNA_TAG = 0x414e4650
PLNA_TAG = 0x414e4c50


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def load_cataloger():
    spec = importlib.util.spec_from_file_location('private_team_cataloger', CATALOGER)
    if spec is None or spec.loader is None:
        raise RuntimeError('cannot load the local private DB reader')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def fixed_text(row: bytes, descriptor: tuple[int, int, int], expected_width: int) -> bytes:
    kind, bit, width = descriptor
    if kind != 0 or bit % 8 or width != expected_width * 8:
        raise ValueError('unexpected PLAY fixed-string descriptor')
    raw = row[bit // 8:(bit + width) // 8]
    return raw + b'\0'


def expected_name(first: bytes, last: bytes, capacity: int) -> bytes:
    first = first.split(b'\0', 1)[0]
    last = last.split(b'\0', 1)[0]
    joined = first + (b' ' if first and last else b'') + last
    return joined[:capacity]


def encode(raw: bytes) -> str:
    return raw.hex() if raw else '-'


def main() -> None:
    WORK.mkdir(parents=True, exist_ok=True)
    WORK.resolve().relative_to(Path('/mnt/w/Daywater').resolve())
    for path in (PRIVATE, MODULE, SDK_INCLUDE, CATALOGER, CHECKER_SOURCE):
        if not path.exists():
            raise FileNotFoundError(path)
    if sha256(MODULE) != MODULE_SHA256:
        raise ValueError('unexpected original module SHA-256')

    sources = [MODULE] + [PRIVATE / 'unpacked/db_teams' / f'{entry:05}.bin'
                          for entry in range(1, 33)]
    protected_before = {str(path): sha256(path) for path in sources}
    parser = load_cataloger()
    queries: list[str] = []
    play_rows = 0
    for entry in range(1, 33):
        data = (PRIVATE / 'unpacked/db_teams' / f'{entry:05}.bin').read_bytes()
        table_at, field_count, row_bytes, used, row_start = parser.db_table(data, PLAY_TAG)
        fields = parser.db_fields(data, table_at, field_count)
        for required in (DIGT_TAG, PFNA_TAG, PLNA_TAG):
            if required not in fields:
                raise ValueError(f'team DB {entry:05} is missing a PLAY label field')
        for row_index in range(used):
            row = data[row_start + row_index * row_bytes:row_start + (row_index + 1) * row_bytes]
            team_id = parser.get_value(row, fields[DIGT_TAG])
            if not team_id:
                continue
            pfna = fixed_text(row, fields[PFNA_TAG], 11).ljust(12, b'\0')
            plna = fixed_text(row, fields[PLNA_TAG], 13).ljust(14, b'\0')
            for capacity in range(27):
                expected = expected_name(pfna, plna, capacity)
                queries.append(f'{capacity}\t{encode(pfna)}\t{encode(plna)}\t{encode(expected)}\tplay\n')
            play_rows += 1
    if play_rows != 402:
        raise ValueError(f'expected 402 actual PLAY rows, got {play_rows}')

    corners = (
        (b'', b''),
        (b'First', b''),
        (b'', b'Last'),
        (b'ABCDEFGHIJK', b'LMNOPQRSTUVWX'),
    )
    for first, last in corners:
        pfna = first.ljust(12, b'\0')
        plna = last.ljust(14, b'\0')
        for capacity in range(27):
            expected = expected_name(pfna, plna, capacity)
            queries.append(f'{capacity}\t{encode(pfna)}\t{encode(plna)}\t{encode(expected)}\tcorner\n')
    QUERIES.write_text(''.join(queries))

    build_command = ['cc', '-O2', f'-I{SDK_INCLUDE}', str(CHECKER_SOURCE),
                     '-ldl', '-o', str(CHECKER)]
    subprocess.run(build_command, cwd=REPO, check=True)
    run_command = [str(CHECKER), str(MODULE), str(QUERIES)]
    tracked = [*sources, QUERIES, CATALOGER, CHECKER_SOURCE, Path(__file__), CHECKER]
    before = {str(path): sha256(path) for path in tracked}
    result = subprocess.run(run_command, cwd=REPO, check=True, text=True,
                            capture_output=True)
    after = {str(path): sha256(path) for path in tracked}
    protected_after = {str(path): sha256(path) for path in sources}
    if before != after or protected_before != protected_after:
        raise RuntimeError('protected module or DB input changed during original-helper check')

    expected_cases = (402 + len(corners)) * 27
    if f'passed: {402 * 27} actual PLAY row-capacity probes and {len(corners) * 27}' not in result.stdout:
        raise RuntimeError(f'unexpected original helper result: {result.stdout.strip()}')
    report = {
        'scope': 'original 8014E9A0 name helper with supplied raw PFNA/PLNA DB getter; no renderer claim',
        'original_dol_sha1': '46e3de854998a0f66c9be8c72a1164d22cd29eb4',
        'original_module': str(MODULE),
        'original_module_sha256': MODULE_SHA256,
        'play_rows': play_rows,
        'capacity_values': list(range(27)),
        'corner_cases': len(corners),
        'query_cases': expected_cases,
        'query_generation': 'actual PLAY/PFNA and PLAY/PLNA fixed DB slots from all 32 team blocks; expected First + Last with separator only when both are nonempty; capacities 0..26; four synthetic empty/full-width edges',
        'build_command': build_command,
        'run_command': run_command,
        'result': result.stdout.strip(),
        'sha256_before_and_after': before,
        'protected_inputs_unchanged': protected_before == protected_after,
    }
    REPORT.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({'report': str(REPORT), 'result': report['result'],
                      'protected_inputs_unchanged': True}))


if __name__ == '__main__':
    main()
