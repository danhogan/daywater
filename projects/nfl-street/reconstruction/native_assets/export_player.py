"""Identify a bounded fixed-pose player from exact original asset/capture bytes.

NAP1 contains asset selectors and measured pose/light/palette state. It never
contains triangles, positions, normals, UVs or texture index planes.
"""
import argparse
from collections import Counter, defaultdict
import hashlib
import json
from pathlib import Path
import struct
from player_assets import model
from verify_fifo_capture import Capture

CATEGORIES = ['plamodel', 'plaface', 'plahair', 'plahat', 'plaglass', 'plarosterhair', 'statmod']


def attributes(m, p, d, ram, template=False):
    b = m['bytes']; at = 3; checked = 0
    for _ in range(p['vertices']):
        slot = 0
        if p['matrix']:
            slot = p['payload'][at]; at += 1
            if slot not in p['slots']: return False
        for a in p['attrs']:
            offset, count, kind, stride = struct.unpack_from('>IHBB', b, a)
            mode = b[a+11]&15
            if mode not in [2, 3]: raise ValueError('indexed original attribute form')
            idx = p['payload'][at] if mode == 2 else struct.unpack_from('>H', p['payload'], at)[0]; at += mode-1
            array = {9: 0, 10: 1, 11: 2, 13: 4, 14: 5}.get(kind)
            if array is None: raise ValueError('unhandled original player attribute')
            src, live = offset+idx*stride, d['array_bases'][array]+idx*stride
            if idx >= count or src+stride > len(b) or live+stride > len(ram): return False
            if d['array_strides'][array] != stride: return False
            if ram[live:live+stride] != b[src:src+stride] and not (template and kind in [9,10]): return False
            checked += 1
        # Indexed position matrix source is defined by the original display
        # list. Check the selected GPU rows against the recorded source RAM.
        if not p['matrix']: continue
        cluster = p['slots'][slot]; addr = d['array_bases'][12]+cluster*d['array_strides'][12]
        if ram[addr:addr+48] != struct.pack('>12I', *d['position_palette'][slot*4:slot*4+12]): return False
    return checked


