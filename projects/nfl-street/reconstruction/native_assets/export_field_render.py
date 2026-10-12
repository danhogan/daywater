"""Export bounded native EARS render settings after exact original-byte checks.

The native renderer loads original assets itself. The private profile supplies
measured draw order/state and camera, not geometry, emulator RAM or shaders.
"""
import argparse
from collections import Counter, defaultdict
import hashlib
import json
from pathlib import Path
import struct
from verify_fifo_capture import Capture


def parse_model(path):
    b = path.read_bytes()
    u16 = lambda o: struct.unpack_from('>H', b, o)[0]
    u32 = lambda o: struct.unpack_from('>I', b, o)[0]
    sections = {b[o:o+4]: u32(o+4) for o in range(u16(10), u16(10)+u16(8)*16, 16)}
    name = b[sections[b'Info']:].split(b'\0', 1)[0].decode('ascii')
    g = sections[b'Geom']; table, count, arrays, _ = struct.unpack_from('>4I', b, g)
    primitives = []
    for i in range(count):
        row = table+i*24
        attrs = [arrays+12*j for j in range(u16(row+14), u16(row+14)+b[row+16])]
        if b[row+17]&1:
            raise ValueError('field matrix selector unsupported')
        size = sum((b[a+11]&15)-1 for a in attrs)
        at, end = u32(row), u32(row)+u16(row+4)*32
        while at < end:
            if b[at] == 0:
                at += 1; continue
            if b[at] not in [0x80, 0x90, 0x98, 0xa0]:
                raise ValueError('field command')
            n = u16(at+1); payload = b[at:at+3+n*size]
            if len(payload) != 3+n*size or at+len(payload)>end:
                raise ValueError('primitive span')
            primitives.append(dict(offset=at, material=u16(row+6), attrs=attrs, payload=payload, vertices=n, vertex_size=size))
            at += len(payload)
    return dict(bytes=b, sections=sections, name=name, entry=int(path.stem), primitives=primitives)


