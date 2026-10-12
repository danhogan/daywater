"""Compare native environment assembly to independently grouped original TMdl layers.

Offline geometry checks do not verify original transforms, scene scheduling,
material state, instance placement, or live rendering.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
import subprocess
from pathlib import Path
from verify_models import DOL_SHA1, triangles, windows

def info(data):
    if data[:4] != b'TMdl':
        return None
    count, offset = struct.unpack_from('>HH', data, 8)
    names = []
    for at in range(offset, offset+count*16, 16):
        if data[at:at+4] == b'Info':
            start, length = struct.unpack_from('>II', data, at+4)
            names.append(data[start:start+length].split(b'\0')[0].decode('ascii'))
    assert len(names) == 1
    return names[0]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('exe', 'assets', 'unpacked', 'output', 'dol'):
        parser.add_argument('--'+name, type=Path, required=True)
    parser.add_argument('--windows', action='store_true')
    args = parser.parse_args()
    args.output.resolve().relative_to(Path('/mnt/w/Daywater').resolve())
    assert hashlib.sha1(args.dol.read_bytes()).hexdigest() == DOL_SHA1
    args.output.mkdir(parents=True, exist_ok=True)
    models = {}
    for path in sorted((args.unpacked/'envrnmt').glob('*.bin')):
        data = path.read_bytes()
        name = info(data)
        if name and (name.endswith('_rField.ea3') or '_FieldPass' in name):
            assert name not in models
            models[name] = (int(path.stem), data)
    venues = sorted(name[:-11] for name in models if name.endswith('_rField.ea3'))
    results = []
    for venue in venues:
        names = [f'{venue}_rField.ea3']+[f'{venue}_FieldPass{i}.ea3' for i in range(1, 6)]
        parts, layers, total_draws, material_offset = [], [], 0, 0
        for name in names:
            entry, data = models[name]
            dump, draws, count = triangles(data, inspection=True)
            payload = bytearray(dump[12:])
            for at in range(0, len(payload), 64):
                selector = struct.unpack_from('<I', payload, at)[0]
                struct.pack_into('<I', payload, at, selector+material_offset)
            section_count, directory = struct.unpack_from('>HH', data, 8)
            for at in range(directory, directory+section_count*16, 16):
                if data[at:at+4] == b'Matl':
                    material_offset += struct.unpack_from('>I', data, struct.unpack_from('>I', data, at+4)[0])[0]
            parts.append(payload)
            total_draws += draws
            layers.append(dict(name=name, entry=entry, triangles=count, sha256=hashlib.sha256(data).hexdigest()))
        count = sum(layer['triangles'] for layer in layers)
        expected = b'NMSH'+struct.pack('<II', count, total_draws)+b''.join(parts)
        bmp, dump = args.output/(venue+'.bmp'), args.output/(venue+'.mesh')
        argv = [str(args.exe), str(args.assets/'envrnmt.dat'), venue, str(bmp), '--headless', str(dump)]
        if args.windows:
            argv = ['cmd.exe', '/d', '/c', windows(args.exe), windows(args.assets/'envrnmt.dat'), venue, windows(bmp), '--headless', windows(dump)]
        run = subprocess.run(argv, capture_output=True, text=True)
        (args.output/(venue+'.log')).write_text(run.stdout+run.stderr)
        if run.returncode:
            results.append(dict(venue=venue, supported=False, layers=layers))
            continue
        assert dump.read_bytes() == expected, f'Assembly differs: {venue}'
        results.append(dict(venue=venue, supported=True, layers=layers, triangles=count,
                            exact_geometry_match=True, native_sha256=hashlib.sha256(expected).hexdigest()))
    report = dict(confidence='candidate', windows=args.windows, dol_sha1=DOL_SHA1,
                  original_scene_transform_verification=False, live_pixel_comparison=False,
                  inspection_color_exception=True, cases=results)
    (args.output/'verification.json').write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(dict(venues=len(results), supported=sum(r['supported'] for r in results),
                         triangles=sum(r.get('triangles', 0) for r in results))))

if __name__ == '__main__':
    main()
