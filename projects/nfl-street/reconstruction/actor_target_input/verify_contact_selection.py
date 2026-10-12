"""Integration comparison of native contact selection and bounded completion.

All resolved candidates are inputs. Expected selection, random output and
paired requests are never supplied to native code. Animation-channel effects
are reported separately from the fields owned by this completion component.
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

ACTOR_SIZE=0x14D4
OWNED_CURRENT=((0xC,4),(0x150,12),(0x20E,1),(0x274,8),(0x3F0,4))
OWNED_INCOMING=((0xC,4),(0x20E,1),(0x274,8),(0x3F0,4))

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--trace',type=Path,required=True)
    p.add_argument('--dol',type=Path,required=True)
    p.add_argument('--executable',type=Path,required=True)
    p.add_argument('--windows',action='store_true')
    p.add_argument('--work-dir',type=Path,required=True,help='Private scratch directory on W:')
    a=p.parse_args();check_dol(a.dol)
    raw=a.trace.read_bytes();rows=list(map(json.loads,raw.splitlines()))
    first,last=rows[0],rows[-1]
    if(first['event']!='ready' or first['dol_sha1']!=DOL_SHA1 or first['dolphin']!='2603a' or
       first['savestate_sha256']!='e2faeb8f9481096ff87ae261c8c2943452fbc023cd27510f641808e307603d93' or
       last['event']!='done' or last['incomplete_stage'] or last['incomplete_setup']):
        raise ValueError('Incomplete capture or reference identity differs')
    a.work_dir.mkdir(exist_ok=True,parents=True)
    active=None; table=None; assets=[]; rng_entry=None; rng_return=None; requests=None
    calls=waits=completions=lookups=0;service_changes={};manager=None;stage_entry=None
    for row in rows:
        k=row['event']
        if k=='early_stage_entry':stage_entry=row
        elif k=='contact_select_entry':
            if active:raise ValueError('Nested selection')
            active=row;table=None;assets=[];rng_entry=rng_return=requests=None
            manager=bytes.fromhex(row['manager_hex']) if 'manager_hex' in row else manager
        elif k=='contact_candidates':table=bytes.fromhex(row['table_hex'])
        elif k=='contact_asset_lookup':assets.append(row)
        elif k=='contact_rng_entry':rng_entry=row
        elif k=='contact_rng_return':rng_return=row
        elif k=='contact_pair_outputs' and row['matched']:requests=row
        elif k=='contact_after_apply':
            if not active:raise ValueError('Completion without selection inputs')
            if table is None or len(assets)!=int.from_bytes(table[:2],'big') or not assets:
                raise ValueError('Candidate asset list is incomplete')
            expected=next(r for r in rows if r['event']=='contact_asset' and r['frames']==row['frames'])
            if manager is None:
                # Older capture lacks the complete initial manager block.
                # Only ownership is needed here: take its handle from the
                # pre-call record list, never from the expected completion.
                if stage_entry is None or first['selection']['index']!=0 or len(stage_entry['record_handles'])!=1:
                    raise ValueError('Missing bounded manager ownership inputs')
                initial=bytearray(256);initial[0]=1
                initial[0xBC:0xC0]=struct.pack('>I',stage_entry['record_handles'][0])
                manager=bytes(initial)
            rng=bytes.fromhex(rng_entry['state_hex']) if rng_entry else bytes(24)
            gen=bytes.fromhex(rng_entry['generator_hex']) if rng_entry else bytes(256)
            if len(gen)!=256:raise ValueError('Truncated random generator')
            payload=b''.join(bytes.fromhex(active[key]) for key in
                            ('params_hex','actor_hex','incoming_hex','motion_hex','angle_tolerance_hex'))
            if stage_entry is None:raise ValueError('Missing descriptor inputs')
            payload+=rng+gen+manager+bytes.fromhex(stage_entry['descriptor_hex'])+bytes.fromhex(stage_entry['incoming_descriptor_hex'])+struct.pack('>I',len(assets))
            for i,asset in enumerate(assets):
                key=int.from_bytes(table[6+8*i:8+8*i],'big')&0x7fff
                if key!=asset['key']:raise ValueError('Source-table asset lookup key differs')
                choice=int.from_bytes(table[4+8*i:6+8*i],'big')
                data=bytes.fromhex(asset['data_hex'])
                payload+=struct.pack('>4I',asset['identity'],choice,key,len(data))+data
            scratch=a.work_dir/'contact-input.bin';scratch.write_bytes(payload)
            prefix=[str(a.executable.resolve()),str(scratch.resolve())]
            if a.windows:
                paths=[subprocess.run(['wslpath','-w',str(x.resolve())],check=True,text=True,capture_output=True).stdout.strip()
                       for x in (a.executable,scratch)]
                prefix=['cmd.exe','/c',*paths]
            result=subprocess.run(prefix,check=True,text=True,capture_output=True,timeout=15,cwd=a.work_dir)
            output=result.stdout.strip().splitlines();identity,choice,accepted=output[0].split()
            if int(identity,16)!=expected['asset']:raise ValueError('Native asset selection differs')
            if rng_entry:
                if int(accepted)!=rng_entry['count'] or output[1]!=rng_return['state_hex'] or output[2]!=rng_return['generator_hex']:
                    raise ValueError('Native candidate count or RNG transition differs')
                if int(choice)!=expected['choice']:raise ValueError('Native choice differs')
            elif int(accepted):raise ValueError('Native accepted candidates on original failure')
            calls+=1;lookups+=len(assets)
            if expected['asset']:
                if not requests or output[3:5]!=[requests['current_hex'],requests['incoming_hex']]:
                    raise ValueError('Native requests differ')
                for name,ranges,index in [('actor_hex',OWNED_CURRENT,5),('incoming_hex',OWNED_INCOMING,6)]:
                    native=bytes.fromhex(output[index]);actual=bytes.fromhex(row[name]);before=bytes.fromhex(active[name])
                    if len(native)!=ACTOR_SIZE or len(actual)!=ACTOR_SIZE:raise ValueError('Invalid actor block')
                    for start,size in ranges:
                        if native[start:start+size]!=actual[start:start+size]:
                            raise ValueError(f'Native completion differs in {name} +{start:#x}: '
                                             f'{native[start:start+size].hex()} != {actual[start:start+size].hex()}')
                    owned={i for start,size in ranges for i in range(start,start+size)}
                    changed=[i for i,(old,new) in enumerate(zip(before,actual)) if old!=new and i not in owned]
                    service_changes[name]=changed
                if bytes.fromhex(output[7])[0xD0:0xD4]!=bytes.fromhex(row['manager_hex'])[0xD0:0xD4]:
                    raise ValueError('Manager reset differs')
                completions+=1
            else:waits+=1
            active=None;manager=bytes.fromhex(row['manager_hex'])
    if active or not completions:raise ValueError('No complete successful selection')
    report=dict(trace=a.trace.name,trace_sha256=hashlib.sha256(raw).hexdigest(),
                matched_selections=calls,failed_selections=waits,matched_completions=completions,
                resolved_candidates=lookups,remaining_service_byte_offsets=service_changes)
    report_path=a.work_dir/(a.trace.stem+'.verification.json')
    report_path.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    summary={key:value for key,value in report.items() if key!='remaining_service_byte_offsets'}
    summary['remaining_service_changed_bytes']={key:len(value) for key,value in service_changes.items()}
    summary['report']=str(report_path)
    print(json.dumps(summary,sort_keys=True))

if __name__=='__main__':main()