def texture(model, material, bp, check_texture):
    b,s = model['bytes'],model['sections']
    u16=lambda o:struct.unpack_from('>H',b,o)[0]
    u32=lambda o:struct.unpack_from('>I',b,o)[0]
    r=u32(s[b'Matl']+4)+material*48
    shader=b[u32(r+4):].split(b'\0',1)[0].decode('ascii')
    if not u16(r+8):
        return None
    if shader!='OnePass' or u16(r+8)!=1:
        raise ValueError('field material not bounded OnePass')
    name=b[u32(r+12):].split(b'\0',1)[0]
    t=s[b'Text']; mm=b[t:]
    m16=lambda o:struct.unpack_from('>H',mm,o)[0]
    m32=lambda o:struct.unpack_from('>I',mm,o)[0]
    found=[i for i in range(m16(12)) if mm[m32(32)+i*16:m32(32)+(i+1)*16].split(b'\0',1)[0]==name]
    if len(found)!=1:raise ValueError('texture name ambiguity')
    descriptor=bp[0x88];matches=[]
    for image in range(m16(14)):
        tr=m32(24)+image*16;width,height,fmt=struct.unpack_from('>3H',mm,tr)
        if (descriptor&1023)+1!=width or ((descriptor>>10)&1023)+1!=height or (descriptor>>20)&15!=fmt:continue
        tilew=8 if fmt in [0,1,2,8,9,14] else 4
        tileh=8 if fmt in [0,8,14] else 4
        size=((width+tilew-1)//tilew)*((height+tileh-1)//tileh)*(64 if fmt==6 else 32)
        if fmt in [8,9,10]:raise ValueError('paletted field texture verification requires TMEM')
        data=mm[m32(tr+12):m32(tr+12)+size]
        if len(data)!=size:raise ValueError('texture span')
        try:check_texture((bp[0x94]<<5)&0x1ffffff,data)
        except ValueError:continue
        matches.append(dict(width=width,height=height,format=fmt,data=data,name=name.decode('ascii'),image=image))
    if not matches:raise ValueError('no embedded texture plane matches captured descriptor and bytes')
    if len({r['data'] for r in matches})!=1:raise ValueError('texture identity ambiguity')
    return matches[0]


def check_attributes(model, primitive, draw, ram):
    b=model['bytes']; payload=primitive['payload'];at=3;checked=Counter()
    for _ in range(primitive['vertices']):
        for a in primitive['attrs']:
            offset,count,kind,stride=struct.unpack_from('>IHBB',b,a)
            mode=b[a+11]&15
            if mode not in [2,3] or kind not in [9,11,13]:raise ValueError('field attribute form')
            index=payload[at] if mode==2 else struct.unpack_from('>H',payload,at)[0];at+=mode-1
            array={9:0,11:2,13:4}[kind]
            if draw['array_strides'][array]!=stride:raise ValueError('attribute stride differs')
            src=offset+index*stride;live=draw['array_bases'][array]+index*stride
            if src+stride>len(b) or live+stride>len(ram):raise ValueError('attribute byte span')
            if b[src:src+stride]!=ram[live:live+stride]:return None
            checked[kind]+=1
            if kind==11 and (b[a+9]!=5 or stride!=4):raise ValueError('field color must be RGBA8')
            if index>=count and kind!=11:raise ValueError('attribute count')
    return checked


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('directory',type=Path)
    p.add_argument('--assets',type=Path,default=Path('/mnt/w/Daywater/private/nflstreet/GNNE69-r0/unpacked/envrnmt'))
    p.add_argument('--capture',default='reference');a=p.parse_args()
    root=a.directory.resolve();root.relative_to(Path('/mnt/w/Daywater').resolve())
    names=['EARS_rField.ea3']+[f'EARS_FieldPass{i}.ea3' for i in range(1,6)]
    matches=json.loads((root/'model-matches.json').read_text())['matches']
    eligible={(r['frame'],r['draw']):{names.index(s['model']) for s in r['sources'] if s['model'] in names}
              for r in matches if any(s['model'] in names for s in r['sources'])}
    entries={s['entry'] for r in matches for s in r['sources'] if s['model'] in names}
    models=[parse_model(a.assets/(entry+'.bin')) for entry in sorted(entries)]
    models.sort(key=lambda m:names.index(m['name']))
    if [m['name'] for m in models]!=names:raise ValueError('six field models required')
    index=defaultdict(list)
    for layer,m in enumerate(models):
        for primitive in m['primitives']:
            index[hashlib.sha256(primitive['payload']).digest()].append((layer,m,primitive))
    cap=Capture(root/(a.capture+'.dff'));state=json.loads((root/(a.capture+'-state.json')).read_text())
    field_cameras={tuple(state['frames'][fi]['draws'][di]['position_matrix']) for fi,di in eligible}
    if len(field_cameras)!=1:raise ValueError('expected one measured field camera')
    ram=bytearray(32*1024*1024);commands=[];attributes=Counter();texture_checks=0
    texture_updates=[]
    for fi in range(len(state['frames'])):
        rec=cap.u64(60)+fi*64;up,n=cap.u64(rec+20),cap.u32(rec+28)
        for ui in range(n):
            row=up+ui*24
            if cap.span(row+20,1)[0] in [1,8]:
                texture_updates.append((cap.u32(row+4)&0x1ffffff,cap.span(cap.u64(row+8),cap.u32(row+16))))
    checked_textures=set()
    def check_texture(addr,data):
        key=(addr,hashlib.sha256(data).digest())
        if key in checked_textures:return
        covered=bytearray(len(data))
        for start,payload in texture_updates:
            lo,hi=max(addr,start),min(addr+len(data),start+len(payload))
            if lo>=hi:continue
            if payload[lo-start:hi-start]!=data[lo-addr:hi-addr]:
                raise ValueError('recorded texture resource differs or changes during capture')
            covered[lo-addr:hi-addr]=b'\1'*(hi-lo)
        if not all(covered):raise ValueError('texture capture does not cover full base plane')
        checked_textures.add(key)
    for fi,frame in enumerate(state['frames']):
        rec=cap.u64(60)+fi*64;up,n=cap.u64(rec+20),cap.u32(rec+28);update=0;fifo=cap.u64(rec)
        for di,draw in enumerate(frame['draws']):
            while update<n and cap.u32(up+update*24)<=draw['fifo_position']:
                row=up+update*24;address,size=cap.u32(row+4),cap.u32(row+16);dest=address&0x1ffffff
                if address&0x10000000 or size>len(ram)-dest:raise ValueError('RAM update span')
                ram[dest:dest+size]=cap.span(cap.u64(row+8),size);update+=1
            payload=cap.span(fifo+draw['fifo_position'],3+draw['vertices']*draw['vertex_size'])
            if tuple(draw['position_matrix']) not in field_cameras:continue
            found=[]
            for layer,m,prim in index.get(hashlib.sha256(payload).digest(),[]):
                checked=check_attributes(m,prim,draw,ram)
                if checked is not None:found.append((layer,m,prim,checked))
            if not found:
                if (fi,di) in eligible:raise ValueError(f'verified field draw {di} lacks exact native command/attribute match')
                continue
            # Identical original commands can occupy duplicate draw-list slots.
            # Collapse only when their model, material and vertex bytes agree.
            identities={(r[0],r[2]['material']) for r in found}
            if len(identities)!=1:raise ValueError('ambiguous native material assignment')
            layer,m,prim,checked=found[0];bp=draw['bp'];xf=draw['xf_registers']
            if (bp[0]>>10)&15 or (bp[0]>>14)&3:raise ValueError('not one-stage uncullled field')
            tex=texture(m,prim['material'],bp,check_texture) if bp[0x28]&0x40 else None
            if tex:
                image=bp[0x88];addr=(bp[0x94]<<5)&0x1ffffff
                if (image&1023)+1!=tex['width'] or ((image>>10)&1023)+1!=tex['height'] or (image>>20)&15!=tex['format']:
                    raise ValueError('texture descriptor differs from native material')
                # Texture-cache loads can be recorded after a batched primitive.
                # Verify the immutable resource over the capture, without inventing
                # an earlier RAM update or claiming exact cache playback timing.
                check_texture(addr,tex['data'])
                texture_checks+=1
            if xf[14]!=0x403 or xf[16]!=0x403:raise ValueError('field lighting branch differs')
            if draw['projection'][6]!=0:raise ValueError('perspective required')
            ints=[layer,prim['offset'],bp[0x40],bp[0x41],bp[0xc0],bp[0xc1],bp[0xf3],bp[0x80],xf[12],xf[10],xf[14],xf[16],xf[64],bp[0x43],prim['material'],tex['image'] if tex else 0xffffffff]
            floats=draw['position_matrix']+draw['projection'][:6]+xf[26:32]+draw['texture_matrix']
            commands.append(struct.pack('<52I',*(ints+floats)));attributes.update(checked)
        while update<n:
            row=up+update*24;address,size=cap.u32(row+4),cap.u32(row+16);dest=address&0x1ffffff
            if address&0x10000000 or size>len(ram)-dest:raise ValueError('RAM update span')
            ram[dest:dest+size]=cap.span(cap.u64(row+8),size);update+=1
    if not commands:raise ValueError('no field commands')
    # Header has count, width, height and six layer entry indices (9 words).
    output=b'NFR1'+struct.pack('<9I',len(commands),640,448,*[m['entry'] for m in models])+b''.join(commands)
    (root/(a.capture+'-field-profile.bin')).write_bytes(output)
    report=dict(confidence='verified',scope='exact referenced EARS primitive/position/color/UV/texture bytes and measured field camera',
                commands=len(commands),attribute_checks=dict(attributes),texture_checks=texture_checks,
                profile_sha256=hashlib.sha256(output).hexdigest(),models=[m['name'] for m in models],
                limitations=['Profile supplies saved camera/state, not native camera update logic.',
                             'Texture bytes are checked across the interval; cache load timing is not reconstructed.',
                             'Animated texture image identity is captured; name animation/frame-selection logic is not reconstructed.',
                             'Native rendering is not yet an exact-pixel comparison.'])
    (root/(a.capture+'-field-profile.json')).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))


if __name__=='__main__':main()
