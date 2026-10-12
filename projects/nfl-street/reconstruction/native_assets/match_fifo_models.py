"""Identify captured position arrays by exact bytes from original TMdl assets.

Matches the entire declared position array at each draw's recorded RAM state.
This identifies geometry sources, not gameplay objects or material identity.
"""
import argparse
from collections import Counter, defaultdict
import hashlib
import json
from pathlib import Path
import struct
from verify_fifo_capture import Capture


def catalog(root):
    result = defaultdict(list)
    for archive in ['envrnmt', 'objmodel', 'statmod']:
        for path in sorted((root / archive).glob('*.bin')):
            data = path.read_bytes()
            if data[:4] != b'TMdl':
                continue
            u16 = lambda at: struct.unpack_from('>H', data, at)[0]
            u32 = lambda at: struct.unpack_from('>I', data, at)[0]
            sections = {data[at:at+4]: u32(at+4) for at in range(u16(10), u16(10)+16*u16(8), 16)}
            name = data[sections[b'Info']:].split(b'\0', 1)[0].decode('ascii')
            geom = sections[b'Geom']
            arrays, count = u32(geom+8), u32(geom+12)
            seen = set()
            for i in range(count):
                at = arrays+i*12
                offset, length, kind, stride = struct.unpack_from('>IHBB', data, at)
                if kind != 9 or not stride or not length or (offset, length, stride) in seen:
                    continue
                seen.add((offset, length, stride))
                payload = data[offset:offset+length*stride]
                if len(payload) != length*stride:
                    raise ValueError('model position span')
                if len(payload) < 64:
                    continue
                result[hashlib.sha256(payload[:64]).digest()].append(dict(
                    archive=archive, entry=path.stem, model=name, offset=offset, count=length,
                    stride=stride, data=payload))
    return result


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('directory', type=Path)
    p.add_argument('--assets', type=Path, default=Path('/mnt/w/Daywater/private/nflstreet/GNNE69-r0/unpacked'))
    a = p.parse_args()
    directory = a.directory.resolve()
    directory.relative_to(Path('/mnt/w/Daywater').resolve())
    models = catalog(a.assets)
    capture = Capture(directory/'reference.dff')
    states = json.loads((directory/'reference-state.json').read_text())
    ram = bytearray(32*1024*1024)
    matches = []
    for frame_index, frame in enumerate(states['frames']):
        record = capture.u64(60)+frame_index*64
        updates, count = capture.u64(record+20), capture.u32(record+28)
        update = 0
        for draw_index, draw in enumerate(frame['draws']):
            while update < count and capture.u32(updates+update*24) <= draw['fifo_position']:
                row = updates+update*24
                address, size = capture.u32(row+4), capture.u32(row+16)
                if address & 0x10000000:
                    raise ValueError('EXRAM unsupported')
                dest = address & 0x1ffffff
                if size > len(ram)-dest:
                    raise ValueError('RAM span')
                ram[dest:dest+size] = capture.span(capture.u64(row+8), size)
                update += 1
            # CP descriptor bits 9..10 select position input; only indexed input.
            if ((draw['vertex_descriptor'][0] >> 9) & 3) not in (2, 3):
                continue
            base = draw['array_bases'][0]
            prefix = bytes(ram[base:base+64])
            found = []
            for model in models.get(hashlib.sha256(prefix).digest(), []):
                payload = model['data']
                if draw['array_strides'][0] == model['stride'] and ram[base:base+len(payload)] == payload:
                    found.append({k:v for k,v in model.items() if k != 'data'})
            if found:
                matches.append(dict(frame=frame_index, draw=draw_index, sources=found,
                                    tev_stages=1+((draw['bp'][0] >> 10)&15),
                                    blend_enabled=bool(draw['bp'][0x41]&1),
                                    color0=hex(draw['bp'][0xc0]), alpha0=hex(draw['bp'][0xc1])))
        # Preserve updates after the final draw for subsequent frames.
        while update < count:
            row = updates+update*24;address,size = capture.u32(row+4),capture.u32(row+16)
            dest = address & 0x1ffffff
            if address & 0x10000000 or size > len(ram)-dest:
                raise ValueError('RAM span')
            ram[dest:dest+size] = capture.span(capture.u64(row+8),size);update += 1
    names = Counter(name for match in matches for name in {source['model'] for source in match['sources']})
    report = dict(confidence='verified', interpretation='exact full position-array identity in captured draw RAM',
                  matched_draws=len(matches), model_draw_counts=dict(names), matches=matches,
                  limitations=['Only envrnmt, objmodel and statmod catalogs searched.',
                               'Shared or duplicated position arrays retain every matching source.',
                               'Array identity alone does not identify a particular game object or material.'])
    (directory/'model-matches.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(matched_draws=len(matches),model_draw_counts=dict(names))))


if __name__ == '__main__':
    main()
