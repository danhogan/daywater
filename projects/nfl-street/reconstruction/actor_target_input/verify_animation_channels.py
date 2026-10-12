"""Compare allocation and animation state primitives with fresh original calls.

Integration verification. Original allocation results are comparison outputs,
never replay inputs. Source banks, group tables and clip programs are data
providers. Full callback/transition orchestration is outside this component.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
import subprocess
from pathlib import Path
from verify_handle_writer import check_dol
from verify_target_selection import DOL_SHA1

def word(v):return struct.pack('>I',v&0xffffffff)
def single(hex_value):return struct.pack('>f',struct.unpack('>d',bytes.fromhex(hex_value))[0])
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--trace',type=Path,required=True)
    p.add_argument('--dol',type=Path,required=True)
    p.add_argument('--executable',type=Path,required=True)
    p.add_argument('--work-dir',type=Path,required=True)
    p.add_argument('--windows',action='store_true')
    a=p.parse_args();check_dol(a.dol)
    raw=a.trace.read_bytes();rows=list(map(json.loads,raw.splitlines()))
    if(rows[0]['event']!='ready' or rows[0]['dol_sha1']!=DOL_SHA1 or rows[0]['dolphin']!='2603a' or
       rows[0]['savestate_sha256']!='e2faeb8f9481096ff87ae261c8c2943452fbc023cd27510f641808e307603d93' or
       rows[-1]['event']!='done' or rows[-1]['incomplete_stage'] or rows[-1]['incomplete_setup']):
        raise ValueError('Incomplete reference capture or wrong saved state')
    params=bytes.fromhex(next(r['params_hex'] for r in rows if r['event']=='contact_select_entry'))
    bases=struct.unpack('>II',params[:8]);entries={};returns={};positions={}
    for i,r in enumerate(rows):
        if r['event']=='animation_entry':entries[r['call']]=r;positions[r['call']]=i
        elif r['event']=='animation_return':returns[r['call']]=r
    if not entries or entries.keys()!=returns.keys():raise ValueError('Incomplete animation calls')
    def block(r,address,size):
        for base,name in zip(bases,('actor_hex','incoming_hex')):
            if base<=address and address+size<=base+0x14d4:
                data=bytes.fromhex(r[name]);return data[address-base:address-base+size]
        raise ValueError(f'Object outside captured actors: {address:#x}')
    a.work_dir.mkdir(parents=True,exist_ok=True);checks=[]
    def run(label,payload,expected,index=None,flags=None):
        path=a.work_dir/(label+'.bin');path.write_bytes(payload)
        if a.windows:
            exe=subprocess.check_output(['wslpath','-w',str(a.executable)],text=True).strip()
            arg=subprocess.check_output(['wslpath','-w',str(path)],text=True).strip()
            result=subprocess.run(['cmd.exe','/d','/c',exe,arg],capture_output=True,text=True,check=True)
        else:result=subprocess.run([str(a.executable),str(path)],capture_output=True,text=True,check=True)
        lines=result.stdout.strip().splitlines();got_index,got_flags=lines[0].split()
        actual=bytes.fromhex(lines[1]);diff=[i for i,(x,y) in enumerate(zip(actual,expected)) if x!=y]
        if len(actual)!=len(expected) or diff or (index is not None and int(got_index)!=index) or (flags is not None and int(got_flags,16)!=flags):
            raise ValueError(f'{label}: index {got_index}/{index}, flags {got_flags}/{flags}, byte differences {diff}')
        checks.append(dict(call=label,matched_bytes=len(expected),index=int(got_index)))
    for call,e in entries.items():
        r=returns[call];regs=e['regs'];pc=e['pc'];begin=positions[call]
        end=rows.index(r);children=rows[begin+1:end];label=f'{call:03d}-{pc:08x}'
        if pc==0x80173c24:
            h=bytes.fromhex(e['header_hex']);count=int.from_bytes(h[4:6],'big')
            run(label,b'A'+word(count)+bytes.fromhex(e['channels_hex']),
                block(r,regs[1],count*124),r['result'] if r['result']<0x80000000 else r['result']-0x100000000)
        elif pc==0x80177acc:
            run(label,b'M'+block(e,regs[0],404),block(r,regs[0],404),r['result'])
        elif pc==0x80179288:
            run(label,b'B'+block(e,regs[0],20)+single(e['f1_hex'])+single(e['f2_hex'])+word(regs[1]),block(r,regs[0],20))
        elif pc==0x80173e78:
            if regs[5]!=bases[0] and regs[5]!=bases[1]:raise ValueError('Unexpected alternate actor')
            h=block(e,regs[0],16);count=int.from_bytes(h[4:6],'big')
            bank=next(c for c in children if c['event']=='animation_return' and c['pc']==0x80175f48)
            stream=next(c for c in children if c['event']=='animation_entry' and c['pc']==0x80174444)
            groups=bytes.fromhex(bank['groups_hex']);program=bytes.fromhex(stream['program_hex'])
            actor_base=regs[5];flags=int.from_bytes(block(e,actor_base+12,4),'big')
            payload=b'C'+h+word(count)+block(e,regs[1],count*124)+word(regs[2])+bytes.fromhex(bank['bank_hex'])
            payload+=word(len(groups))+groups+word(regs[3])+word(regs[4])+word(regs[6])+word(stream['program'])
            payload+=word(len(program))+program+single(e['f1_hex'])+word(flags)
            run(label,payload,block(r,regs[1],count*124),r['result'],int.from_bytes(block(r,actor_base+12,4),'big'))
        elif pc==0x80177ef4:
            callback=next(c for c in children if c['event']=='animation_entry' and c['pc']==0x80068e8c)
            table=next(c for c in children if c['event']=='animation_return' and c['pc']==0x8017869c)
            trans=bytes.fromhex(table['transition_hex'])
            payload=b'G'+block(e,regs[0],404)+word(regs[3])+single(e['f1_hex'])+word(len(trans))+trans
            manager=block(callback,regs[0],404);slot=next(i for i in range(4) if int.from_bytes(manager[i*100+6:i*100+8],'big')==3)
            run(label+'-pre-callback',payload,manager,slot)
    if len(checks)<10:raise ValueError('Too few animation comparisons')
    report=dict(trace_sha256=hashlib.sha256(raw).hexdigest(),executable_sha256=hashlib.sha256(a.executable.read_bytes()).hexdigest(),
                checks=checks,comparisons=len(checks),matched_bytes=sum(c['matched_bytes'] for c in checks),
                limitation='Source bank/transition resolution and game-specific callbacks remain original providers; this is not a complete animation loop.')
    (a.work_dir/(a.trace.stem+'.verification.json')).write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))
if __name__=='__main__':main()
