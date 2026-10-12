"""Independent original-record checks for native static scene placement.

Reports omissions explicitly. Host trigonometry is an inspection approximation;
this does not verify original animation poses, material state or live pixels.
"""
import argparse
import hashlib
import json
import math
import re
import struct
import subprocess
import xml.etree.ElementTree as ET
from pathlib import Path
from verify_models import DOL_SHA1, triangles, windows
from verify_scene import info

def lookup(data, name):
    at = struct.unpack_from('>I', data, 4)[0]
    seed = hashes = None
    while at < len(data):
        size = struct.unpack_from('>I', data, at+4)[0]
        assert 8 <= size <= len(data)-at
        if data[at:at+4] == b'HSH1':
            assert seed is None
            algorithm, arguments, width, seed = struct.unpack_from('>4I', data, at+8)
            assert (algorithm, arguments, width) == (1, 4, 4)
            count = struct.unpack_from('>H', data, 14)[0]
            hashes = struct.unpack_from('>'+str(count)+'I', data, at+24)
        at += size
    assert hashes is not None
    value = 0
    for char in name.upper().encode('ascii'):
        value = (value*seed+char) & 0xffffffff
    matches = [i for i, h in enumerate(hashes) if h == value]
    return matches[0] if len(matches)==1 else None

def instances(data):
    assert struct.unpack_from('>I', data)[0] == 105
    strides = [8,36,24,24,24,24,24,24,56,36,36,36,36,48,48,60,4]
    arrays, at = [], 156
    for i, stride in enumerate(strides):
        count = struct.unpack_from('>I', data, 16+i*8)[0]
        assert at+count*stride <= len(data)
        arrays.append([data[p:p+stride] for p in range(at, at+count*stride, stride)])
        at += count*stride
    for group, (records, names) in enumerate(((9,3),(11,5),(12,6))):
        for index, record in enumerate(arrays[records]):
            key = struct.unpack_from('>H', record, 24)[0]
            name = arrays[names][key].split(b'\0',1)[0].decode('ascii')
            pose = struct.unpack_from('>6f', record)
            yield dict(group=group, record=index, name=name, position=pose[:3], angles=pose[3:])

