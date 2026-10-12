"""Offline comparison: Windows native triangles versus an independent raw-TMdl reader."""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
import subprocess
from pathlib import Path

DOL_SHA1 = '46e3de854998a0f66c9be8c72a1164d22cd29eb4'
CASES = {'objmodel': [0, 1, 13, 16, 24, 25, 33, 35, 36, 37], 'statmod': [1], 'envrnmt': [4, 15, 22]}

def triangles(data, inspection=False, bind_pose=False):
    assert data[:4] == b'TMdl'
    u16 = lambda at: struct.unpack_from('>H', data, at)[0]
    u32 = lambda at: struct.unpack_from('>I', data, at)[0]
    sections = {data[at:at+4]: u32(at+4) for at in range(u16(10), u16(10)+16*u16(8), 16)}
    geom = sections[b'Geom']
    draw_table, draw_count, array_table, array_count = struct.unpack_from('>4I', data, geom)
    result = []
    palette = None
    slots = {}
    formats = ('>B', '>b', '>H', '>h', '>f')
    for draw in range(draw_count):
        row = draw_table+draw*24
        matrix_draw = bool(data[row+17] & 1)
        if matrix_draw:
            assert bind_pose
            if palette is None:
                from verify_cluster_pose import cluster_palette
                palette = cluster_palette(data, sections)
        pos, end = u32(row), u32(row)+u16(row+4)*32
        attrs = [array_table+12*i for i in range(u16(row+14), u16(row+14)+data[row+16])]
        assert all((a-array_table)//12 < array_count for a in attrs)
        material = u16(row+6)
        while pos < end:
            command = data[pos]
            pos += 1
            if not command:
                continue
            if matrix_draw and command in (0x20,0x28):
                source, packed = struct.unpack_from('>HH',data,pos)
                pos += 4
                address, length = packed&4095, (packed>>12)+1
                if command==0x20:
                    assert source<len(palette) and length==12 and address%4==0 and address//4<=61
                    slots={key:value for key,value in slots.items() if abs(key-address//4)>2}
                    slots[address//4]=source
                else:
                    assert source<len(palette)*3 and length==3 and 0x400<=address<=0x4fd
                continue
            assert command in (0x80, 0x90, 0x98, 0xa0)
            vertices = []
            count = u16(pos)
            pos += 2
            for _ in range(count):
                v = [0.0]*5
                transform = None
                if matrix_draw:
                    transform = palette[slots[data[pos]]]
                    pos += 1
                for a in attrs:
                    mode = data[a+11] & 15
                    assert mode in (2, 3)
                    index = data[pos] if mode == 2 else u16(pos)
                    pos += mode-1
                    if index >= u16(a+4):
                        assert inspection and data[a+6] in (11, 12)
                        assert u32(a)+data[a+7]*(index+1) <= len(data)
                    attr, stride = data[a+6:a+8]
                    if attr not in (9, 13):
                        continue
                    components, kind, frac = data[a+8:a+11]
                    length = (3 if components else 2) if attr == 9 else (2 if components else 1)
                    offset = u32(a)+stride*index
                    fmt = formats[kind]
                    for i in range(length):
                        value = struct.unpack_from(fmt, data, offset+i*struct.calcsize(fmt))[0]
                        if kind != 4:
                            value /= 2**frac
                        v[i if attr == 9 else i+3] = value
                if transform is not None:
                    v[:3]=[sum(transform[r][k]*v[k] for k in range(3))+transform[r][3] for r in range(3)]
                vertices.append(v)
            assert pos <= end
            if command == 0x90:
                groups = [(i, i+1, i+2) for i in range(0, count, 3)]
            elif command == 0x80:
                groups = [(i, i+1, i+2) for i in range(0, count, 4)] + [(i, i+2, i+3) for i in range(0, count, 4)]
                groups.sort(key=lambda t: t[0])
            elif command == 0xa0:
                groups = [(0, i-1, i) for i in range(2, count)]
            else:
                groups = [(i-1, i-2, i) if i % 2 else (i-2, i-1, i) for i in range(2, count)]
            for a, b, c in groups:
                result.append((material, vertices[a], vertices[b], vertices[c]))
    out = bytearray(b'NMSH'+struct.pack('<II', len(result), draw_count))
    for material, *vertices in result:
        out.extend(struct.pack('<I', material))
        for v in vertices:
            out.extend(struct.pack('<5f', *v))
    return out, draw_count, len(result)

def windows(path):
    return 'W:\\'+str(path.resolve().relative_to('/mnt/w')).replace('/', '\\')

def main():
    p = argparse.ArgumentParser(description=__doc__)
    for name in ('exe', 'assets', 'unpacked', 'output', 'dol'):
        p.add_argument('--'+name, type=Path, required=True)
    p.add_argument('--windows', action='store_true')
    args = p.parse_args()
    args.output.resolve().relative_to(Path('/mnt/w/Daywater').resolve())
    if hashlib.sha1(args.dol.read_bytes()).hexdigest() != DOL_SHA1:
        raise ValueError('Wrong original module')
    args.output.mkdir(parents=True, exist_ok=True)
    report = []
    for archive, indexes in CASES.items():
        for index in indexes:
            name = f'{archive}-{index:05d}'
            source = args.unpacked/archive/f'{index:05d}.bin'
            expected, draws, count = triangles(source.read_bytes())
            bmp, dump = args.output/(name+'.bmp'), args.output/(name+'.mesh')
            argv = [str(args.exe), str(args.assets/(archive+'.dat')), str(index), str(bmp), '--headless', str(dump)]
            if args.windows:
                argv = ['cmd.exe', '/d', '/c', windows(args.exe), windows(args.assets/(archive+'.dat')), str(index), windows(bmp), '--headless', windows(dump)]
            run = subprocess.run(argv, capture_output=True, text=True, check=True)
            (args.output/(name+'.log')).write_text(run.stdout+run.stderr)
            actual = dump.read_bytes()
            if actual != expected:
                raise ValueError(f'Native triangle/vertex bytes differ: {name}')
            report.append(dict(archive=archive, index=index, draws=draws, triangles=count,
                               source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
                               native_sha256=hashlib.sha256(actual).hexdigest(), exact_geometry_match=True))
    result = dict(confidence='probable', dol_sha1=DOL_SHA1, windows=args.windows,
                  live_original_render_comparison=False, cases=report,
                  triangles=sum(c['triangles'] for c in report))
    (args.output/'verification.json').write_text(json.dumps(result, indent=2)+'\n')
    print(json.dumps(dict(cases=len(report), triangles=result['triangles'], exact_geometry_match=True)))

if __name__ == '__main__':
    main()
