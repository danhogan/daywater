"""Catalog PLAY-row portrait/head/hair resource indices into private W output.

The HSH1 lookup reproduces the name hash and unique-match selection used by
original 0x801A8088. This is an offline index check, not a live DOL call.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
from pathlib import Path

PRIVATE = Path('/mnt/w/Daywater/private/nflstreet/GNNE69-r0')
DEFAULT_OUTPUT = Path('/mnt/w/Daywater/research-temp/nflstreet-team-player-loader/selected-resource-hash-catalog.json')


def u16(data: bytes, at: int) -> int:
    return struct.unpack_from('>H', data, at)[0]


def u32(data: bytes, at: int) -> int:
    return struct.unpack_from('>I', data, at)[0]


def archive_hashes(path: Path) -> tuple[int, list[int]]:
    data = path.read_bytes()
    if data[:4] != b'TERF':
        raise ValueError(f'{path.name}: not TERF')
    at = u32(data, 4)
    hsh = None
    while at < len(data):
        if len(data) - at < 8:
            raise ValueError(f'{path.name}: truncated chunk header')
        size = u32(data, at + 4)
        if size < 8 or size > len(data) - at:
            raise ValueError(f'{path.name}: invalid chunk length')
        if data[at:at + 4] == b'HSH1':
            if hsh is not None or size < 24:
                raise ValueError(f'{path.name}: duplicate or short HSH1')
            algorithm, arg_bytes, hash_bytes, seed = struct.unpack_from('>4I', data, at + 8)
            count = u16(data, 14)
            if (algorithm, arg_bytes, hash_bytes) != (1, 4, 4) or 24 + count * 4 > size:
                raise ValueError(f'{path.name}: unsupported HSH1 layout')
            hsh = seed, list(struct.unpack_from('>' + 'I' * count, data, at + 24))
        at += size
    if hsh is None:
        raise ValueError(f'{path.name}: missing HSH1')
    return hsh


def lookup(name: str, seed: int, hashes: list[int]) -> dict:
    value = 0
    for char in name.upper().encode('ascii'):
        value = (value * seed + char) & 0xffffffff
    matches = [i for i, h in enumerate(hashes) if h == value]
    return {'hash': f'{value:08x}', 'index': matches[0] if len(matches) == 1 else None,
            'match_count': len(matches)}


def db_table(data: bytes, wanted: int) -> tuple[int, int, int, int, int]:
    if data[:4] != b'DB\0\x08' or u32(data, 8) != len(data):
        raise ValueError('invalid DB header')
    count = u32(data, 16)
    base = 28 + count * 8
    matches = [base + u32(data, 28 + i * 8) for i in range(count)
               if u32(data, 24 + i * 8) == wanted]
    if len(matches) != 1:
        raise ValueError(f'table {wanted:08x}: expected one match')
    at = matches[0]
    fields = data[at + 24]
    return at, fields, u32(data, at + 4), u16(data, at + 18), at + 36 + fields * 16


def db_fields(data: bytes, table_at: int, count: int) -> dict[int, tuple[int, int, int]]:
    return {u32(data, q + 8): (u32(data, q), u32(data, q + 4), u32(data, q + 12))
            for q in (table_at + 36 + i * 16 for i in range(count))}


def get_value(row: bytes, desc: tuple[int, int, int]) -> int:
    _kind, bit, width = desc
    value = 0
    for i in range(width):
        value = (value << 1) | ((row[(bit + i) // 8] >> (7 - (bit + i) % 8)) & 1)
    return value


def get_text(row: bytes, desc: tuple[int, int, int]) -> str:
    kind, bit, width = desc
    if kind != 0 or bit % 8 or width % 8:
        raise ValueError('expected byte-aligned fixed-width string field')
    return row[bit // 8:(bit + width) // 8].split(b'\0', 1)[0].decode('ascii')


def model_info(root: Path, category: str) -> dict[int, str]:
    from sys import path
    path.insert(0, str(Path(__file__).resolve().parents[1] / 'native_assets'))
    from player_assets import model
    result = {}
    for source in sorted((root / category).glob('*.bin')):
        parsed = model(source)
        if parsed:
            result[int(source.stem)] = parsed['name']
    return result


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--private-root', type=Path, default=PRIVATE)
    parser.add_argument('--output', type=Path, default=DEFAULT_OUTPUT)
    args = parser.parse_args()
    root, output = args.private_root.resolve(), args.output.resolve()
    root.relative_to(Path('/mnt/w/Daywater/private').resolve())
    output.relative_to(Path('/mnt/w/Daywater').resolve())

    unpacked = root / 'unpacked'
    face_hash_seed, face_hashes = archive_hashes(root / 'assets/plaface.dat')
    hair_hash_seed, hair_hashes = archive_hashes(root / 'assets/plarosterhair.dat')
    port_count = u16((root / 'assets/uis_port.dat').read_bytes(), 14)
    faces, hairs = model_info(unpacked, 'plaface'), model_info(unpacked, 'plarosterhair')
    rows_out = []
    for team_entry in range(1, 33):
        data = (unpacked / 'db_teams' / f'{team_entry:05}.bin').read_bytes()
        at, field_count, row_bytes, used, row_start = db_table(data, 0x59414c50)
        fields = db_fields(data, at, field_count)
        required = (0x44494754, 0x44494750, 0x414e4c50, 0x414e4650,
                    0x59545250, 0x50585350, 0x43465250, 0x54425950)
        if any(tag not in fields for tag in required):
            raise ValueError(f'db_teams/{team_entry:05}: missing required PLAY field')
        for row_index in range(used):
            row = data[row_start + row_index * row_bytes:row_start + (row_index + 1) * row_bytes]
            team_id, player_id = (get_value(row, fields[tag]) for tag in required[:2])
            if not team_id:
                continue
            surname = get_text(row, fields[0x414e4c50])
            given = get_text(row, fields[0x414e4650])
            name = ''.join(ch for ch in surname + given if ch.upper() in 'ABCDEFGHIJKLMNOPQRSTUVWXYZ')
            selector = (get_value(row, fields[0x43465250]) * 3 + 0x1ad +
                        (1 if get_value(row, fields[0x54425950]) == 1 else
                         2 if get_value(row, fields[0x54425950]) == 2 else 0)
                        if get_value(row, fields[0x59545250]) else
                        get_value(row, fields[0x50585350]))
            head_key = f'HEAD_MODEL_{name}_2700'
            hair_key = f'HAIR_MODEL_{name}_2700'
            hair_hat_key = f'HAIR_MODEL_{name}HAT_2700'
            head = lookup(head_key, face_hash_seed, face_hashes)
            hair = lookup(hair_key, hair_hash_seed, hair_hashes)
            hair_hat = lookup(hair_hat_key, hair_hash_seed, hair_hashes)
            for label, result, info, expected_info in (
                ('head', head, faces, f'{name}2700.ea3'),
                ('hair', hair, hairs, f'{name}Hair2700.ea3'),
                ('hair_hat', hair_hat, hairs, f'{name}HairHat2700.ea3')):
                idx = result['index']
                result['info_name'] = info.get(idx) if idx is not None else None
                result['info_name_match'] = (result['info_name'] or '').casefold() == expected_info.casefold()
            rows_out.append({
                'team_db_entry': team_entry, 'play_row': row_index,
                'team_id': team_id, 'player_id': player_id,
                'tbyp': get_value(row, fields[0x54425950]),
                'name_for_model_keys': name,
                'portrait_selector': selector, 'portrait_index': selector,
                'portrait_entry_exists': selector < port_count,
                'head_key': head_key, 'head': head,
                'head_runtime_result': head['index'] if head['index'] is not None else 0x28a,
                'head_runtime_result_is_fallback': head['index'] is None,
                'hair_key': hair_key, 'hair': hair,
                'hair_hat_key': hair_hat_key, 'hair_hat': hair_hat,
            })

    def count(field: str) -> dict[str, int]:
        return {
            'unique_hash_index': sum(r[field]['match_count'] == 1 for r in rows_out),
            'index_matches_info_name': sum(r[field]['match_count'] == 1 and r[field]['info_name_match'] for r in rows_out),
            'no_hash_match': sum(r[field]['match_count'] == 0 for r in rows_out),
            'ambiguous_hash': sum(r[field]['match_count'] > 1 for r in rows_out),
        }

    output.parent.mkdir(parents=True, exist_ok=True)
    report = {
        'scope': 'private original row/resource-name catalog; HSH1 index implementation, not live DOL call',
        'game': 'NFL Street GameCube GNNE69-r0',
        'source_dol_sha1': '46e3de854998a0f66c9be8c72a1164d22cd29eb4',
        'team_db_count': 32, 'play_row_count': len(rows_out),
        'lookup_algorithm': 'original 801A8088 -> 801A8B20 -> 801AA884 HSH1 algorithm 1; uppercase polynomial hash, unique match only',
        'archive_sha256': {f'{name}.dat': hashlib.sha256((root / 'assets' / f'{name}.dat').read_bytes()).hexdigest()
                           for name in ('plaface', 'plarosterhair', 'uis_port')},
        'counts': {'head': count('head'), 'hair': count('hair'), 'hair_hat': count('hair_hat'),
                   'head_fallback_0x28a': sum(r['head_runtime_result_is_fallback'] for r in rows_out),
                   'portrait_direct_index_exists': sum(r['portrait_entry_exists'] for r in rows_out),
                   'portrait_index_out_of_range': sum(not r['portrait_entry_exists'] for r in rows_out)},
        'records': rows_out,
    }
    output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({'output': str(output), 'team_db_count': 32,
                      'play_row_count': report['play_row_count'], 'counts': report['counts']}))


if __name__ == '__main__':
    main()
