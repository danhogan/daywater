#!/usr/bin/env python3
"""Verify selected body/head geometry coverage for every regular-team PLAY row."""
from __future__ import annotations

import argparse
import csv
import hashlib
import json
from pathlib import Path

from verify_controller_checks import PRIVATE, BASELINES, digest, private, run, windows

HERE = Path(__file__).resolve().parent
GAME = PRIVATE / 'private/nflstreet/GNNE69-r0'
PLAYER_OUT = PRIVATE / 'research-temp/nflstreet-team-player-loader'
DEFAULT_OUT = PLAYER_OUT / 'all-rows'
CATALOG = PLAYER_OUT / 'selected-resource-hash-catalog.json'
LOOKUP = PLAYER_OUT / 'original-lookup-verification.json'
RESOLVER = PRIVATE / 'research-temp/nflstreet-team-resolve-20261011/verification.json'
BODY = PRIVATE / 'research-temp/nflstreet-team-lineup-20261010/body-binding-verification.json'
DOL_SHA1 = '46e3de854998a0f66c9be8c72a1164d22cd29eb4'
FIELDS = ('team_id', 'digp', 'tbyp', 'body_key', 'body_key_supported',
          'head_index', 'body_info', 'head_info', 'body_triangles',
          'head_triangles', 'shared_pose', 'status')


def sha1(path: Path) -> str:
    return hashlib.sha1(path.read_bytes()).hexdigest()


