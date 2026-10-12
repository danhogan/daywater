"""Compare native timing outputs with fresh original-game before/after calls.

The bounded readiness implementation rejects other callback/control families.
Those calls are counted explicitly, never counted as matches. Initial state,
program replacement, and elapsed-time inputs still come from the reference.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
import subprocess
from pathlib import Path
from verify_handle_writer import check_dol
from verify_target_selection import DOL_SHA1


def main() -> None:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--trace', type=Path, required=True)
    p.add_argument('--dol', type=Path, required=True)
    p.add_argument('--executable', type=Path, required=True)
    p.add_argument('--windows', action='store_true')
    a = p.parse_args()
    check_dol(a.dol)
    raw = a.trace.read_bytes()
    rows = [json.loads(line) for line in raw.splitlines()]
    if rows[0]['event'] != 'ready' or rows[-1]['event'] != 'done' or rows[-1]['incomplete_timing']:
        raise ValueError('trace lacks complete timing boundaries')
    if (rows[0]['dol_sha1'] != DOL_SHA1 or rows[0]['dolphin'] != '2603a' or
            rows[0]['savestate_sha256'] != 'e2faeb8f9481096ff87ae261c8c2943452fbc023cd27510f641808e307603d93'):
        raise ValueError('trace reference identity differs')
    prefix = [str(a.executable.resolve())]
    if a.windows:
        exe = subprocess.run(['wslpath', '-w', str(a.executable.resolve())],
                             check=True, capture_output=True, text=True).stdout.strip()
        prefix = ['cmd.exe', '/c', exe]
    matched = rejected = commands = waits = terminal = longest = 0
    emitted = {}
    carried = {}
    active = None
    inside = []
    for row in rows:
        kind = row['event']
        if kind == 'timing_entry':
            if active is not None:
                raise ValueError('nested timing calls')
            active = row
            inside = []
        elif active is not None and kind != 'timing_return':
            inside.append(row)
        elif kind == 'timing_return':
            if active is None or row['stream'] != active['stream']:
                raise ValueError('unpaired timing return')
            calls = [r for r in inside if r['event'] == 'timing_command']
            returns = [r for r in inside if r['event'] == 'timing_callback_return']
            if len(calls) != len(returns) or any(c['command'] != r['command'] for c,r in zip(calls,returns)):
                raise ValueError('callback boundary mismatch')
            if any(r['result'] != 1 for r in returns):
                # Callback context rejection is outside the accepted branch.
                unsupported_callback = True
            else:
                unsupported_callback = False
            if len(bytes.fromhex(active['state_hex'])) != 64 or len(bytes.fromhex(active['data_hex'])) != 256:
                raise ValueError('capture span mismatch')
            if active['constants_hex'] != '000000000000000043300000800000003d8000000000000043300000000000003d800000000000003f800000000000003f800000':
                raise ValueError('original timing constants differ')
            stream = row['stream']
            previous = carried.get(stream)
            continuity = previous is not None and previous[0] == active['state_hex'] and previous[1] == active['data_hex']
            seed = previous[0] if continuity else active['state_hex']
            delta = struct.unpack('>d', bytes.fromhex(active['delta_fpr_hex']))[0]
            stores = [r for r in inside if r['event'] == 'event_flag_command']
            flags = stores[0]['before'] if stores else 0
            result = subprocess.run(prefix + [seed, active['data_hex'], repr(delta), hex(flags)],
                                    capture_output=True, text=True, timeout=15,
                                    cwd=a.executable.resolve().parent)
            if result.returncode == 3:
                rejected += 1
                carried.pop(stream, None)
            elif result.returncode:
                raise ValueError(f'native replay failed: {result.returncode} {result.stderr}')
            else:
                output = result.stdout.strip().split()
                native_commands = [int(x,16) for x in output[3:]]
                expected_commands = [r['command'] for r in calls]
                if unsupported_callback or output[0] != row['state_hex'] or int(output[1]) != row['result'] or native_commands != expected_commands:
                    raise ValueError(f'timing output differs at frame {row["frames"]}, stream {stream:#x}')
                if stores:
                    writes = [r for r in inside if r['event'] == 'event_flag_written']
                    if len(writes) != len(stores) or int(output[2],16) != writes[-1]['after']:
                        raise ValueError('emitted readiness flag effect differs')
                streak = previous[2]+1 if continuity else 1
                longest = max(longest,streak)
                carried[stream] = (output[0],active['data_hex'],streak)
                matched += 1
                waits += not native_commands
                terminal += bool(row['result'])
                commands += len(native_commands)
                for command in native_commands:
                    emitted[f'{command:04x}'] = emitted.get(f'{command:04x}',0)+1
            active = None
    if active is not None or matched + rejected != rows[-1]['timing_updates'] or not matched or not commands:
        raise ValueError('incomplete or command-free timing verification')
    print(json.dumps({'trace': a.trace.name, 'trace_sha256': hashlib.sha256(raw).hexdigest(),
                      'matched_updates': matched, 'unsupported_updates': rejected,
                      'command_free_updates': waits, 'terminal_updates': terminal,
                      'emitted_commands': emitted, 'longest_carried_updates': longest}, sort_keys=True))


if __name__ == '__main__':
    main()
