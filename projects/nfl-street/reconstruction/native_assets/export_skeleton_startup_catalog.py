#!/usr/bin/env python3
"""Export source constants privately; the native runtime reads NSC1, not a DOL."""
import hashlib,struct
from pathlib import Path
from verify_controller_checks import BASELINES,PRIVATE,private

def export(output:Path):
 source=BASELINES['dol'][0];data=source.read_bytes()
 if hashlib.sha1(data).hexdigest()!='46e3de854998a0f66c9be8c72a1164d22cd29eb4':raise ValueError('Unexpected original revision')
 def span(address,length):
  for i in range(18):
   offset=struct.unpack_from('>I',data,4*i)[0];start=struct.unpack_from('>I',data,0x48+4*i)[0];size=struct.unpack_from('>I',data,0x90+4*i)[0]
   if start<=address and address+length<=start+size:
    at=offset+address-start
    if at+length>len(data):raise ValueError('Invalid original section bounds')
    return data[at:at+length]
  raise ValueError('Constant range is not in original sections')
 result=b'NSC1'+struct.pack('>III',1,384,114)+span(0x80246490,384)+span(0x80246610,12)+span(0x8024661c,114)+span(0x80246690,4)
 output=private(output);output.parent.mkdir(parents=True,exist_ok=True);output.write_bytes(result)
 return hashlib.sha256(result).hexdigest()
if __name__=='__main__':
 import argparse
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,default=PRIVATE/'private/nflstreet/GNNE69-r0/native/skeleton-startup.nsc1');args=parser.parse_args();print('Private skeleton catalog SHA256 '+export(args.output))
