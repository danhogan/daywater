#!/usr/bin/env python3
"""Compare direct original and native team event 7 source snapshots."""
from __future__ import annotations

import hashlib
import json
from pathlib import Path

from verify_controller_checks import BASELINES, PRIVATE, baseline_hashes, digest, private


def source_delta(before: bytes, after: bytes) -> list[dict[str, object]]:
    if len(before) != len(after):
        raise ValueError('source snapshot sizes differ')
    return [
        {'offset': f'0x{i:x}', 'before': f'{a:02x}', 'after': f'{b:02x}'}
        for i, (a, b) in enumerate(zip(before, after)) if a != b
    ]


def compare_pair(original_pre: Path, original_post: Path,
                 native_pre: Path, native_post: Path) -> dict[str, object]:
    op, oq, np, nq = (p.read_bytes() for p in
                      (original_pre, original_post, native_pre, native_post))
    if len({len(op), len(oq), len(np), len(nq)}) != 1:
        raise ValueError('original/native source snapshots have different sizes')
    original_delta = source_delta(op, oq)
    native_delta = source_delta(np, nq)
    return {
        'source_size': len(op),
        'original_pre_sha256': hashlib.sha256(op).hexdigest(),
        'native_pre_sha256': hashlib.sha256(np).hexdigest(),
        'original_post_sha256': hashlib.sha256(oq).hexdigest(),
        'native_post_sha256': hashlib.sha256(nq).hexdigest(),
        'pre_bytes_equal': op == np,
        'post_bytes_equal': oq == nq,
        'pre_diff_byte_count': sum(a != b for a, b in zip(op, np)),
        'post_diff_byte_count': sum(a != b for a, b in zip(oq, nq)),
        'original_event_delta': original_delta,
        'native_event_delta': native_delta,
        'event_delta_equal': original_delta == native_delta,
    }


