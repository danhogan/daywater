#!/usr/bin/env python3
"""Find direct linked PowerPC branches in original DOL text sections.

Indirect calls and function ownership are not inferred. Results are instruction
references only; keep game-specific output on private W storage.
"""
import argparse,hashlib,json,struct
from pathlib import Path

def direct_calls(data,targets):
 if len(data)<256:raise ValueError('Truncated DOL header')
 refs=[]
 for section in range(7):
  off,address,size=(struct.unpack_from('>I',data,table+section*4)[0] for table in (0,0x48,0x90))
  if not size:continue
  if off<256 or off>len(data) or size>len(data)-off or address%4 or size%4 or address+size>2**32:raise ValueError('Invalid DOL text section')
  for relative in range(0,size,4):
   word=struct.unpack_from('>I',data,off+relative)[0];opcode=word>>26
   if not word&1 or opcode not in (16,18):continue
   bits=26 if opcode==18 else 16;mask=0x03fffffc if opcode==18 else 0x0000fffc
   delta=word&mask
   if delta&(1<<(bits-1)):delta-=1<<bits
   site=address+relative;target=(delta if word&2 else site+delta)&0xffffffff
   if target in targets:refs.append({'site':f'{site:08x}','target':f'{target:08x}','word':f'{word:08x}','section':section,'conditional':opcode==16})
 return refs

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('dol',type=Path);p.add_argument('targets',nargs='+',type=lambda x:int(x,16));p.add_argument('--output',required=True,type=Path);a=p.parse_args()
 root=Path('/mnt/w/Daywater').resolve();source=a.dol.resolve();out=a.output.resolve()
 if not source.is_relative_to(root) or not out.is_relative_to(root):raise ValueError('Game input and diagnostic output must stay on private W')
 b=source.read_bytes();report={'dol_sha1':hashlib.sha1(b).hexdigest(),'scope':'Direct linked instruction references; indirect calls and function ownership unproven','references':direct_calls(b,set(a.targets))}
 out.parent.mkdir(parents=True,exist_ok=True);out.write_text(json.dumps(report,indent=2)+'\n');print(f"{len(report['references'])} direct instruction references recorded privately")
if __name__=='__main__':main()
