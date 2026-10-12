"""Compare native posed player triangles to independent original-asset math.

Head checks verify the explicitly chosen base template, not the unresolved
runtime-personalized face. No original pixel-equivalence claim is made.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
from player_assets import model

CATEGORIES = ['plamodel', 'plaface', 'plahair', 'plahat', 'plaglass', 'plarosterhair', 'statmod']


def main():
    a = argparse.ArgumentParser(description=__doc__)
    a.add_argument('directory', type=Path)
    a.add_argument('--assets', type=Path, default=Path('/mnt/w/Daywater/private/nflstreet/GNNE69-r0/unpacked'))
    a.add_argument('--ball', action='store_true')
    args = a.parse_args(); root = args.directory.resolve(); root.relative_to(Path('/mnt/w/Daywater').resolve())
    profile_kind='ball' if args.ball else 'player'
    b = (root/('reference-'+profile_kind+'-profile.bin')).read_bytes(); dump = (root/('native-'+profile_kind+'-mesh.bin')).read_bytes()
    assert b[:4] == b'NAP1' and dump[:4] == b'NMSH'
    count = struct.unpack_from('<I', b, 4)[0]; models = {}; expected = []
    for i in range(count):
        r = 16+i*3876; meta = struct.unpack_from('<12I', b, r); category, entry, row, off, n, material = meta[:6]
        key = (category, entry)
        if key not in models: models[key] = model(args.assets/CATEGORIES[category]/f'{entry:05}.bin')
        m = models[key]; p = next(p for p in m['primitives'] if p['offset']==off and p['row']==row)
        at = 3; vertices = []
        for _ in range(n):
            slot = 0
            if p['matrix']: slot = p['payload'][at]; at += 1
            xyz = uv = None
            for a in p['attrs']:
                data = m['bytes']; array, length, kind, stride = struct.unpack_from('>IHBB', data, a)
                mode = data[a+11]&15; idx = p['payload'][at] if mode==2 else struct.unpack_from('>H', p['payload'], at)[0]; at += mode-1
                if kind not in [9, 13]: continue
                assert data[a+9]==3
                values = struct.unpack_from('>'+('3h' if kind==9 else '2h'), data, array+idx*stride)
                values = [x/(2**data[a+10]) for x in values]
                if kind==9: xyz = values
                else: uv = values
            matrix = struct.unpack_from('<12f', b, r+256+slot*16)
            # Double-precision affine calculation is independent of native
            # float accumulation; require a tight measured error bound.
            eye = [matrix[j*4+3]+sum(matrix[j*4+k]*xyz[k] for k in range(3)) for j in range(3)]
            vertices.append(eye+uv)
        cmd = p['payload'][0]
        if cmd==0x90: tris = [(j,j+1,j+2) for j in range(0,n,3)]
        elif cmd==0x80: tris = [t for j in range(0,n,4) for t in [(j,j+1,j+2),(j,j+2,j+3)]]
        elif cmd==0xa0: tris = [(0,j-1,j) for j in range(2,n)]
        else: tris = [(j-2,j-1,j) if j%2==0 else (j-1,j-2,j) for j in range(2,n)]
        expected += [(i, [vertices[j] for j in tri]) for tri in tris]
    triangles, draws = struct.unpack_from('<2I', dump, 4)
    assert triangles==len(expected) and draws==count and len(dump)==12+triangles*64
    worst = 0; uv_equal = True
    for i, (material, vertices) in enumerate(expected):
        at = 12+i*64; assert struct.unpack_from('<I', dump, at)[0]==material
        for j, reference in enumerate(vertices):
            native = struct.unpack_from('<5f', dump, at+4+j*20)
            worst = max(worst, *(abs(x-y) for x,y in zip(native[:3], reference[:3])))
            uv_equal &= native[3:]==tuple(reference[3:])
    report = dict(confidence='verified', scope='Native Windows geometry against original selected assets plus measured fixed pose',
                  triangles=triangles, referenced_triangle_vertices=triangles*3, draws=draws,
                  maximum_position_error=worst, uv_exact=uv_equal, native_mesh_sha256=hashlib.sha256(dump).hexdigest(),
                  limitation=('Shared GameBall source choice; does not verify original pixels or attachment scheduling.' if args.ball else 'The selected head is the original base template; this does not verify runtime face personalization or original pixels.'))
    assert worst<0.000004 and uv_equal
    (root/('native-'+profile_kind+'-geometry-verification.json')).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))


if __name__ == '__main__': main()
