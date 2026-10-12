"""Extract a bounded private movement fixture from original live JSONL calls.

Writes raw actor data to the caller's private W: destination; never Git.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path


def prepare(trace: Path, directions: str, destination: Path) -> dict:
    rows = [json.loads(line) for line in trace.read_bytes().splitlines()]
    def event(name: str, index: int) -> dict:
        found = [r for r in rows if r.get('event') == name and r.get('index') == index]
        if len(found) != 1:
            raise ValueError(f'expected one {name} {index}')
        return found[0]
    done = [r for r in rows if r.get('event') == 'done']
    if len(done) != 1 or done[0]['captured'] != done[0]['requested'] or done[0]['captured'] != len(directions):
        raise ValueError('incomplete capture or direction count mismatch')
    first = event('preparation', 0)
    reset = [r for r in rows if r.get('event') == 'reset_entry' and r.get('after_index') == 0]
    if len(reset) != 1:
        raise ValueError('expected captured reset constants')
    data = bytearray(b'NMS1' + struct.pack('>I', len(directions)))
    data += bytes.fromhex(first['actor_header_hex'] + first['actor_bb4_hex'] + reset[0]['actor_bbc_hex'] + reset[0]['actor_408_hex'] + first['actor_inputs_hex'] + first['state_hex'])
    if len(data) != 152:
        raise ValueError('wrong seed lengths')
    for index, direction in enumerate(directions):
        prep, end = event('preparation', index), event('movement_exit', index)
        entry = event('movement_entry', index)
        if prep['pc'] != '0x800F705C' or entry['pc'] != '0x80036B28' or end['pc'] != '0x80036E58':
            raise ValueError('unexpected original boundary')
        if prep['actor_header_hex'] != first['actor_header_hex'] or prep['actor_bb4_hex'] != first['actor_bb4_hex']:
            raise ValueError('changing actor constants')
        vector = bytearray(32)
        if direction not in 'RFL':
            raise ValueError('unsupported direction')
        struct.pack_into('>f', vector, 4 if direction in 'RL' else 8, -1.0 if direction == 'L' else 1.0)
        data += vector + bytes.fromhex(prep['actor_inputs_hex'] + end['state_hex'])
    if len(data) != 152 + len(directions)*164:
        raise ValueError('wrong checkpoint lengths')
    destination.write_bytes(data)
    return {'trace': str(trace), 'trace_sha256': hashlib.sha256(trace.read_bytes()).hexdigest(), 'directions': directions, 'fixture': str(destination), 'fixture_sha256': hashlib.sha256(data).hexdigest()}


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('trace', type=Path)
    p.add_argument('directions')
    p.add_argument('destination', type=Path)
    args = p.parse_args()
    print(json.dumps(prepare(args.trace, args.directions, args.destination), indent=2))
