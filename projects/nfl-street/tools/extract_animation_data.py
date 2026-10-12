"""Extract original GNNE69 animdata.dat by its GameCube FST entry to W:."""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path
ISO_SHA1='813c4511ba3135059a13de424555647e8252254b'
ANIMDATA_SHA256='a0856f4241086f4568fdfce7821decc3710b57ef4bc1c4e8e3055886cf1f242a'
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--iso',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    a.output.resolve().relative_to(Path('/mnt/w/Daywater').resolve())
    digest=hashlib.sha1()
    with a.iso.open('rb') as f:
        for block in iter(lambda:f.read(8*1024*1024),b''):digest.update(block)
        if digest.hexdigest()!=ISO_SHA1:raise ValueError('Wrong game revision/image')
        f.seek(0x424);offset,size=struct.unpack('>II',f.read(8))
        if size>16*1024*1024 or offset+size>a.iso.stat().st_size:raise ValueError('Invalid FST bounds')
        f.seek(offset);fst=f.read(size);count=struct.unpack_from('>I',fst,8)[0]
        if not 1<=count<=size//12:raise ValueError('Invalid FST count')
        names=fst[count*12:];stack=[];matches=[]
        for i in range(1,count):
            while stack and i>=stack[-1][1]:stack.pop()
            word,start,length=struct.unpack_from('>III',fst,i*12);n=word&0xffffff
            name=names[n:names.index(0,n)].decode('ascii');path='/'.join([s[0] for s in stack]+[name])
            if word>>24:stack.append((name,length));continue
            if path=='animdata.dat':matches.append((start,length))
        if len(matches)!=1:raise ValueError('Expected one animdata.dat entry')
        start,length=matches[0]
        if start+length>a.iso.stat().st_size:raise ValueError('Invalid asset bounds')
        f.seek(start);data=f.read(length)
    if hashlib.sha256(data).hexdigest()!=ANIMDATA_SHA256:raise ValueError('Animation data hash differs')
    with a.output.open('xb') as f:f.write(data)
    print(json.dumps(dict(file='animdata.dat',size=len(data),sha256=ANIMDATA_SHA256,iso_sha1=ISO_SHA1)))
if __name__=='__main__':main()
