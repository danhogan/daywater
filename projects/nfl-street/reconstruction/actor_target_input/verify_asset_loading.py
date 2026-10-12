"""Integration: derive source tables natively from a raw private loaded arena.

No resolved assets, selected row, duration result or reference actor outputs
are replay inputs. Controlled mutations are labeled in the report.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
import subprocess
from pathlib import Path
from verify_handle_writer import check_dol
from verify_animation_callbacks import w, chunk, data
from verify_target_selection import DOL_SHA1

def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('trace','dol','executable','work-dir'):p.add_argument('--'+name,type=Path,required=True)
    p.add_argument('--animdata',type=Path,help='private original animdata.dat; replace every resident context-one bank with offset-form disc bytes')
    p.add_argument('--windows',action='store_true');a=p.parse_args();check_dol(a.dol)
    raw=a.trace.read_bytes();rows=[json.loads(x) for x in raw.splitlines()]
    if rows[0]['dol_sha1']!=DOL_SHA1 or rows[0]['savestate_sha256']!='e2faeb8f9481096ff87ae261c8c2943452fbc023cd27510f641808e307603d93' or rows[0]['dolphin']!='2603a':raise ValueError('wrong state/build')
    if rows[-1]['event']!='done' or any(rows[-1].get(k) for k in ('incomplete_animation','incomplete_stage','incomplete_setup','incomplete_call')):raise ValueError('incomplete capture')
    arena_event=next(r for r in rows if r['event']=='loaded_assets');original=data(arena_event['arena_hex']);registry=data(arena_event['registry_hex'])
    entries={r['call']:r for r in rows if r['event']=='animation_entry'};returns={r['call']:r for r in rows if r['event']=='animation_return'}
    if entries.keys()!=returns.keys():raise ValueError('unpaired calls')
    a.work_dir.mkdir(parents=True,exist_ok=True);checks=[]
    def run(payload,name):
        path=a.work_dir/(name+'.bin');path.write_bytes(payload)
        exe=str(a.executable);fixture=str(path)
        if a.windows:
            exe=subprocess.check_output(['wslpath','-w',exe],text=True).strip();fixture=subprocess.check_output(['wslpath','-w',fixture],text=True).strip()
            cmd=['cmd.exe','/d','/c',exe,fixture]
        else:cmd=[exe,fixture]
        r=subprocess.run(cmd,capture_output=True,text=True)
        if r.returncode:raise ValueError(f'{name}: native exit {r.returncode} {r.stderr}')
        return r.stdout.strip().splitlines()
    def actor(r,base):
        for field in ('actor_hex','incoming_hex'):
            value=data(r[field]);identity=int.from_bytes(value[0x31c:0x320],'big')-0x4d8
            if identity==base:return value
        raise ValueError('actor outside captured pair')
    experiments=[r for r in rows if r['event']=='transition_experiment']
    disc_banks=[]
    if a.animdata:
        disc=a.animdata.read_bytes()
        if hashlib.sha256(disc).hexdigest()!='a0856f4241086f4568fdfce7821decc3710b57ef4bc1c4e8e3055886cf1f242a':raise ValueError('Wrong animation asset revision')
        pointers=sorted({int.from_bytes(registry[i+12:i+16],'big') for i in range(0,len(registry),28)
                         if registry[i+18]==1 and arena_event['base']<=int.from_bytes(registry[i+12:i+16],'big')<arena_event['base']+len(original)})
        for i in range(0,len(registry),28):
            if registry[i+18]!=1 or int.from_bytes(registry[i+20:i+22],'big')!=1:continue
            identity=int.from_bytes(registry[i+12:i+16],'big');offset=identity-arena_event['base'];bank=original[offset:offset+32]
            # Resource identity (context, bank tag, frame count) selects a
            # unique offset-form bank. No captured tables/programs are copied.
            prefix=bank[:6]+b'\x00\x0b';found=[];pos=0
            while True:
                pos=disc.find(prefix,pos)
                if pos<0:break
                if pos%32==0:found.append(pos)
                pos+=1
            if len(found)!=1:raise ValueError('disc bank identity is ambiguous/missing')
            limit=next((v for v in pointers if v>identity),identity+0x100000)
            length=min(limit-identity,len(disc)-found[0])
            disc_banks.append((identity,disc[found[0]:found[0]+length],found[0]))
    for call,e in entries.items():
        if e['pc'] not in (0x80068e8c,0x80178780,0x80177ef4):continue
        # Complete group scope contains callback source environment; derive
        # bank/program/candidates/transition descriptors inside C.
        scope=next(g for g in entries.values() if g['pc']==0x80177ef4 and rows.index(g)<=rows.index(e)<rows.index(returns[g['call']]))
        sub=rows[rows.index(scope):rows.index(returns[scope['call']])]
        callback=next(r for r in sub if r['event']=='animation_entry' and r['pc']==0x80068e8c)
        profile=next(r for r in sub if r['event']=='animation_entry' and r['pc']==0x8004f0d4)
        side=next(r for r in sub if r['event']=='animation_entry' and r['pc']==0x80103e64)
        base=scope['regs'][4];initial=actor(e,base);start=actor(scope,base)
        old=next(int.from_bytes(start[0x9bc+i*100+4:0x9bc+i*100+6],'big') for i in range(4) if int.from_bytes(start[0x9bc+i*100+6:0x9bc+i*100+8],'big')==3)
        cbactor=actor(callback,base);bank=int.from_bytes(cbactor[0x3f2:0x3f4],'little');clip=int.from_bytes(cbactor[0x3f0:0x3f2],'little')
        context=int.from_bytes(initial[0x4da:0x4dc],'big');desc=0x1a if e['actor_hex']==initial.hex() else 0x1b
        manager=next(r for r in rows if r['event']=='contact_select_entry');m=data(manager['manager_hex']);handle=m[8+m[1]*244+180:12+m[1]*244+180]
        partner=next(data(e[k]) for k in ('actor_hex','incoming_hex') if data(e[k])[:4]==handle)
        mode={0x80068e8c:b'C',0x80178780:b'T',0x80177ef4:b'G'}[e['pc']]
        arena=bytearray(original)
        # Group includes the controlled row edit later in its call tree.
        for exp in experiments:
            if rows.index(exp)<rows.index(returns[call]):
                offset=exp['row']-arena_event['base'];arena[offset:offset+16]=data(exp['after_hex'])
        payload=(b'F' if a.animdata else b'A')+mode+initial+partner+data(e['motion_globals_hex'])+w(desc)+w(30)+w(old)+w(bank)+w(clip)+w(context)
        payload+=w(arena_event['base'])+chunk(bytes(len(arena)) if a.animdata else arena)+chunk(registry)+chunk(data(profile['profile_hex']) or bytes(16))+chunk(data(profile['speed_table_hex']))
        payload+=w(int(side['side_source_hex'],16)+1 if side['side_source_hex'] is not None else 0)
        if a.animdata:
            payload+=w(len(disc_banks))
            for identity,raw_bank,_ in disc_banks:
                raw_bank=bytearray(raw_bank)
                for exp in experiments:
                    if rows.index(exp)<rows.index(returns[call]) and identity<=exp['row']<identity+len(raw_bank):
                        off=exp['row']-identity;raw_bank[off:off+12]=data(exp['after_hex'])[:12]
                payload+=w(identity)+chunk(raw_bank)
        out=run(payload,f'{call:03d}-{mode.decode()}');expected=actor(returns[call],base);actual=data(out[0])
        diff=[hex(i) for i,(x,y) in enumerate(zip(actual,expected)) if x!=y]
        if len(actual)!=len(expected) or diff:raise ValueError(f'{call}: actor differs {diff}')
        if data(out[1])!=data(returns[call]['motion_globals_hex']):raise ValueError('globals differ')
        checks.append(dict(call=call,mode=mode.decode(),matched_actor_bytes=len(actual)))
    for e in entries.values():
        if e['pc']!=0x8017749c:continue
        out=run(b'D'+chunk(data(e['program_hex'])),f"duration-{e['call']}")
        expected=struct.pack('>f',struct.unpack('>d',data(returns[e['call']]['f1_hex']))[0]).hex()
        if out[0]!=expected:raise ValueError(f'original duration {expected}, native {out[0]}')
        checks.append(dict(call=e['call'],mode='duration',result=expected))
    report=dict(trace_sha256=hashlib.sha256(raw).hexdigest(),executable_sha256=hashlib.sha256(a.executable.read_bytes()).hexdigest(),checks=checks,
                disc_banks_loaded=len(disc_banks),animdata_sha256=hashlib.sha256(a.animdata.read_bytes()).hexdigest() if a.animdata else None,
                controlled_transition=bool(experiments),spawn_calls=sum(e['pc']==0x8017749c for e in entries.values()),
                limitation='Initial registry supplied. Disc mode loads bank bytes from a privately extracted file; frame payload relocation/rendering and archive startup remain open. Forced rows demonstrate original branch mechanics, not natural gameplay eligibility. Disc archive and cold-start loading remain open.')
    (a.work_dir/'asset-verification.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
if __name__=='__main__':main()
