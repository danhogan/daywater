#!/usr/bin/env python3
"""Build and run the private entry8 field-navigation/queue lifecycle check."""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import re

from verify_controller_checks import PRIVATE, baseline_hashes, digest, private, run, windows


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--no-build', action='store_true')
    parser.add_argument('--output', type=Path,
        default=PRIVATE / 'research-temp/nflstreet-field-runtime-20261011')
    args = parser.parse_args()
    out = private(args.output)
    out.mkdir(parents=True, exist_ok=True)
    record_path = out / 'verification.json'
    record = {'status': 'running', 'boundary':
        'Native entry8 source event/tick host with supplied field data, profile predicate, '
        'port mapping, rate clock, archive availability and coordinator route. Effect queues '
        'are acknowledged by a synthetic host; no resource I/O, audio playback, route dispatch, '
        'rendering, original live screen or field-to-match commit is claimed.'}
    record_path.write_text(json.dumps(record, indent=2) + '\n')
    try:
        before = baseline_hashes()
        root = Path(__file__).resolve().parent
        if not args.no_build:
            run(['bash', str(root / 'build_windows_field_screen_runtime_check.sh'), str(out)],
                out / 'build.log')
        exe = private(out / 'field-screen-runtime-check.exe')
        source = private(PRIVATE / 'research-temp/nflstreet-original-menus-20261009/nested/uis_all/00000/00008.bin')
        template = private(PRIVATE / 'private/nflstreet/GNNE69-r0/unpacked/template/00002.bin')
        dol = private(PRIVATE / 'research-temp/nflstreet-native-windows-local/game/sys/main.dol')
        fonts = private(PRIVATE / 'private/nflstreet/GNNE69-r0/unpacked/uis_font')
        protected_inputs = (source, template, dol, *sorted(fonts.glob('*.bin')))
        input_hashes_before = {str(path): digest(path) for path in protected_inputs}
        hashes = {str(path): digest(path) for path in (exe, source, template, dol)}
        for path in sorted(fonts.glob('*.bin')):
            hashes[str(private(path))] = digest(path)
        text = run([str(exe), windows(source), windows(template), windows(dol),
                    windows(fonts), windows(out)], out / 'runtime-check.log')
        if 'field navigation: slots1-7, forward boundary and reverse step passed; long browse 128 steps passed' not in text:
            raise RuntimeError('Native checker did not return its navigation and long-browse summary')
        check_report = out / 'field-screen-runtime-report.txt'
        report_text = check_report.read_text(encoding='utf-8')
        required = ('status=passed', 'forward_cache_indices=1,2,3,4,5,6,7',
                    'forward_boundary_at_7=passed; reverse_to_6=passed',
                    'event7_transition=completed; linked_route_captured=5; link_notification_acknowledged_before_finish=1; FFFD_slot=0; FFFD_raw_key=7; match_creation=not_executed',
                    'runtime_finish_pretransition_rejection_atomic=passed; duplicate_finish_rejection_atomic=passed; owner_not_one_release_without_commit=passed',
                    'long_browse_steps=128; final_cache_index=0',
                    'queue_ack_mismatch_atomicity=passed; appended_prefix_preservation=passed')
        if any(field not in report_text for field in required):
            raise RuntimeError('Native runtime report is missing a required checkpoint')
        after = baseline_hashes()
        if before != after:
            raise RuntimeError('Original baseline changed during the native checker')
        input_hashes_after = {str(path): digest(path) for path in protected_inputs}
        if input_hashes_before != input_hashes_after:
            raise RuntimeError('Source, template, DOL, or font input changed during the native checker')
        for path in (root / 'field_screen_runtime_check.c',
                     root / 'field_screen_runtime.h', root / 'field_screen_probe.c',
                     root / 'build_windows_field_screen_runtime_check.sh', Path(__file__).resolve()):
            hashes[str(path)] = digest(path)
        record.update(status='passed', baselines=after, hashes=hashes,
                      protected_input_hashes_before=input_hashes_before,
                      protected_input_hashes_after=input_hashes_after,
                      results={'forward_cache_rows': 7, 'boundary_noop_at_row7': True,
                               'reverse_row_after_boundary': 6, 'long_browse_steps': 128,
                               'queue_ack_mismatch_atomicity': True,
                               'appended_prefix_preservation': True,
                               'link_notification_acked_before_fffd': True,
                               'fffd_capture_slot': 0, 'fffd_capture_raw_key': 7,
                               'finish_pretransition_rejection_atomic': True,
                               'finish_duplicate_rejection_atomic': True,
                               'finish_owner_not_one_release_without_commit': True,
                               'match_creation_executed': False},
                      native_report_sha256=digest(check_report),
                      limitations=['Effects are acknowledged by a synthetic queue consumer, not executed.',
                                   'Profile/status/port/archive/clock inputs are supplied host state.',
                                   'Field-to-match commit and original live rendering remain unverified.'])
        print('Field runtime navigation and 128-step queue lifecycle checks pass; private report ' + str(record_path))
    except Exception as error:
        record.update(status='failed', error=str(error))
        raise
    finally:
        record_path.write_text(json.dumps(record, indent=2) + '\n')


if __name__ == '__main__':
    main()
