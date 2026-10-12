#!/usr/bin/env python3
"""Rebuild and repeat the private selected PLAY body/head comparison."""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
from pathlib import Path

from verify_controller_checks import PRIVATE, BASELINES, digest, private, run, windows

HERE = Path(__file__).resolve().parent
DEFAULT_OUT = PRIVATE / 'research-temp/nflstreet-team-player-loader'
GAME = PRIVATE / 'private/nflstreet/GNNE69-r0'
CATALOG = DEFAULT_OUT / 'selected-resource-hash-catalog.json'
LOOKUP_REPORT = DEFAULT_OUT / 'original-lookup-verification.json'
BODY_REPORT = PRIVATE / 'research-temp/nflstreet-team-lineup-20261010/body-binding-verification.json'
DOL_SHA1 = '46e3de854998a0f66c9be8c72a1164d22cd29eb4'


def sha1(path: Path) -> str:
    return hashlib.sha1(path.read_bytes()).hexdigest()


def sections(data: bytes) -> dict[bytes, tuple[int, int]]:
    if len(data) < 16 or data[:4] != b'TMdl':
        raise ValueError('invalid TMdl input')
    count, directory = struct.unpack_from('>HH', data, 8)
    if directory + count * 16 > len(data):
        raise ValueError('TMdl section table exceeds file')
    found: dict[bytes, tuple[int, int]] = {}
    for i in range(count):
        off = directory + i * 16
        tag = data[off:off + 4]
        start, size = struct.unpack_from('>II', data, off + 4)
        if start + size > len(data) or tag in found:
            raise ValueError('invalid or duplicate TMdl section')
        found[tag] = start, size
    return found