def read_rows(path: Path) -> list[dict[str, str]]:
    with path.open(newline='') as f:
        reader = csv.DictReader(f, delimiter='\t')
        if tuple(reader.fieldnames or ()) != FIELDS:
            raise ValueError('all-row output has an unexpected TSV schema')
        return list(reader)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=DEFAULT_OUT)
    args = parser.parse_args()
    out = private(args.output)
    out.mkdir(parents=True, exist_ok=True)
    report_path = out / 'all-play-pairs-verification.json'
    report: dict[str, object] = {
        'status': 'running',
        'confidence': 'verified for bounded selected resource resolution and native geometry decoding; stored-pose composition remains candidate',
        'scope': 'All nonzero-DIGT PLAY rows enumerated across db_teams/00001..00032; host explicitly selects PLYRBODY variant 0.',
        'render_boundary': 'Only one representative per body key is rendered. Side-by-side and shared-skeleton-gated combined candidate images are native inspection output, not original pixels or full runtime actors.',
    }
    report_path.write_text(json.dumps(report, indent=2) + '\n')
    try:
        prerequisite_reports = [LOOKUP, RESOLVER, BODY]
        lookup_report, resolver_report, body_report = [json.loads(p.read_text()) for p in prerequisite_reports]
        if lookup_report.get('status') != 'passed' or resolver_report.get('status') != 'passed' or body_report.get('status') != 'passed':
            raise ValueError('a prerequisite original lookup/resolver/body-binding report did not pass')
        if resolver_report.get('actual_play_rows') != 402 or resolver_report.get('decoded_head_payloads') != 402 or resolver_report.get('original_lookup_queries') != 2416:
            raise ValueError('resolver report does not prove the expected 402 rows and 2,416 original queries')
        if sha1(BASELINES['dol'][0]) != DOL_SHA1:
            raise ValueError('original DOL SHA1 mismatch')
        catalog = json.loads(CATALOG.read_text())
        expected_records = catalog.get('records', [])
        if len(expected_records) != 402 or catalog.get('source_dol_sha1') != DOL_SHA1:
            raise ValueError('selected PLAY resource catalog does not match the 402-row fixture')
        expected = {(int(r['team_id']), int(r['player_id'])): r for r in expected_records}
        if len(expected) != 402:
            raise ValueError('catalog contains duplicate team/DIGP identities')

        sources = [
            HERE / name for name in (
                'team_player_all_rows_pair_check.c', 'team_player_resolve.c', 'team_player_resolve.h',
                'team_player_manifest.c', 'team_player_manifest.h', 'team_identity.c',
                'team_catalog.c', 'team_catalog.h', 'team_player_body_binding.c',
                'team_player_body_binding.h', 'team_player_selected_pair.c',
                'team_player_selected_pair.h', 'team_player_selected_head.c',
                'team_player_selected_head.h', 'build_windows_team_player_all_rows_pair_check.sh',
                'verify_team_player_all_rows_pairs.py',
            )
        ] + [HERE.parent / 'native_assets' / name for name in (
            'model.c', 'model.h', 'mesh_render.c', 'mesh_render.h', 'archive.c',
            'archive.h', 'lzh1.c', 'lzh1.h', 'texture.c', 'texture.h',
        )]
        private_inputs = [
            *[GAME / f'unpacked/db_teams/{i:05}.bin' for i in range(1, 33)],
            GAME / 'assets/plamodel.dat', GAME / 'assets/plaface.dat',
            GAME / 'assets/plarosterhair.dat',
            *[GAME / f'unpacked/plamodel/{i:05}.bin' for i in range(10)],
            CATALOG, LOOKUP, RESOLVER, BODY, BASELINES['dol'][0],
        ]
        for p in (*sources, *private_inputs):
            if not p.is_file():
                raise FileNotFoundError(p)
        before = {str(p): digest(p) for p in (*sources, *private_inputs)}

        build_log = out / 'build.log'
        run(['bash', str(HERE / 'build_windows_team_player_all_rows_pair_check.sh'), str(out)], build_log)
        exe = out / 'team-player-all-rows-pair-check.exe'
        exe_hash = digest(exe)
        outputs = []
        summaries = []
        for suffix in ('', '-repeat'):
            tsv = out / f'all-play-pairs{suffix}.tsv'
            prefix = windows(out / f'representative{suffix}')
            log = out / f'all-play-pairs{suffix}.log'
            result = run([
                str(exe), windows(GAME / 'unpacked/db_teams'),
                windows(GAME / 'assets/plamodel.dat'), windows(GAME / 'assets/plaface.dat'),
                windows(GAME / 'assets/plarosterhair.dat'), windows(tsv), prefix,
            ], log)
            if 'rows=402' not in result:
                raise RuntimeError('all-row native checker did not enumerate 402 records: ' + result)
            rows = read_rows(tsv)
            if len(rows) != 402:
                raise RuntimeError(f'expected 402 native rows, found {len(rows)}')
            actual: dict[tuple[int, int], dict[str, str]] = {}
            for row in rows:
                key = (int(row['team_id']), int(row['digp']))
                if key in actual:
                    raise RuntimeError(f'duplicate native PLAY identity {key}')
                actual[key] = row
                want = expected.get(key)
                if want is None:
                    raise RuntimeError(f'native row absent from resource catalog: {key}')
                if int(row['tbyp']) != int(want['tbyp']):
                    raise RuntimeError(f'TBYP differs from catalog for {key}')
                expected_body = int(want['tbyp']) if int(want['tbyp']) <= 2 else (2 if int(want['tbyp']) == 7 else -1)
                if expected_body < 0 or int(row['body_key']) != expected_body or row['body_key_supported'] != '1':
                    raise RuntimeError(f'body key mapping unsupported or mismatched for {key}')
                if int(row['head_index']) != int(want['head_runtime_result']):
                    raise RuntimeError(f'head index differs from original resolver catalog for {key}')
                expected_head = want.get('head', {}).get('info_name')
                if expected_head and row['head_info'].casefold() != expected_head.casefold():
                    raise RuntimeError(f'head Info name differs from catalog for {key}')
                if int(row['shared_pose']) and row['status'] == 'candidate_pair_ready':
                    if not row['body_info'] or not row['head_info'] or int(row['body_triangles']) <= 0 or int(row['head_triangles']) <= 0:
                        raise RuntimeError(f'pair status lacks decoded geometry for {key}')
            if set(actual) != set(expected):
                missing = sorted(set(expected) - set(actual))[:10]
                raise RuntimeError(f'native PLAY identities differ from catalog; missing={missing}')

            unsupported = [
                {'team_id': int(r['team_id']), 'digp': int(r['digp']),
                 'tbyp': int(r['tbyp']), 'body_key': int(r['body_key']),
                 'head_index': int(r['head_index']), 'status': r['status']}
                for r in rows if r['status'] != 'candidate_pair_ready'
            ]
            counts: dict[str, int] = {}
            by_body: dict[str, int] = {}
            for r in rows:
                counts[r['status']] = counts.get(r['status'], 0) + 1
                by_body[r['body_key']] = by_body.get(r['body_key'], 0) + 1
            image_hashes: dict[str, dict[str, str]] = {}
            for body_key in range(3):
                for suffix2 in ('side-by-side', 'combined-candidate'):
                    image = out / f'representative{suffix}-body-key{body_key}-{suffix2}.bmp'
                    if image.exists():
                        image_hashes[f'body-key-{body_key}-{suffix2}'] = {
                            'path': str(image), 'sha256': digest(image)}
            outputs.append({'tsv': str(tsv), 'tsv_sha256': digest(tsv),
                            'log': str(log), 'image_sha256': image_hashes})
            summaries.append({'row_count': len(rows), 'status_counts': counts,
                              'body_key_counts': by_body, 'unsupported_cases': unsupported,
                              'summary_line': result.strip()})
        if outputs[0]['tsv_sha256'] != outputs[1]['tsv_sha256']:
            raise RuntimeError('independent all-row TSV outputs differ')
        if outputs[0]['image_sha256'].keys() != outputs[1]['image_sha256'].keys():
            raise RuntimeError('representative image sets differ between runs')
        for image in outputs[0]['image_sha256']:
            if outputs[0]['image_sha256'][image]['sha256'] != outputs[1]['image_sha256'][image]['sha256']:
                raise RuntimeError('representative image outputs differ')
        after = {str(p): digest(p) for p in (*sources, *private_inputs)}
        if before != after:
            raise RuntimeError('a protected source or private input changed during verification')
        report.update({
            'status': 'passed',
            'game': 'NFL Street GameCube GNNE69-r0',
            'original_dol_sha1': DOL_SHA1,
            'prerequisites': {'original_head_resolver_rows': 402,
                              'original_lookup_queries': 2416,
                              'original_body_binding_report': str(BODY)},
            'native_checker_sha256': exe_hash,
            'runs': summaries,
            'repeated_outputs': outputs,
            'representative_images': 'At most one pair per body key per run; renders are optional geometry inspections only.',
            'protected_sha256_before_after': {'before': before, 'after': after},
            'protected_inputs_unchanged': True,
        })
    except Exception as exc:
        report.update(status='failed', error=str(exc))
        raise
    finally:
        report_path.write_text(json.dumps(report, indent=2) + '\n')
    print('All regular-team PLAY body/head geometry checks pass; report ' + str(report_path))


if __name__ == '__main__':
    main()
