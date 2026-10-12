"""Integration comparison of native group-30 callback and transition updates.

Source assets are inputs; selected metadata rows and transition outcomes are
not. Optional contact replay joins the already verified selector/completion
with both complete animation group updates and compares entire actor objects.
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

def w(v):return struct.pack('>I',v&0xffffffff)
def data(v):return bytes.fromhex(v) if v else b''
def chunk(v):return w(len(v))+v
def blobs(values):return w(len(values))+b''.join(w(v['identity'])+chunk(data(v['data_hex'])) for v in values)
def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('trace','dol','executable','work-dir'):p.add_argument('--'+name,type=Path,required=True)
    p.add_argument('--contact-executable',type=Path)
    p.add_argument('--windows',action='store_true')
    a=p.parse_args();check_dol(a.dol);raw=a.trace.read_bytes();rows=list(map(json.loads,raw.splitlines()))
    first,last=rows[0],rows[-1]
    if(first['event']!='ready' or first['dol_sha1']!=DOL_SHA1 or first['dolphin']!='2603a' or
       first['savestate_sha256']!='e2faeb8f9481096ff87ae261c8c2943452fbc023cd27510f641808e307603d93' or
       last['event']!='done' or any(last.get(k) for k in ('incomplete_stage','incomplete_setup','incomplete_animation'))):
        raise ValueError('Incomplete reference or wrong identity')
    entries={};returns={};positions={}
    for i,r in enumerate(rows):
        if r['event']=='animation_entry':entries[r['call']]=r;positions[r['call']]=i
        elif r['event']=='animation_return':returns[r['call']]=r
    if entries.keys()!=returns.keys() or not entries:raise ValueError('Incomplete animation calls')
    root=next(r for r in rows if r['event']=='contact_select_entry');bases=struct.unpack('>II',data(root['params_hex'])[:8])
    a.work_dir.mkdir(parents=True,exist_ok=True);checks=[];group_sources={};group_returns={};globals_checks=0;selector_checks=0
    def run(exe,path):
        if a.windows:
            e=subprocess.check_output(['wslpath','-w',str(exe)],text=True).strip()
            f=subprocess.check_output(['wslpath','-w',str(path)],text=True).strip()
            command=['cmd.exe','/d','/c',e,f]
        else:command=[str(exe),str(path)]
        result=subprocess.run(command,capture_output=True,text=True)
        if result.returncode:raise ValueError(f'{path.name}: exit {result.returncode}: {result.stderr.strip()}')
        return result.stdout.strip().splitlines()
    def children(e):return rows[positions[e['call']]+1:rows.index(returns[e['call']])]
    def actor(r,base):return data(r['actor_hex'] if base==bases[0] else r['incoming_hex'])
    def source_fixture(group):
        sub=children(group)
        callback=next(r for r in sub if r['event']=='animation_entry' and r['pc']==0x80068e8c)
        cb=children(callback)
        channel=next(r for r in cb if r['event']=='animation_entry' and r['pc']==0x80173e78)
        bank=next(r for r in children(channel) if r['event']=='animation_return' and r['pc']==0x80175f48)
        program=next(r for r in children(channel) if r['event']=='animation_entry' and r['pc']==0x80174444)
        assets=[dict(identity=r['asset'],data_hex=r['asset_hex']) for r in cb if r['event']=='animation_return' and r['pc']==0x80175c18]
        metadata_table=next(r for r in cb if r['event']=='animation_return' and r['pc']==0x8017859c)
        if len(assets)!=int.from_bytes(data(metadata_table['table_hex'])[:2],'big'):raise ValueError('Incomplete metadata candidate list')
        profile=next(r for r in cb if r['event']=='animation_entry' and r['pc']==0x8004f0d4)
        side=next(r for r in cb if r['event']=='animation_entry' and r['pc']==0x80103e64)
        trans=next(r for r in sub if r['event']=='animation_return' and r['pc']==0x8017869c)
        registry=next((r for r in sub if r['event']=='animation_entry' and r['pc']==0x80175e00),None)
        if not assets:raise ValueError('Missing metadata source assets')
        transition_assets=trans['transition_assets'];targets={}
        for asset in transition_assets:
            for t in asset['targets']:
                if t['identity'] in targets and targets[t['identity']]!=t:raise ValueError('Conflicting target source')
                targets[t['identity']]=t
        payload=w(channel['regs'][2])+chunk(data(bank['bank_hex']))+chunk(data(bank['groups_hex']))
        payload+=chunk(data(bank['program_table_hex']))+w(program['program'])+chunk(data(program['program_hex']))+blobs(assets)
        payload+=chunk(data(profile['profile_hex']) or bytes(16))+chunk(data(profile['speed_table_hex']))
        payload+=w(int(side['side_source_hex'],16)+1 if side['side_source_hex'] is not None else 0)
        payload+=chunk(data(trans['transition_hex']))+blobs(transition_assets)+blobs(list(targets.values()))
        reg=data(registry['registry_hex']) if registry else b'';payload+=w(len(reg)//28)
        for i in range(len(reg)//28):
            b=registry['registry_banks'][i]
            payload+=chunk(reg[i*28:i*28+28])+chunk(data(b['bank_hex']) if b else b'')+chunk(data(b['groups_hex']) if b else b'')
        return payload
    def check(label,mode,e,r,base,source,old_group,initial_actor=None,partner=None,initial_globals=None):
        nonlocal globals_checks
        aa=actor(e,base) if initial_actor is None else initial_actor
        # Contact partner comes from the selected record's initial handle,
        # resolved against the two captured actor headers, not a callback result.
        manager=data(root['manager_hex']);selected=manager[1];handle=manager[8+selected*244+180:12+selected*244+180]
        original_partner=next((actor(e,b) for b in bases if actor(e,b)[:4]==handle),None)
        if original_partner is None:raise ValueError('Selected partner outside bounded pair')
        pp=original_partner if partner is None else partner
        gg=data(e.get('motion_globals_hex')) if initial_globals is None else initial_globals
        if not gg:gg=bytes(8)
        desc=0x1a if base==bases[0] else 0x1b
        payload=mode.encode()+aa+pp+gg+w(desc)+w(30)+w(old_group)+source
        path=a.work_dir/(label+'.bin');path.write_bytes(payload);out=run(a.executable,path)
        actual=data(out[0]);expected=actor(r,base)
        diff=[hex(i) for i,(x,y) in enumerate(zip(actual,expected)) if x!=y]
        if len(actual)!=len(expected) or diff:raise ValueError(f'{label}: actor byte differences {diff}')
        if 'motion_globals_hex' in r and data(out[1])!=data(r['motion_globals_hex']):raise ValueError(f'{label}: motion globals differ')
        if 'motion_globals_hex' in r:globals_checks+=1
        checks.append(dict(label=label,matched_actor_bytes=len(actual),globals_verified='motion_globals_hex' in r))
        return actual,data(out[1])
    for call,e in entries.items():
        if e['pc']!=0x80177ef4:continue
        base=e['regs'][4];source=source_fixture(e);group_sources[base]=source
        aa=actor(e,base);old=next(int.from_bytes(aa[0x9bc+i*100+4:0x9bc+i*100+6],'big') for i in range(4) if int.from_bytes(aa[0x9bc+i*100+6:0x9bc+i*100+8],'big')==3)
        sub=children(e)
        callback=next(r for r in sub if r['event']=='animation_entry' and r['pc']==0x80068e8c)
        transition=next(r for r in sub if r['event']=='animation_entry' and r['pc']==0x80178780)
        check(f'{call:03d}-callback','C',callback,returns[callback['call']],base,source,old)
        check(f'{call:03d}-transition','T',transition,returns[transition['call']],base,source,old)
        check(f'{call:03d}-group','G',e,returns[call],base,source,old)
        group_returns[base]=returns[call]
        trans=next(r for r in sub if r['event']=='animation_return' and r['pc']==0x8017869c)
        for candidate in children(transition):
            if candidate['event']!='animation_entry' or candidate['pc']!=0x80178d8c:continue
            regs=candidate['regs'];path=a.work_dir/(f"selector-{candidate['call']}.bin")
            payload=b'S'+w(regs[3])+w(regs[5])+w(regs[6])+w(candidate['r10'])+w(int(candidate['stack_bank_hex'],16))+blobs(trans['transition_assets'])
            path.write_bytes(payload);out=run(a.executable,path)
            if int(out[0],16)!=returns[candidate['call']]['result']:raise ValueError('Transition row selection differs')
            selector_checks+=1
    if len(checks)!=6:raise ValueError('Expected two complete actor group updates')
    if a.contact_executable:
        # Use the previous verifier to produce private inputs and check native
        # selection/requests. Then consume those native outputs, not originals.
        script=Path(__file__).with_name('verify_contact_selection.py')
        subprocess.run(['python',str(script),'--trace',str(a.trace),'--dol',str(a.dol),
                        '--executable',str(a.contact_executable),'--work-dir',str(a.work_dir),
                        *(['--windows'] if a.windows else [])],check=True,capture_output=True,text=True)
        contact_inputs=list(a.work_dir.glob('contact-input.bin'))
        if len(contact_inputs)!=1:raise ValueError('Expected one successful contact replay input')
        out=run(a.contact_executable,contact_inputs[0]);initial=contact_inputs[0].read_bytes()
        after=next(r for r in reversed(rows) if r['event']=='contact_after_apply')
        globals_before=data(next(e for e in entries.values() if e['pc']==0x80177ef4).get('motion_globals_hex')) or bytes(8)
        manager_offset=60+2*0x14d4+44+4+24+256
        manager=initial[manager_offset:manager_offset+256]
        descriptors=bytes((initial[manager_offset+256],initial[manager_offset+262]))
        payload=b'P'+initial[60:60+2*0x14d4]+manager+globals_before+data(out[3])+data(out[4])+descriptors
        payload+=group_sources[bases[0]]+group_sources[bases[1]]
        path=a.work_dir/'paired-contact.bin';path.write_bytes(payload);native=run(a.executable,path)
        for i,base in enumerate(bases):
            actual=data(native[i]);expected=actor(after,base)
            diff=[hex(j) for j,(x,y) in enumerate(zip(actual,expected)) if x!=y]
            if len(actual)!=len(expected) or diff:raise ValueError(f'Paired contact actor {i} differs at {diff}')
            checks.append(dict(label=f'paired-contact-{i}',matched_actor_bytes=len(actual),globals_verified=False))
        if data(native[2])!=data(after['manager_hex']):raise ValueError('Paired contact manager differs')
        final_globals=group_returns[bases[1]].get('motion_globals_hex')
        if final_globals is not None:
            if data(native[3])!=data(final_globals):raise ValueError('Paired contact motion globals differ')
            globals_checks+=1
    report=dict(trace_sha256=hashlib.sha256(raw).hexdigest(),executable_sha256=hashlib.sha256(a.executable.read_bytes()).hexdigest(),
                checks=checks,actor_comparisons=len(checks),motion_global_comparisons=globals_checks,transition_selection_comparisons=selector_checks,
                limitation='Group 30 / descriptors 0x1A and 0x1B; live transitions have no matching row. Raw bank/asset tables and initial state remain providers.')
    (a.work_dir/(a.trace.stem+'.callbacks-verification.json')).write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
if __name__=='__main__':main()
