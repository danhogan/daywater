"""Verify native procedural sky against both private captures; export NFR2.

Private original constants/palette and measured state only: no captured mesh
or emulator RAM is supplied to the independent renderer.
"""
import argparse
import ctypes as C
import hashlib
import json
from pathlib import Path
import struct
from verify_fifo_capture import Capture


class Vertex(C.Structure):
    _fields_ = [('position', C.c_float * 3), ('uv', C.c_float * 2), ('rgba', C.c_uint8 * 4)]


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('directory', type=Path)
    p.add_argument('--dol', type=Path, required=True)
    p.add_argument('--library', type=Path, required=True)
    p.add_argument('--windows-dump', type=Path)
    a = p.parse_args(); root = a.directory.resolve()
    root.relative_to(Path('/mnt/w/Daywater').resolve())
    module = a.dol.read_bytes()
    if hashlib.sha1(module).hexdigest() != '46e3de854998a0f66c9be8c72a1164d22cd29eb4':
        raise ValueError('original module revision differs')

    def original(addr, count):
        for i in range(18):
            off, base, size = [struct.unpack_from('>I', module, k+i*4)[0] for k in (0, 0x48, 0x90)]
            if base <= addr and addr+count <= base+size:
                return module[off+addr-base:off+addr-base+count]
        raise ValueError('original module data span')

    def floats(addr, n):
        return list(struct.unpack('>'+str(n)+'f', original(addr, n*4)))

    f32 = lambda x: struct.unpack('<f', struct.pack('<f', x))[0]
    factors = floats(0x80261754, 7); pi = floats(0x80261770, 1)[0]
    exponents = floats(0x80261774, 8)
    coefficients = [f32(pi**exponents[0])]
    coefficients += [f32((-1 if i % 2 == 0 else 1)*factors[i]*pi**exponents[i+1]) for i in range(7)]
    # Palette identity is checked against every captured RGB8 below. This
    # records the matching source palette, not a guessed time-of-day meaning.
    palette = floats(0x8027babc+7*60, 15)
    cfg = floats(0x802536d0, 1)+[floats(0x802536b8, 1)[0]/8]+floats(0x802536bc, 1)
    cfg += floats(0x8027be7c, 4)+coefficients+floats(0x802617a0, 1)+palette
    library = C.CDLL(str(a.library.resolve()))
    library.nflstreet_sky_vertices.argtypes = [C.POINTER(C.c_float), C.POINTER(Vertex)]
    vertices = (Vertex*41)()
    if not library.nflstreet_sky_vertices((C.c_float*31)(*cfg), vertices):
        raise ValueError('native procedural constructor failed')
    colors = bytes(int(f32(x*255)) for x in palette)
    expected = [[], []]
    for r in range(4):
        for s in range(8):
            n = (s+1) % 8
            expected[0] += [(8*(r+1)+s, r if r else 0), (8*(r+1)+n, r if r else 0),
                            (8*r+n, r-1 if r else 0), (8*r+s, r-1 if r else 0)]
    expected[1] = [(idx, color) for s in range(8) for idx, color in [(32+s, 3), (32+(s+1)%8, 3), (40, 4)]]
    windows_checked = False
    if a.windows_dump:
        diagnostic = b''.join(struct.pack('<3f', *v.position) for v in vertices)
        triangles = []
        for at in range(0, len(expected[0]), 4):
            quad = expected[0][at:at+4]
            triangles += [quad[:3], [quad[0], quad[2], quad[3]]]
        triangles += [expected[1][at:at+3] for at in range(0, 24, 3)]
        for triangle in triangles:
            for idx, color in triangle:
                diagnostic += struct.pack('<3f', *vertices[idx].position)+colors[color*3:color*3+3]+b'\xff'
        if a.windows_dump.read_bytes() != diagnostic:
            raise ValueError('Windows native vertices/triangles/colors differ')
        windows_checked = True
    reports = []; outputs = []
    for name in ['reference', 'repeat']:
        cap = Capture(root/(name+'.dff'))
        state = json.loads((root/(name+'-state.json')).read_text())
        ram = bytearray(32*1024*1024); commands = []; max_error = 0; exact = checked = 0
        for fi, frame in enumerate(state['frames']):
            rec = cap.u64(60)+fi*64; up, n = cap.u64(rec+20), cap.u32(rec+28); update = 0
            for di, draw in enumerate(frame['draws']):
                while update < n and cap.u32(up+update*24) <= draw['fifo_position']:
                    row = up+update*24; address, size = cap.u32(row+4), cap.u32(row+16); dest = address&0x1ffffff
                    if address&0x10000000 or size > len(ram)-dest: raise ValueError('RAM span')
                    ram[dest:dest+size] = cap.span(cap.u64(row+8), size); update += 1
                if fi != 1 or di > 1: continue
                if draw['vertex_size'] != 4 or draw['vertices'] != len(expected[di]) or draw['primitive'] != [0, 2][di]:
                    raise ValueError('sky primitive shape differs')
                if draw['vertex_descriptor'] != [0x6600, 0] or [draw['array_strides'][i] for i in [0, 2]] != [16, 3]:
                    raise ValueError('sky vertex encoding differs')
                payload = cap.span(cap.u64(rec)+draw['fifo_position']+3, draw['vertices']*4)
                pairs = list(struct.iter_unpack('>HH', payload))
                if pairs != expected[di]: raise ValueError('sky topology/color selectors differ')
                for idx, color in pairs:
                    pos = draw['array_bases'][0]+idx*16
                    rgb = draw['array_bases'][2]+color*3
                    captured = ram[pos:pos+12]; generated = struct.pack('>3f', *vertices[idx].position)
                    exact += captured == generated; checked += 1
                    error = max(abs(x-y) for x, y in zip(struct.unpack('>3f', captured), vertices[idx].position))
                    max_error = max(max_error, error)
                    if error > 0.0001: raise ValueError(f'original procedural position differs: {error}')
                    if ram[rgb:rgb+3] != colors[color*3:color*3+3]: raise ValueError('original palette differs')
                bp, xf = draw['bp'], draw['xf_registers']
                if (bp[0]>>10)&15 or (bp[0]>>14)&3 or draw['projection'][6]: raise ValueError('sky state unsupported')
                ints = [6, di, bp[0x40], bp[0x41], bp[0xc0], bp[0xc1], bp[0xf3], bp[0x80],
                        xf[12], xf[10], xf[14], xf[16], xf[64], bp[0x43], 0, 0xffffffff]
                data = draw['position_matrix']+draw['projection'][:6]+xf[26:32]+draw['texture_matrix']
                commands.append(struct.pack('<52I', *(ints+data)))
            while update < n:
                row = up+update*24; dest = cap.u32(row+4)&0x1ffffff; size = cap.u32(row+16)
                if size > len(ram)-dest: raise ValueError('RAM span')
                ram[dest:dest+size] = cap.span(cap.u64(row+8), size); update += 1
        if len(commands) != 2: raise ValueError('two sky draws required')
        field = (root/(name+'-field-profile.bin')).read_bytes()
        if field[:4] != b'NFR1' or len(field) != 40+struct.unpack_from('<I', field, 4)[0]*208: raise ValueError('field profile')
        output = b'NFR2'+struct.pack('<I', struct.unpack_from('<I', field, 4)[0]+2)+field[8:40]
        output += b'NSK1'+struct.pack('<31f', *cfg)+b''.join(commands)+field[40:]
        (root/(name+'-sky-field-profile.bin')).write_bytes(output); outputs.append(output)
        reports.append(dict(capture=name, referenced_vertices=checked, exact_positions=exact,
                            maximum_position_error=max_error, profile_sha256=hashlib.sha256(output).hexdigest()))
    report = dict(confidence='verified' if all(r['exact_positions']==152 for r in reports) else 'probable',
                  game='NFL Street', revision='GNNE69 revision 0', module_sha1=hashlib.sha1(module).hexdigest(),
                  constructor='0x800e35f0', draw='0x801655d8', math='0x8018889c / 0x801888f0 / 0x80188a70 / 0x80188aec',
                  position_array='0x80385e8c: XYZ float32 BE, 16-byte stride',
                  color_array='0x80310a78: RGB8, 3-byte stride', source_palette='0x8027babc + 7*60',
                  test='Native C generation checked against each referenced vertex and color in two fresh original captures',
                  geometry=dict(vertices=41, quads=32, cap_triangles=8, radius=cfg[0]),
                  profiles_equal=outputs[0]==outputs[1], captures=reports,
                  windows_constructor_and_all_72_triangle_colors_checked=windows_checked,
                  limitations=['Only the supplied saved match has been verified.',
                               'Palette identity matches quantized RGB8; live palette selection/time interpolation is not reconstructed.',
                               'Camera rotation and draw state are captured; live camera updates are not reconstructed.',
                               'Nearby screenshot is not exact-frame pixel evidence; rasterizer pixel fidelity remains candidate.'])
    (root/'native-sky-verification.json').write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(report))
    if not report['profiles_equal']: raise ValueError('repeat sky profile differs')


if __name__ == '__main__': main()