def shared_skeleton_report(body_path: Path, head_path: Path) -> dict[str, object]:
    body, head = body_path.read_bytes(), head_path.read_bytes()
    bs, hs = sections(body), sections(head)
    bo, _ = bs[b'Skel']; ho, _ = hs[b'Skel']
    bcount, hcount = struct.unpack_from('>I', body, bo)[0], struct.unpack_from('>I', head, ho)[0]
    if bcount != hcount:
        raise ValueError('body/head stored bone counts differ')
    bt, br, bscale = struct.unpack_from('>III', body, bo + 8)
    ht, hr, hscale = struct.unpack_from('>III', head, ho + 8)
    bc, hc = bs[b'Clst'][0], hs[b'Clst'][0]
    b_rows, b_row_count = struct.unpack_from('>II', body, bc)
    h_rows, h_row_count = struct.unpack_from('>II', head, hc)
    body_rows = [tuple(body[b_rows + i * 4:b_rows + i * 4 + 2]) for i in range(b_row_count)]
    head_rows = [tuple(head[h_rows + i * 4:h_rows + i * 4 + 2]) for i in range(h_row_count)]
    def parent_bone(rows: list[tuple[int, int]], index: int) -> int:
        bone, parent = rows[index]
        if parent == 255:
            return 255
        if parent >= index:
            raise ValueError('cluster parent does not precede child')
        return rows[parent][0]
    head_bones = sorted({bone for bone, _ in head_rows})
    for index, (bone, _) in enumerate(head_rows):
        if (bone, parent_bone(head_rows, index)) not in {
            (b, parent_bone(body_rows, j)) for j, (b, _) in enumerate(body_rows)
        }:
            raise ValueError('head cluster parent-bone chain is absent from body cluster')
    different = []
    for bone in range(bcount):
        same = (body[bt + bone * 16:bt + (bone + 1) * 16] == head[ht + bone * 16:ht + (bone + 1) * 16] and
                body[br + bone * 6:br + (bone + 1) * 6] == head[hr + bone * 6:hr + (bone + 1) * 6] and
                body[bscale + bone * 12:bscale + (bone + 1) * 12] == head[hscale + bone * 12:hscale + (bone + 1) * 12])
        if not same:
            different.append(bone)
    if not body[bt:bt + 16] == head[ht:ht + 16] or not body[br:br + 6] == head[hr:hr + 6] or not body[bscale:bscale + 12] == head[hscale:hscale + 12]:
        raise ValueError('root stored transform differs')
    if any(bone in different for bone in head_bones):
        raise ValueError('a face-cluster-used stored bone transform differs')
    return {
        'body_bone_count': bcount,
        'head_bone_count': hcount,
        'face_cluster_bones': head_bones,
        'body_vs_face_full_skeleton_transform_mismatches': different,
        'root_transform_bytes_match': True,
        'face_cluster_parent_bones_exist_in_body': True,
        'face_cluster_transforms_match_body': True,
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=DEFAULT_OUT)
    args = parser.parse_args()
    out = private(args.output)
    out.mkdir(parents=True, exist_ok=True)

    report_path = out / 'selected-pair-preview-verification.json'
    record: dict[str, object] = {
        'status': 'running',
        'confidence': {
            'identity_to_head_resource': 'verified bounded original 801A8088 lookup and matching Info name',
            'tbyp_to_body_resource': 'verified bounded original body descriptor/attach path',
            'stored_pose_mesh_comparison': 'candidate; independent panels and C/Python consistency do not prove original matrix semantics',
            'whole_athlete_attachment': 'not verified',
        },
        'render_boundary': 'The comparison image has independent flat panels. The optional combined candidate uses the common stored model coordinates with no extra placement transform; original actor attachment remains unverified.',
    }
    report_path.write_text(json.dumps(record, indent=2) + '\n')
    try:
        sources = [
            HERE / 'team_player_selected_pair.h', HERE / 'team_player_selected_pair.c',
            HERE / 'team_player_selected_pair_preview.c', HERE / 'team_player_body_binding.h',
            HERE / 'team_player_body_binding.c', HERE / 'team_player_selected_head.h',
            HERE / 'team_player_selected_head.c', HERE / 'build_windows_team_player_selected_pair_preview.sh',
            HERE / 'verify_team_player_selected_pair.py',
            HERE.parent / 'native_assets/model.c', HERE.parent / 'native_assets/model.h',
            HERE.parent / 'native_assets/mesh_render.c', HERE.parent / 'native_assets/mesh_render.h',
            HERE.parent / 'native_assets/archive.c', HERE.parent / 'native_assets/archive.h',
            HERE.parent / 'native_assets/lzh1.c', HERE.parent / 'native_assets/texture.c',
            HERE.parent / 'native_assets/texture.h',
        ]
        private_inputs = [
            GAME / 'assets/plamodel.dat', GAME / 'assets/plaface.dat',
            GAME / 'unpacked/plamodel/00007.bin', GAME / 'unpacked/plaface/00012.bin',
            GAME / 'unpacked/db_teams/00001.bin', CATALOG, LOOKUP_REPORT, BODY_REPORT,
            BASELINES['dol'][0],
        ]
        for path in (*sources, *private_inputs):
            if not path.is_file():
                raise FileNotFoundError(path)
        catalog = json.loads(CATALOG.read_text())
        matches = [r for r in catalog['records'] if r.get('team_id') == 1 and r.get('player_id') == 355]
        if len(matches) != 1:
            raise ValueError('expected one team 1 / DIGP 355 catalog row')
        selected = matches[0]
        if selected.get('tbyp') != 2 or selected.get('head', {}).get('index') != 12:
            raise ValueError('selected PLAY TBYP/head index differs from this checked fixture')
        if sha1(BASELINES['dol'][0]) != DOL_SHA1 or catalog.get('source_dol_sha1') != DOL_SHA1:
            raise ValueError('unexpected original DOL identity')
        lookup = json.loads(LOOKUP_REPORT.read_text())
        body = json.loads(BODY_REPORT.read_text())
        if lookup.get('status') != 'passed' or body.get('status') != 'passed':
            raise ValueError('prerequisite original lookup or body-binding report did not pass')
        if 'key=2 variant=0 archive=7 Info=ThinPlayer2700.ea3' not in body.get('result', ''):
            raise ValueError('body-binding report does not establish the selected variant 0 entry')
        alignment = shared_skeleton_report(
            GAME / 'unpacked/plamodel/00007.bin', GAME / 'unpacked/plaface/00012.bin')
        if alignment['face_cluster_bones'] != [0, 1, 12, 13, 14, 15, 16] or alignment['body_vs_face_full_skeleton_transform_mismatches'] != [31, 58]:
            raise ValueError('selected model skeleton comparison differs from checked evidence')

        tracked = [*sources, *private_inputs]
        before = {str(p): digest(p) for p in tracked}
        build_log = out / 'selected-pair-build.log'
        run(['bash', str(HERE / 'build_windows_team_player_selected_pair_preview.sh'), str(out)], build_log)
        exe = out / 'team-player-selected-pair-preview.exe'
        exe_hash = digest(exe)
        outputs = []
        candidate_outputs = []
        for suffix in ('', '-repeat'):
            bmp = out / f'selected-team1-digp355-body-head-side-by-side{suffix}.bmp'
            candidate_bmp = out / f'selected-team1-digp355-combined-stored-pose-candidate{suffix}.bmp'
            log = out / f'selected-pair{suffix}.log'
            result = run([
                str(exe), windows(GAME / 'assets/plamodel.dat'), '2',
                windows(GAME / 'assets/plaface.dat'), '12', windows(bmp),
                windows(candidate_bmp),
            ], log)
            expected = ('head archive entry 12 Info=StewartKordell2700.ea3; '
                        'TBYP=2 -> body key=2 variant=0 archive entry=7 Info=ThinPlayer2700.ea3')
            if expected not in result or 'head triangles=770 body triangles=15524' not in result:
                raise RuntimeError('selected pair identity/geometry marker differs: ' + result)
            if 'shared head-used skeleton=yes' not in result:
                raise RuntimeError('shared stored-pose skeleton gate did not pass')
            outputs.append({'path': str(bmp), 'sha256': digest(bmp), 'log': str(log)})
            candidate_outputs.append({'path': str(candidate_bmp), 'sha256': digest(candidate_bmp), 'log': str(log)})
        if outputs[0]['sha256'] != outputs[1]['sha256']:
            raise RuntimeError('independent side-by-side captures differ')
        if candidate_outputs[0]['sha256'] != candidate_outputs[1]['sha256']:
            raise RuntimeError('independent combined candidate captures differ')
        after = {str(p): digest(p) for p in tracked}
        if before != after:
            raise RuntimeError('protected source/private input changed during verification')
        record.update({
            'status': 'passed',
            'game': 'NFL Street GameCube GNNE69-r0',
            'original_dol_sha1': DOL_SHA1,
            'selected_play': {
                'team_id': 1, 'digp': 355, 'name_key': selected['name_for_model_keys'],
                'tbyp': 2, 'head_key': selected['head_key'],
                'head_archive_index': 12, 'head_info_name': 'StewartKordell2700.ea3',
                'body_pool_key': 2, 'body_variant': 0,
                'body_archive_index': 7, 'body_info_name': 'ThinPlayer2700.ea3',
                'body_variant_selection': 'Explicit host inspection choice: variant 0. This report does not infer the runtime menu variant.',
            },
            'geometry': {'head_triangles': 770, 'body_triangles': 15524},
            'captures': outputs,
            'combined_stored_pose_candidate_captures': candidate_outputs,
            'shared_skeleton_check': 'Root translation/rotation/scale bytes match; all head-cluster-used bone transforms match the body; each head cluster parent row resolves to the same parent bone in the body cluster table.',
            'shared_skeleton_details': alignment,
            'executable_sha256': exe_hash,
            'prerequisites': {
                'original_head_lookup_report': str(LOOKUP_REPORT),
                'original_body_binding_report': str(BODY_REPORT),
            },
            'protected_sha256_before_after': {'before': before, 'after': after},
            'protected_inputs_unchanged': True,
        })
    except Exception as exc:
        record.update(status='failed', error=str(exc))
        raise
    finally:
        report_path.write_text(json.dumps(record, indent=2) + '\n')
    print('Selected PLAY body/head geometry comparison passes; report ' + str(report_path))


if __name__ == '__main__':
    main()
