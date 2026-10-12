#!/usr/bin/env python3
"""Overlay an explicit native-prepared UI fixture onto a private saved RAM copy.

This supplies source/font/line ownership for controlled original comparisons.
It neither reconstructs original startup nor changes the input snapshot.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

PRIVATE = Path('/mnt/w/Daywater')
BASE = 0x80000000
RAM_SIZE = 24 * 1024 * 1024

def private(path):
    path = Path(path).resolve()
    if not path.is_relative_to(PRIVATE):
        raise ValueError('Fixture inputs and outputs must remain private on W:')
    return path

def digest(data):
    return hashlib.sha256(data).hexdigest()

def number(value):
    return int(value, 0) if isinstance(value, str) else int(value)

def materialize(snapshot, fixture):
    snapshot, fixture = private(snapshot), private(fixture)
    original = snapshot.read_bytes()
    if len(original) != RAM_SIZE:
        raise ValueError('Expected exactly 24 MiB of MEM1')
    description_path = fixture / 'fixture.json'
    description_bytes = description_path.read_bytes()
    description = json.loads(description_bytes)
    if description['format'] != 'daywater-native-team-startup-fixture-v1':
        raise ValueError('Unsupported fixture schema')
    inputs = {str(snapshot): digest(original), str(description_path): digest(description_bytes)}
    patches = []
    def add(address, payload, label):
        address = number(address)
        if address < BASE or address - BASE > RAM_SIZE or len(payload) > RAM_SIZE - (address - BASE):
            raise ValueError('Fixture patch outside MEM1: ' + label)
        for prior, data, name in patches:
            if address < prior + len(data) and prior < address + len(payload):
                raise ValueError('Overlapping fixture owners: ' + label + ' / ' + name)
        patches.append((address, payload, label))
    def read(name, expected):
        path = private(fixture / name)
        if not path.is_relative_to(fixture):
            raise ValueError('Fixture file escapes its directory')
        data = path.read_bytes()
        if len(data) != expected:
            raise ValueError('Fixture payload size differs: ' + name)
        inputs[str(path)] = digest(data)
        return data
    source_base = number(description['source_base'])
    source = read(description['source_file'], description['source_size'])
    add(source_base, source, 'native-prepared source')
    # Original 80129AB8 and team_devices_original_check establish this exact
    # lookup array. Values here are explicit host inputs, not discovered state.
    mapping = description.get('port_mapping')
    if mapping is not None:
        mask = number(description['supplied_port_mapping_mask'])
        if len(mapping) != 10 or not 0 <= mask <= 1023:
            raise ValueError('Invalid supplied port mapping')
        for slot, value in enumerate(mapping):
            value = number(value)
            if not 0 <= value <= 0xffffffff:
                raise ValueError('Invalid supplied mapping word')
            if mask & (1 << slot):
                add(0x80282e34 + slot * 4, struct.pack('>I', value), 'supplied logical port mapping ' + str(slot))
    for index, font in enumerate(description['native_font_copies']):
        words = [number(v) for v in font['record_words']]
        if len(words) != 5 or words[0] != 0:
            raise ValueError('Unsupported supplied font record')
        for slot, field in ((2, 'glyphs_offset'), (3, 'kerning_offset'), (4, 'atlas_offset')):
            if words[slot] != words[1] + font[field] or not 0 <= font[field] < font['size']:
                raise ValueError('Font record does not own its supplied data')
        add(words[1], read(font['file'], font['size']), 'font bytes ' + str(index))
        add(font['record_address'], struct.pack('>5I', *words), 'supplied font record ' + str(index))
        if number(font['asset_record_word']) != number(font['record_address']):
            raise ValueError('Font asset pointer does not identify its owner')
        add(font['asset_record_address'], struct.pack('>I', number(font['asset_record_word'])), 'font asset pointer ' + str(index))
    for plan in description['line_plans']:
        ranges = plan['ranges']
        if len(ranges) != plan['line_count'] or len(ranges) > 255:
            raise ValueError('Invalid line count')
        payload = bytearray(1024)
        for index, pair in enumerate(ranges):
            if len(pair) != 2 or not 0 <= pair[0] <= pair[1] <= 65535:
                raise ValueError('Invalid supplied line range')
            struct.pack_into('>HH', payload, index * 4, *pair)
        if ranges:
            target = number(plan['target_object']) - source_base
            if target < 0 or target + 40 > len(source) or struct.unpack_from('>I', source, target + 36)[0] != number(plan['native_logical_base']):
                raise ValueError('Source object does not reference its supplied line owner')
            add(plan['native_logical_base'], payload, 'native line owner ' + str(plan['ordinal']))
    result = bytearray(original)
    for address, data, _ in patches:
        result[address - BASE:address - BASE + len(data)] = data
    return result, {'boundary': 'Native-prepared source/fonts/lines over saved RAM copy; controlled comparison only, no original startup claim.',
                    'inputs_sha256': inputs, 'output_sha256': digest(result),
                    'patches': [{'address': hex(address), 'size': len(data), 'owner': name} for address, data, name in patches]}

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--snapshot', required=True, type=Path)
    parser.add_argument('--fixture', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    output = private(args.output)
    if output == private(args.snapshot) or output.is_relative_to(private(args.fixture)):
        raise ValueError('Output must preserve the snapshot and fixture inputs')
    data, report = materialize(args.snapshot, args.fixture)
    if output.exists() and output.read_bytes() != data:
        raise ValueError('Refusing to replace a different existing RAM file')
    for name, expected in report['inputs_sha256'].items():
        if digest(Path(name).read_bytes()) != expected:
            raise ValueError('Fixture input changed during materialization: ' + name)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(data)
    output.with_suffix('.json').write_text(json.dumps(report, indent=2) + '\n')
    print('Materialized controlled team fixture privately: ' + str(output))

if __name__ == '__main__':
    main()