def rotation(angles):
    # Original fixed-angle truncation and arithmetic shift; host sine/cosine.
    angles = [((int(a*46603.37890625)//2)*math.tau/16777216) for a in angles]
    sx,sy,sz = map(math.sin, angles)
    cx,cy,cz = map(math.cos, angles)
    x=sx*cy*cz+cx*sy*sz; y=cx*cy*sz+sx*sy*cz
    z=cx*sy*cz-sx*cy*sz; w=cx*cy*cz-sx*sy*sz
    return [[1-2*(y*y+z*z),2*(x*y-z*w),2*(x*z+y*w)],
            [2*(x*y+z*w),1-2*(x*x+z*z),2*(y*z-x*w)],
            [2*(x*z-y*w),2*(y*z+x*w),1-2*(x*x+y*y)]]

def materials(data):
    count, directory = struct.unpack_from('>HH', data, 8)
    for at in range(directory, directory+16*count, 16):
        if data[at:at+4] == b'Matl':
            return struct.unpack_from('>I', data, struct.unpack_from('>I', data, at+4)[0])[0]
    return 0

def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('exe','assets','unpacked','baseline','output','dol'):
        p.add_argument('--'+name,type=Path,required=True)
    args=p.parse_args()
    args.output.resolve().relative_to(Path('/mnt/w/Daywater').resolve())
    assert hashlib.sha1(args.dol.read_bytes()).hexdigest()==DOL_SHA1
    args.output.mkdir(parents=True,exist_ok=True)
    defs=(args.assets/'objdefs.dat').read_bytes(); models=(args.assets/'objmodel.dat').read_bytes()
    venues={info(f.read_bytes())[:-11]:int(f.stem) for f in (args.unpacked/'envrnmt').glob('*.bin')
            if (info(f.read_bytes()) or '').endswith('_rField.ea3')}
    report=[]
    for venue, entry in sorted(venues.items()):
        rows=[]; expected=[]; count=draws=material_count=0
        for layer in [entry]+list(range(entry+4,entry+9)):
            data=(args.unpacked/'envrnmt'/f'{layer:05d}.bin').read_bytes()
            dump,d,n=triangles(data,True);count+=n;draws+=d;material_count+=materials(data)
        baseline=(args.baseline/(venue+'.mesh')).read_bytes()
        for record in instances((args.unpacked/'envrnmt'/f'{entry+1:05d}.bin').read_bytes()):
            definition=lookup(defs,record['name']);record['definition']=definition
            if definition is None:
                record['status']='unsupported';record['reason']='definition lookup';rows.append(record);continue
            xml=(args.unpacked/'objdefs'/f'{definition:05d}.bin').read_bytes().rstrip(b'\0')
            # Some original definitions have malformed unrelated XML. Parse
            # only the render opening tag independently, after removing comments.
            cleaned=re.sub(rb'<!--.*?-->',b'',xml,flags=re.S)
            tag=re.search(rb'<render\s[^>]*>',cleaned)
            render=ET.fromstring(tag[0].rstrip(b'>').rstrip(b'/')+b'/>') if tag else None
            if render is None:
                record['status']='nonvisual';rows.append(record);continue
            reference=render.attrib['tideRef'];index=lookup(models,reference);record['model']=index
            if index is None:
                record['status']='unsupported';record['reason']='model lookup';rows.append(record);continue
            data=(args.unpacked/'objmodel'/f'{index:05d}.bin').read_bytes()
            try: dump,d,n=triangles(data,True,bind_pose=True)
            except AssertionError:
                record['status']='unsupported';record['reason']='matrix/skin/geometry';rows.append(record);continue
            skeleton_tag=re.search(rb'<skeleton\s[^>]*>',cleaned)
            if skeleton_tag:
                skeleton=ET.fromstring(skeleton_tag[0].rstrip(b'>').rstrip(b'/')+b'/>')
                skeleton_index=lookup(models,skeleton.attrib['tideRef'])
                assert skeleton_index is not None
                external=(args.unpacked/'objmodel'/f'{skeleton_index:05d}.bin').read_bytes()
                assert external[:4]==b'SKL1'
                bones=struct.unpack_from('>H',external,6)[0];table=struct.unpack_from('>I',external,12)[0]
                external_positions={external[table+i*20+4:table+(i+1)*20].split(b'\0')[0]:external[16+i*16:28+i*16] for i in range(bones)}
                section_count,directory=struct.unpack_from('>HH',data,8)
                sections={data[at:at+4]:struct.unpack_from('>I',data,at+4)[0] for at in range(directory,directory+section_count*16,16)}
                if b'Skel' in sections and b'Clst' in sections:
                    bone_count,names,translations=struct.unpack_from('>3I',data,sections[b'Skel'])
                    cluster_rows,clusters=struct.unpack_from('>II',data,sections[b'Clst'])
                    checked=set()
                    for cluster in range(1,clusters):
                        bone=data[cluster_rows+cluster*4]
                        name=data[names+bone*16:names+(bone+1)*16].split(b'\0')[0]
                        assert name in external_positions
                        assert data[translations+bone*16:translations+bone*16+12]==external_positions[name]
                        checked.add(name.decode('ascii'))
                    record['external_skeleton']=skeleton_index
                    record['referenced_bone_translations_match']=sorted(checked)
            matrix=rotation(record['angles']);payload=bytearray(dump[12:])
            for at in range(0,len(payload),64):
                mat=struct.unpack_from('<I',payload,at)[0];struct.pack_into('<I',payload,at,mat+material_count)
                for v in range(3):
                    pos=struct.unpack_from('<3f',payload,at+4+v*20)
                    transformed=[record['position'][j]+sum(matrix[j][k]*pos[k] for k in range(3)) for j in range(3)]
                    struct.pack_into('<3f',payload,at+4+v*20,*transformed)
            expected.append(payload);count+=n;draws+=d;material_count+=materials(data)
            record['status']='rendered';record['triangles']=n;rows.append(record)
        bmp=args.output/(venue+'.bmp');mesh=args.output/(venue+'.mesh')
        run=subprocess.run(['cmd.exe','/d','/c',windows(args.exe),windows(args.assets/'envrnmt.dat'),venue,
                            windows(bmp),'--headless','--props',windows(mesh)],capture_output=True,text=True)
        (args.output/(venue+'.log')).write_text(run.stdout+run.stderr)
        assert run.returncode==0, (venue,run.stdout,run.stderr)
        native=mesh.read_bytes();assert struct.unpack_from('<II',native,4)==(count,draws)
        assert native[12:len(baseline)]==baseline[12:]
        actual=native[len(baseline):];target=b''.join(expected);assert len(actual)==len(target)
        maximum=0
        for at in range(0,len(target),64):
            assert actual[at:at+4]==target[at:at+4]
            for v in range(3):
                assert actual[at+16+v*20:at+24+v*20]==target[at+16+v*20:at+24+v*20]
                xyz=struct.unpack_from('<3f',actual,at+4+v*20);ref=struct.unpack_from('<3f',target,at+4+v*20)
                maximum=max(maximum,*[abs(a-b) for a,b in zip(xyz,ref)])
        assert maximum<0.0001,(venue,maximum)
        counters={status:sum(r['status']==status for r in rows) for status in ('rendered','nonvisual','unsupported')}
        matched=re.search(r'Scene instances (\d+): rendered (\d+), nonvisual (\d+), unsupported (\d+)',run.stdout)
        assert matched and tuple(map(int,matched.groups()))==(len(rows),*counters.values())
        report.append(dict(venue=venue,triangles=count,instances=len(rows),**counters,max_position_difference=maximum,
                           baseline_geometry_exact=True,material_and_uv_exact=True,records=rows))
    result=dict(confidence='candidate',dol_sha1=DOL_SHA1,live_original_comparison=False,
                rotation='original formula with approximate host trigonometry',
                cluster_pose='stored Skel/Clst palette and indexed matrix loads; no animation input',cases=report)
    (args.output/'verification.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps([{k:v for k,v in r.items() if k!='records'} for r in report]))

if __name__=='__main__':main()
