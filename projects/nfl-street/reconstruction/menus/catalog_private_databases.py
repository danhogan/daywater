#!/usr/bin/env python3
"""Inventory unpacked original DBv8 headers without exporting proprietary rows."""
from __future__ import annotations
import argparse,hashlib,json,struct
from pathlib import Path
from verify_controller_checks import PRIVATE,private

def inspect(path: Path):
 with path.open('rb') as f:
  header=f.read(28)
  if header[:4]!=b'DB\0\x08':return None
  if len(header)!=28:raise ValueError('Truncated DB header')
  size=struct.unpack_from('>I',header,8)[0];count=struct.unpack_from('>I',header,16)[0]
  if size!=path.stat().st_size or not 0<=count<=64:raise ValueError('Invalid DB header bounds')
  directory=f.read(count*8)
  if len(directory)!=count*8:raise ValueError('Truncated DB table directory')
  entries=header[24:28]+directory
  rows=[];base=28+count*8
  for i in range(count):
   tag,offset=struct.unpack_from('>II',entries,i*8)
   at=base+offset
   if at>size-36:raise ValueError('Invalid DB table offset')
   f.seek(at);table=f.read(36);fields=table[24];n=fields*16
   if at+36+n>size:raise ValueError('Invalid DB field bounds')
   raw=f.read(n);row_bytes=struct.unpack_from('>I',table,4)[0];allocated,used=struct.unpack_from('>HH',table,16)
   if used>allocated or not row_bytes:raise ValueError('Invalid DB row metadata')
   # Allocation capacity need not be serialized: template entry 1 contains
   # unused tables with capacity but no allocated record payload. Inventory
   # the bounded metadata without claiming the original allocation lifecycle.
   next_offsets=[base+struct.unpack_from('>I',entries,j*8+4)[0] for j in range(count)]
   end=min([q for q in next_offsets if q>at]+[size])
   available=max(0,end-(at+36+n))
   rows.append({'tag_hex':f'{tag:08x}','raw_tag':tag.to_bytes(4,'big').decode('ascii','replace'),'record_bytes':row_bytes,'used':used,'allocated':allocated,'serialized_bytes_available':available,'used_records_fit':row_bytes*used<=available,'allocated_records_fit':row_bytes*allocated<=available,'fields':[{'type':struct.unpack_from('>I',raw,q)[0],'bit':struct.unpack_from('>I',raw,q+4)[0],'tag_hex':raw[q+8:q+12].hex(),'width':struct.unpack_from('>I',raw,q+12)[0]} for q in range(0,n,16)]})
 return {'path':str(path),'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'tables':rows}

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--root',type=Path,default=PRIVATE/'private/nflstreet/GNNE69-r0/unpacked');p.add_argument('--output',type=Path,default=PRIVATE/'research-temp/nflstreet-team-screen-20261010/database-inventory.json');a=p.parse_args();root=private(a.root);out=private(a.output);out.parent.mkdir(parents=True,exist_ok=True)
 record={'status':'running','scope':'Original unpacked DBv8 header/table/field metadata only; no inferred registration, joins or STAT semantics.','records':[],'scanned':0};out.write_text(json.dumps(record,indent=2)+'\n')
 try:
  for file in sorted(root.rglob('*.bin')):
   record['scanned']+=1;item=inspect(file)
   if item:record['records'].append(item)
  record['status']='passed'
 except Exception as e:record.update(status='failed',error=str(e));raise
 finally:out.write_text(json.dumps(record,indent=2)+'\n')
 print(f"DB header inventory: {record['scanned']} files scanned; {len(record['records'])} DBv8 blocks; metadata private on W.")
if __name__=='__main__':main()