def main():
    a = argparse.ArgumentParser(description=__doc__)
    a.add_argument('directory', type=Path)
    a.add_argument('--assets', type=Path, default=Path('/mnt/w/Daywater/private/nflstreet/GNNE69-r0/unpacked'))
    a.add_argument('--ball', action='store_true', help='Export the checked frame1 GameBall draw group')
    a.add_argument('--output', type=Path)
    opts = a.parse_args(); root = opts.directory.resolve(); root.relative_to(Path('/mnt/w/Daywater').resolve())
    output_root = (opts.output or root).resolve(); output_root.relative_to(Path('/mnt/w/Daywater').resolve()); output_root.mkdir(parents=True, exist_ok=True)
    suffix = 'ball' if opts.ball else 'player'
    index = defaultdict(list)
    for category in (['statmod'] if opts.ball else CATEGORIES[:6]):
        for path in (opts.assets/category).glob('*.bin'):
            m = model(path)
            if not m: continue
            m.update(category=category, entry=int(path.stem))
            for p in m['primitives']:
                index[hashlib.sha256(p['payload']).digest()].append((m, p))
    texindex = defaultdict(list)
    for path in (opts.assets/('statmod' if opts.ball else 'platex')).glob('*.bin'):
        b = path.read_bytes()
        if b[:4] != b'MMAP': continue
        u16 = lambda at: struct.unpack_from('>H', b, at)[0]
        u32 = lambda at: struct.unpack_from('>I', b, at)[0]
        for image in range(u16(14)):
            row = u32(24)+image*16; w, h, fmt = struct.unpack_from('>3H', b, row)
            if fmt not in [8, 9, 14]: continue
            size = ((w+7)//8)*((h+(7 if fmt in [8,14] else 3))//(8 if fmt in [8,14] else 4))*32
            data = b[u32(row+12):u32(row+12)+size]
            if len(data) != size: raise ValueError('texture plane bounds')
            texindex[(w, h, fmt, hashlib.sha256(data[:64]).digest())].append((int(path.stem), image, data))
    reports = []; profiles = []
    for name in ['reference', 'repeat']:
        cap = Capture(root/(name+'.dff')); decoded = json.loads(((output_root if opts.ball else root)/(name+('-ball-state.json' if opts.ball else '-actor-state.json'))).read_text())
        ram = bytearray(32*1024*1024); resources = []
        for fi in range(len(decoded['frames'])):
            rec = cap.u64(60)+fi*64; up, n = cap.u64(rec+20), cap.u32(rec+28)
            for ui in range(n):
                row = up+ui*24
                if cap.span(row+20, 1)[0] == 1:
                    resources.append((cap.u32(row+4)&0x1ffffff, cap.span(cap.u64(row+8), cap.u32(row+16))))

        def texture(d, unit):
            bp = d['bp']; desc = bp[0x88+unit]; w, h, fmt = (desc&1023)+1, ((desc>>10)&1023)+1, (desc>>20)&15
            addr = (bp[0x94+unit]<<5)&0x1ffffff
            prefixes = {bytes(ram[addr:addr+64])}
            for start, data in resources:
                if start <= addr and addr+64 <= start+len(data): prefixes.add(data[addr-start:addr-start+64])
            found = []
            for prefix in prefixes:
                for entry, image, data in texindex.get((w, h, fmt, hashlib.sha256(prefix).digest()), []):
                    spans = bytearray(len(data)); valid = True
                    for start, payload in resources:
                        lo, hi = max(addr, start), min(addr+len(data), start+len(payload))
                        if hi <= lo: continue
                        if payload[lo-start:hi-start] != data[lo-addr:hi-addr]: valid = False; break
                        spans[lo-addr:hi-addr] = b'\1'*(hi-lo)
                    if valid and all(spans): found.append((entry, image, data))
            if not found: raise ValueError(f'original texture not identified at {addr:x} ({w} {h} {fmt})')
            if len({x[2] for x in found}) != 1: raise ValueError('ambiguous original texture bytes')
            entry, image, _ = min(found)
            return [entry, image, (bp[0x98+unit]>>10)&3]

        records = []; counts = Counter(); ambiguous = []; checks = 0; triangles = 0; skin = 0; templates = []
        for fi, frame in enumerate(decoded['frames']):
            rec = cap.u64(60)+fi*64; up, n = cap.u64(rec+20), cap.u32(rec+28); update = 0
            for di, d in enumerate(frame['draws']):
                while update < n and cap.u32(up+update*24) <= d['fifo_position']:
                    row = up+update*24; dest, size = cap.u32(row+4)&0x1ffffff, cap.u32(row+16)
                    if size > len(ram)-dest: raise ValueError('RAM span')
                    ram[dest:dest+size] = cap.span(cap.u64(row+8), size); update += 1
                # This original interval contains one body/head/hair palette
                # group, ending before the next repeated body palette begins.
                if fi != 1 or not ((4224 <= di < 4329) if opts.ball else (4329 <= di < 4886)): continue
                payload = cap.span(cap.u64(rec)+d['fifo_position'], 3+d['vertices']*d['vertex_size'])
                found = []
                for m, p in index.get(hashlib.sha256(payload).digest(), []):
                    checked = attributes(m, p, d, ram)
                    if checked: found.append((m, p, checked))
                if not found:
                    for m, p in index.get(hashlib.sha256(payload).digest(), []):
                        if m['name'] != 'BaseHead1500.ea3': continue
                        checked = attributes(m, p, d, ram, template=True)
                        if checked: found.append((m, p, checked))
                    if found: templates.append(di)
                if not found: raise ValueError(f'player draw {di}: no original command/UV/pose match')
                # Preserve aliases; choose an original source with identical
                # referenced bytes rather than assert a particular player name.
                found.sort(key=lambda x: (CATEGORIES.index(x[0]['category']), x[0]['entry'], x[1]['offset']))
                m, p, checked = found[0]; checks += checked; counts[m['name']] += 1
                if len(found)>1: ambiguous.append(dict(draw=di, matching_sources=len(found)))
                bp, xf = d['bp'], d['xf_registers']; stages = 1+((bp[0]>>10)&15)
                if stages not in [1, 3] or xf[14]!=0x51e or xf[16] not in ([0x51c] if opts.ball else [0x51e]) or d['projection'][6] or ((bp[0]>>14)&3) != (0 if opts.ball else 2):
                    raise ValueError(f'unsupported player state {di}')
                primary = texture(d, 0); secondary = [0xffffffff, 0, 0]
                if stages == 3:
                    if [bp[i] for i in range(0xc0, 0xc6)] != [0x8fff8, 0x8ffc0, 0x8089f, 0x8ff80, 0x18f0af, 0x18faf0]:
                        raise ValueError('skin combine program differs')
                    if d['texture_matrix'] != d['secondary_texture_matrix'] or xf[64]!=xf[65]: raise ValueError('skin UV generators differ')
                    # The secondary index plane has no complete immutable MMAP
                    # resource match in this recording; its source is unresolved.
                    # Keep base skin and record this explicit rendering omission.
                    skin += 1
                elif [bp[0xc0],bp[0xc1]] != [0x18f8af,0x18f2f0]: raise ValueError('base combine program differs')
                metadata = [CATEGORIES.index(m['category']), m['entry'], p['row'], p['offset'], p['vertices'], p['material'], *primary, *secondary]
                ints = [7, len(records), bp[0x40], bp[0x41], 0x18f8af, 0x18f2f0, bp[0xf3], bp[0x80],
                        xf[12], xf[10], xf[14], xf[16], xf[64], bp[0x43], len(records), 0xffffffff]
                identity = [0x3f800000,0,0,0,0,0x3f800000,0,0,0,0,0x3f800000,0]
                words = metadata+ints+identity+d['projection'][:6]+xf[26:32]+d['texture_matrix']
                positions=list(d['position_palette']); normals=list(d['normal_palette'])
                if not p['matrix']:
                    fixed=d['matrix_index_a']&63; positions[:12]=d['position_matrix']; normals[:9]=d['normal_palette'][(fixed&31)*3:(fixed&31)*3+9]
                words += positions+normals+d['lights']
                record = struct.pack('<'+str(len(words))+'I', *words)
                for u in range(2): record += bytes(d['palettes'][u]).ljust(512, b'\0') if u==0 or stages==3 else bytes(512)
                record += struct.pack('<I', bp[0x81]); assert len(record)==3876
                records.append(record)
                triangles += p['vertices']//3 if d['primitive']==2 else p['vertices']//2 if d['primitive']==0 else p['vertices']-2
            while update<n:
                row=up+update*24;dest,size=cap.u32(row+4)&0x1ffffff,cap.u32(row+16)
                if size>len(ram)-dest:raise ValueError('RAM span')
                ram[dest:dest+size]=cap.span(cap.u64(row+8),size);update+=1
        output = b'NAP1'+struct.pack('<3I', len(records), 640, 448)+b''.join(records)
        (output_root/(name+'-'+suffix+'-profile.bin')).write_bytes(output); profiles.append(output)
        reports.append(dict(capture=name, draws=len(records), triangles=triangles, attribute_checks=checks,
                            sources=dict(counts), aliases=ambiguous, skin_draws=skin, template_head_draws=templates, profile_sha256=hashlib.sha256(output).hexdigest()))
    report = dict(confidence='candidate', scope='Native fixed-pose body/head/hair assembly; original body/hair geometry and textures verified, head uses original base template',
                  assembly_confidence='probable', profiles_equal=profiles[0]==profiles[1], captures=reports,
                  limitations=['One contiguous body/head/hair draw group; roster/player name is not established.',
                               '92 head draws use the original BaseHead1500 template; runtime-personalized face positions/normals are not reconstructed.',
                               'Pose matrices, lights and dynamic color palettes are captured fixed state; live skeletal evaluation/appearance selection remain open.',
                               'The unresolved secondary skin-overlay index plane is omitted for 66 draws; native preview renders their checked base skin texture.',
                               'Native pixel fidelity remains candidate; nearby screenshot is not exact-frame pixel evidence.'])
    if opts.ball:
        report.update(confidence='verified', scope='Exact GameBall primitive, position, normal, UV, texture and matrix resource matches in repeated original captures', assembly_confidence='verified', limitations=['Shared GameBall/GameBallLow primitive and attribute bytes; selected alias is a deterministic source choice.', 'Pose/light/camera state is captured; native pixel fidelity and moving attachment are separate checks.'])
    (output_root/('native-'+suffix+'-export-verification.json')).write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items() if k!='captures'})); print([{k:v for k,v in r.items() if k!='aliases'} for r in reports])
    if not report['profiles_equal']: raise ValueError('repeat player profile differs')


if __name__ == '__main__': main()
