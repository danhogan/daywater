"""Bounded original player TMdl command catalog; no runtime policy."""
from pathlib import Path
import struct,hashlib,json,collections
def model(p):
 b=p.read_bytes()
 if b[:4]!=b'TMdl':return None
 u16=lambda o:struct.unpack_from('>H',b,o)[0];u32=lambda o:struct.unpack_from('>I',b,o)[0]
 sec={b[i:i+4]:u32(i+4) for i in range(u16(10),u16(10)+u16(8)*16,16)}
 name=b[sec[b'Info']:].split(b'\0')[0].decode();rows,count,arr,ac=struct.unpack_from('>4I',b,sec[b'Geom']);prims=[]
 for rowi in range(count):
  row=rows+rowi*24;attrs=[arr+i*12 for i in range(u16(row+14),u16(row+14)+b[row+16])];sz=sum((b[a+11]&15)-1 for a in attrs)+(b[row+17]&1);at=u32(row);end=at+u16(row+4)*32;slots={}
  while at<end:
   cmd=b[at]
   if cmd==0:at+=1;continue
   if cmd in [0x20,0x28]:
    src,packed=struct.unpack_from('>HH',b,at+1);addr=packed&4095;length=1+(packed>>12)
    if cmd==0x20:
     if length!=12 or addr%4:raise ValueError('position indexed load')
     slots[addr//4]=src
    at+=5;continue
   if cmd not in [0x80,0x90,0x98,0xa0]:raise ValueError((p,at,cmd))
   n=u16(at+1);data=b[at:at+3+n*sz];prims.append(dict(row=rowi,offset=at,attrs=attrs,size=sz,payload=data,slots=dict(slots),vertices=n,material=u16(row+6),matrix=b[row+17]&1));at+=len(data)
 return dict(path=p,bytes=b,name=name,sections=sec,primitives=prims)
