"""Verify one selected PLAY head lookup joins to original model geometry.

The current example is selected team 1 / DIGP 355. The original lookup
verifier must have passed before this script accepts the private catalog row.
This verifies a stored-pose geometry inspection, not the original actor's live
matrix/texture/attachment result.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
import struct
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[3]
WORK = Path('/mnt/w/Daywater/research-temp/nflstreet-team-player-loader')
PRIVATE = Path('/mnt/w/Daywater/private/nflstreet/GNNE69-r0')
DOL_SHA1 = '46e3de854998a0f66c9be8c72a1164d22cd29eb4'


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--team-id', type=int, default=1)
    parser.add_argument('--player-id', type=int, default=355)
    parser.add_argument('--catalog', type=Path, default=WORK / 'selected-resource-hash-catalog.json')
    parser.add_argument('--oracle-report', type=Path, default=WORK / 'original-lookup-verification.json')
    parser.add_argument('--model-file', type=Path)
    parser.add_argument('--native-mesh', type=Path, default=WORK / 'selected-digp-355-head.mesh')
    parser.add_argument('--dol', type=Path, default=Path('/mnt/w/Daywater/research-temp/nflstreet-animation-20261008/original.dol'))
    parser.add_argument('--report', type=Path, default=WORK / 'selected-head-geometry-verification.json')
    args = parser.parse_args()
    for p in (args.catalog, args.oracle_report, args.native_mesh, args.dol, args.report):
        p.resolve().relative_to(Path('/mnt/w/Daywater').resolve())
    if args.model_file is None:
        args.model_file = PRIVATE / 'unpacked/plaface/00012.bin'
    args.model_file.resolve().relative_to(Path('/mnt/w/Daywater/private').resolve())
    args.report.parent.mkdir(parents=True, exist_ok=True)

    protected = (args.dol, PRIVATE / 'assets/plaface.dat', PRIVATE / 'assets/plarosterhair.dat',
                 PRIVATE / 'unpacked/db_teams/00001.bin', args.model_file, args.catalog,
                 args.oracle_report, args.native_mesh)
    before = {str(p): sha256(p) for p in protected}
    if hashlib.sha1(args.dol.read_bytes()).hexdigest() != DOL_SHA1:
        raise ValueError('unexpected original DOL SHA1')
    oracle = json.loads(args.oracle_report.read_text())
    if oracle.get('status') != 'passed' or not oracle.get('protected_inputs_unchanged'):
        raise ValueError('original lookup oracle report is absent or not passed')
    catalog = json.loads(args.catalog.read_text())
    rows = [r for r in catalog['records']
            if r['team_id'] == args.team_id and r['player_id'] == args.player_id]
    if len(rows) != 1:
        raise ValueError(f'expected exactly one selected PLAY catalog row; found {len(rows)}')
    row = rows[0]
    head = row['head']
    if (head['match_count'] != 1 or head['index'] is None or
            not head['info_name_match'] or
            head['info_name'] != f"{row['name_for_model_keys']}2700.ea3"):
        raise ValueError('head resource does not have one checked index and matching Info name')
    model_index = int(args.model_file.stem)
    if model_index != head['index']:
        raise ValueError('model path index differs from original lookup result')

    sys_path = str(REPO / 'projects/nfl-street/reconstruction/native_assets')
    import sys
    sys.path.insert(0, sys_path)
    from player_assets import model
    from verify_models import triangles

    parsed = model(args.model_file)
    if not parsed or parsed['name'] != head['info_name']:
        raise ValueError('unpacked model Info name differs from the selected head lookup')
    expected, draws, triangle_count = triangles(args.model_file.read_bytes(), bind_pose=True)
    actual = args.native_mesh.read_bytes()
    if len(expected) != len(actual) or expected[:12] != actual[:12] or actual[:4] != b'NMSH':
        raise ValueError('native mesh header/count differs from independent Python geometry')
    max_error = 0.0
    expected_count = struct.unpack_from('<I', expected, 4)[0]
    for tri in range(expected_count):
        base = 12 + tri * 64
        if struct.unpack_from('<I', expected, base)[0] != struct.unpack_from('<I', actual, base)[0]:
            raise ValueError(f'material index differs at triangle {tri}')
        for component in range(15):
            offset = base + 4 + component * 4
            left = struct.unpack_from('<f', expected, offset)[0]
            right = struct.unpack_from('<f', actual, offset)[0]
            if not math.isfinite(left) or not math.isfinite(right):
                raise ValueError('non-finite stored-pose vertex value')
            max_error = max(max_error, abs(left-right))
    if max_error > 1e-6:
        raise ValueError(f'native geometry differs from independent Python reader: max error {max_error}')
    after = {str(p): sha256(p) for p in protected}
    if before != after:
        raise RuntimeError('a protected original input changed during verification')

    report = {
        'status': 'passed',
        'confidence': 'candidate',
        'identity_join_confidence': 'verified',
        'stored_pose_confidence': 'candidate',
        'scope': 'selected PLAY head lookup -> original face archive entry -> unpacked Info/geometry -> stored-pose native inspection',
        'game': 'NFL Street GameCube GNNE69-r0',
        'original_dol_sha1': DOL_SHA1,
        'team_id': args.team_id,
        'player_id': args.player_id,
        'play_row': row['play_row'],
        'head_key': row['head_key'],
        'original_lookup_index': head['index'],
        'matched_info_name': head['info_name'],
        'model_file_sha256': sha256(args.model_file),
        'native_mesh_sha256': sha256(args.native_mesh),
        'draw_rows': draws,
        'triangles': triangle_count,
        'native_vs_python_max_abs_float_error': max_error,
        'stored_scale_vectors': 'included by candidate R*S local matrix composition; C/Python agreement checks implementation consistency only',
        'limitations': [
            'The original DOL lookup result is checked by the separate controlled 801A8088 oracle.',
            'The C and Python geometry readers agree to tolerance; 801CA4E8 is an animation matrix builder, not an oracle for TMdl stored-scale composition.',
            'No original live matrix, selected actor attachment, external texture/palette, or complete body/hair assembly is verified.',
        ],
        'protected_sha256_before_after': {'before': before, 'after': after},
        'protected_inputs_unchanged': True,
    }
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({k: report[k] for k in ('status', 'confidence', 'team_id', 'player_id',
                                              'original_lookup_index', 'matched_info_name',
                                              'triangles', 'native_vs_python_max_abs_float_error')}))


if __name__ == '__main__':
    main()
