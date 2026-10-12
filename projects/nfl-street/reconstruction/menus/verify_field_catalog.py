#!/usr/bin/env python3
"""Repeat the private field catalog/original-reader gate and preserve provenance."""
import argparse
import json
from pathlib import Path

from verify_controller_checks import PRIVATE, baseline_hashes, digest, private, run, windows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--no-build', action='store_true')
    args = parser.parse_args()
    root = Path(__file__).resolve().parent
    out = private(PRIVATE / 'research-temp/nflstreet-field-catalog-20261011')
    out.mkdir(parents=True, exist_ok=True)
    report = out / 'verification.json'
    record = {'status': 'running', 'boundary':
              'Actual consumed RVNE numeric/text fields compared to original primitive readers; '
              'native-built records compared to original cache-hit reader; original uncached key '
              'selection and signed numeric labels checked separately. Actual IVID parsing is '
              'compared to the original no-profile C9 branch with supplied iterator/profile '
              'responses. Other RVNE fields, query ownership, general unlocks and complete '
              'cache-miss initialization excluded. Record text42/C9 remain caller inputs.'}
    report.write_text(json.dumps(record, indent=2) + '\n')
    try:
        before = baseline_hashes()
        template = private(PRIVATE / 'private/nflstreet/GNNE69-r0/unpacked/template/00002.bin')
        template_hash = digest(template)
        if not args.no_build:
            run(['bash', str(root / 'build_windows_field_catalog_check.sh'), str(out)], out / 'build.log')
            run(['bash', str(root / 'build_windows_field_availability_check.sh'), str(out)], out / 'availability-build.log')
        exe = out / 'field-catalog-original-check.exe'
        command = [str(exe), windows(PRIVATE / 'research-temp/nflstreet-native-windows-local/gGNNE69_recomp.dll'),
                   windows(template), windows(PRIVATE / 'research-temp/nflstreet-native-windows-local/game/sys/main.dol')]
        logs = []
        for iteration in range(2):
            log = out / ('check-' + str(iteration) + '.log')
            text = run(command, log)
            if ('40 numeric and 24 text fields' not in text or 'matched for 8 raw DIVE keys' not in text
                    or 'native slot order for eight slots' not in text
                    or 'numeric field text matches13' not in text):
                raise RuntimeError('Field original-reader checks did not complete')
            logs.append(digest(log))
        if logs[0] != logs[1]:
            raise RuntimeError('Fresh field-check traces differ')
        availability_exe = out / 'field-availability-original-check.exe'
        availability_logs = []
        for iteration in range(2):
            log = out / ('availability-' + str(iteration) + '.log')
            text = run([str(availability_exe), command[1], windows(template)], log)
            if 'matches native parsing for 8 actual field IDs' not in text:
                raise RuntimeError('Original no-profile availability branch did not complete')
            availability_logs.append(digest(log))
        if availability_logs[0] != availability_logs[1]:
            raise RuntimeError('Fresh availability traces differ')
        if baseline_hashes() != before or digest(template) != template_hash:
            raise RuntimeError('Protected original/template baseline changed')
        record.update(status='passed', numeric_fields=40, text_fields=24, cached_records=8,
                      numeric_label_cases=13, original_slot_keys=8,
                      no_profile_availability_cases=8,
                      availability_boundary='Original C9 branch; actual IVID iterator results and no-profile returns supplied. Other cache-fill services excluded.',
                      availability_executable_sha256=digest(availability_exe),
                      repeated_availability_log_sha256=availability_logs,
                      baseline=before, template_sha256=template_hash,
                      executable_sha256=digest(exe), repeated_log_sha256=logs)
    except Exception as exc:
        record.update(status='failed', error=str(exc))
        raise
    finally:
        report.write_text(json.dumps(record, indent=2) + '\n')
    print('Field extraction/cache checks pass; private report ' + str(report))


if __name__ == '__main__':
    main()