def main() -> None:
    out = private(PRIVATE / 'research-temp/nflstreet-team-dispatcher-probe-20261011')
    out.mkdir(parents=True, exist_ok=True)
    original_mode0 = private(PRIVATE / 'research-temp/nflstreet-team-dispatcher-probe-20261011-materialized/original-mode0')
    original_mode6 = private(PRIVATE / 'research-temp/nflstreet-team-dispatcher-probe-20261011-materialized/original-run')
    native = out
    source = private(PRIVATE / 'research-temp/nflstreet-original-menus-20261009/nested/uis_all/00000/00007.bin')
    dol = BASELINES['dol'][0]
    dll = BASELINES['original_dll'][0]
    snapshot_paths = [PRIVATE / f'research-temp/position-neutral-{c}.mem1' for c in (14, 15, 16)]
    fixture = private(PRIVATE / 'research-temp/nflstreet-team-dispatcher-probe-20261011')
    materialized = private(PRIVATE / 'research-temp/nflstreet-team-dispatcher-probe-20261011-materialized/context14.mem1')
    checker = private(PRIVATE / 'research-temp/nflstreet-team-input-20261011/team-manager-original-check.exe')
    mode0_log = original_mode0 / 'run.log'
    mode6_log = original_mode6 / 'run.log'
    native_manifest = fixture / 'dispatcher-probe.json'
    inputs = [source, dol, dll, *snapshot_paths, materialized, checker,
              fixture / 'native-pre-input-source.bin', fixture / 'fixture.json',
              native_manifest, mode0_log, mode6_log,
              *(fixture / f'font-{i:02}.bin' for i in range(4))]
    before = baseline_hashes()
    input_hashes = {str(p): digest(p) for p in inputs}

    pairs: dict[str, object] = {}
    for mode in (0, 6):
        original_dir = original_mode0 if mode == 0 else original_mode6
        original_stem = f'original-mode{mode}-context14'
        pairs[str(mode)] = compare_pair(
            original_dir / f'{original_stem}-pre-event7-source.bin',
            original_dir / f'{original_stem}-post-event7-source.bin',
            native / f'dispatcher-mode{mode}-pre-event7-source.bin',
            native / f'dispatcher-mode{mode}-post-event7-source.bin',
        )

    trace0 = mode0_log.read_text(encoding='utf-8', errors='replace')
    trace6 = mode6_log.read_text(encoding='utf-8', errors='replace')
    native_runs = json.loads(native_manifest.read_text(encoding='utf-8'))['mode_runs']
    evidence_checks = {
        'original_mode0_no_transition': 'stored-mode=00000000' in trace0 and '80137B84 invoked' not in trace0,
        'original_mode6_transition': ('stored-mode=00000006' in trace6 and
                                      '80137B84 invoked ticks=500' in trace6 and
                                      'audio-enabled=1' in trace6),
        'original_vm_context': all('VM-stack=817f2380 manager-row(group/screen/link)=0/7/ffffffff' in log
                                   for log in (trace0, trace6)),
        'native_mode0_state': (native_runs[0]['input_returned'] and
                               native_runs[0]['effect_delta']['transition'] == 0 and
                               native_runs[0]['source_delta_bytes'] == 6 and
                               native_runs[0]['callback_state'] == [0, 0]),
        'native_mode6_state': (native_runs[1]['input_returned'] and
                               native_runs[1]['effect_delta']['transition'] == 1 and
                               native_runs[1]['source_delta_bytes'] == 6 and
                               native_runs[1]['callback_state'] == [0, 0]),
        'native_mode6_transition_inputs': (native_runs[1]['transition']['clock'] == 0x1a0d and
                                           native_runs[1]['transition']['audio_enabled'] == 1),
    }
    trace_evidence_ok = all(evidence_checks.values())

    result = {
        'format': 'daywater-team-dispatcher-event7-comparison-v1',
        'status': 'partial',
        'boundary': 'Original D094C/D399C source event7 versus native source input on a byte-identical native-prepared source after focus nodes21 then22. Original callback 80137B84 receives 500 and audio-enabled=1 in mode6; native mode6 supplies transition clock 6669, audio-enabled=1, and broadcast context0. Prepared source/fonts/line plans are explicitly materialized into a private saved-RAM copy; original D0264 registration executes, while CF8C8/CE994 construction is skipped. This is not normal full-game startup.',
        'vm_context': {'current_screen': 7, 'linked_screen': 4294967295, 'stack_base': '0x817f2380', 'port': 0, 'event': 7, 'float_input': 1.0},
        'modes': pairs,
        'original_mode0_trace': {'mode_zero_path': True, 'transition_called': False},
        'original_mode6_trace': {'stored_mode': 6, 'transition_ticks': 500, 'audio_enabled': 1,
                                 'clock': 0x1a0d, 'broadcast_arguments': [0, 0x80000003, 0, 0]},
        'original_vm_context': {'current_screen': 7, 'linked_screen': 4294967295,
                                'stack_base': '0x817f2380', 'row_group_screen_link': [0, 7, 4294967295]},
        'native_transition_boundary': native_runs[1]['transition'],
        'native_audio_note': 'audio_enabled is a supplied/requested transition argument, not evidence that hardware played sound.',
        'evidence_checks': evidence_checks,
        'mode0_mode6_original_event_delta_equal': pairs['0']['original_event_delta'] == pairs['6']['original_event_delta'],
        'mode0_mode6_native_event_delta_equal': pairs['0']['native_event_delta'] == pairs['6']['native_event_delta'],
        'protected_inputs_sha256': input_hashes,
        'baseline_sha1_sha256': before,
        'claim': 'Full pre- and post-event source bytes match exactly in both modes. The six source-byte mutations match exactly. Mode0 returns through the no-transition path; mode6 makes one 500-tick transition request with the original clock/audio inputs supplied. No source normalization is applied.',
    }
    after = baseline_hashes()
    result['protected_inputs_unchanged'] = all(digest(p) == h for p, h in
                                               ((Path(k), v) for k, v in input_hashes.items()))
    result['baseline_hashes_unchanged'] = before == after
    result['status'] = 'verified' if (
        result['protected_inputs_unchanged'] and result['baseline_hashes_unchanged'] and trace_evidence_ok and
        all(pairs[str(m)]['event_delta_equal'] for m in (0, 6)) and
        all(pairs[str(m)]['pre_bytes_equal'] and pairs[str(m)]['post_bytes_equal']
            for m in (0, 6))
    ) else 'failed'
    report = out / 'dispatcher-comparison-verification.json'
    report.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print(f"Event 7 comparison: {result['status']}; report {report}")
    if result['status'] != 'verified':
        raise SystemExit(1)


if __name__ == '__main__':
    main()
