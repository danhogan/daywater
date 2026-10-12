"""Integration evidence checker for native stream setup and early event logic.

Positioning/contact completion is explicitly unsupported by the event replay.
No captured service decision is substituted for that branch.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import subprocess
from pathlib import Path
from verify_handle_writer import check_dol
from verify_target_selection import DOL_SHA1


def main() -> None:
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--trace',type=Path,required=True)
    p.add_argument('--dol',type=Path,required=True)
    p.add_argument('--setup-executable',type=Path,required=True)
    p.add_argument('--event-executable',type=Path,required=True)
    p.add_argument('--windows',action='store_true')
    a=p.parse_args(); check_dol(a.dol)
    raw=a.trace.read_bytes(); rows=[json.loads(l) for l in raw.splitlines()]
    if (rows[0]['event']!='ready' or rows[0]['dol_sha1']!=DOL_SHA1 or rows[0]['dolphin']!='2603a' or
        rows[0]['savestate_sha256']!='e2faeb8f9481096ff87ae261c8c2943452fbc023cd27510f641808e307603d93' or
        rows[-1]['event']!='done' or rows[-1]['incomplete_setup'] or rows[-1]['incomplete_stage']):
        raise ValueError('reference identity or complete boundaries differ')
    def invoke(exe,args):
        prefix=[str(exe.resolve())]
        if a.windows:
            path=subprocess.run(['wslpath','-w',str(exe.resolve())],check=True,text=True,capture_output=True).stdout.strip()
            prefix=['cmd.exe','/c',path]
        return subprocess.run(prefix+args,text=True,capture_output=True,timeout=15,cwd=exe.resolve().parent)
    setups=lookups=events=rejected=missing=steering=pairs=pair_lookups=0
    pair_output={}; pair_key={}
    initialized={}; active_stage=None; inside=[]; lookup={}
    for row in rows:
        k=row['event']
        if k=='stream_lookup':
            if row['actor'] in pair_key:
                if row['index']!=pair_key.pop(row['actor']):
                    raise ValueError('paired source data did not select the observed stream key')
                pair_lookups+=1
            output=invoke(a.setup_executable,['L',row['table_hex'],str(row['index'])])
            output.check_returncode(); identity=int(output.stdout.strip(),16)
            if identity!=int(row['table_hex'][8+8*row['index']:16+8*row['index']],16):
                raise ValueError('native lookup differs')
            lookup[row['stream']]=identity; lookups+=1
        elif k=='stream_setup_entry':
            if row['alternate']!=0 or row['stream'] in initialized:
                raise ValueError('unsupported or nested initializer')
            identity=lookup.pop(row['stream'],row['program'])
            if identity!=row['program']: raise ValueError('lookup is not the initialized program')
            result=invoke(a.setup_executable,['S',row['state_hex'],row['data_hex'],hex(identity),hex(row['flags'])])
            result.check_returncode(); initialized[row['stream']]=result.stdout.strip()
        elif k=='stream_setup_return':
            output=initialized.pop(row['stream']).split()
            if output!=[row['state_hex'],str(row['result']),f'{row["flags"]:08x}']:
                raise ValueError('native initializer differs')
            setups+=1
        elif k=='contact_asset' and row['asset']:
            output=invoke(a.setup_executable,['P',row['data_hex'],row['params_hex'],str(row['choice'])])
            output.check_returncode(); pair_output[row['actor']]=output.stdout.strip().split()
            pair_key[row['actor']]=int(pair_output[row['actor']][0][20:24],16)
        elif k=='contact_pair_outputs' and row['matched']:
            if pair_output.pop(row['actor'])!=[row['current_hex'],row['incoming_hex']]:
                raise ValueError('native pair request construction differs')
            pairs+=1
        elif k=='early_stage_entry':
            if active_stage is not None: raise ValueError('nested event stage')
            active_stage=row; inside=[]
        elif k=='early_stage_return':
            if active_stage is None or row['actor']!=active_stage['actor']: raise ValueError('unpaired stage')
            entry=active_stage; active_stage=None
            phase=[r for r in inside if r['event']=='stage_tracking_inputs' and r['pc']==0x8007B9C0]
            field=[r for r in inside if r['event']=='stage_tracking_inputs' and r['pc']==0x8007BA20]
            original_steering=any(r['event']=='early_stage_service' and r['pc']==0x8007BEC0 for r in inside)
            if original_steering and not phase:
                missing+=1; continue
            stage=int(entry['event_hex'][8:12],16)
            if stage==2 and entry['incoming_descriptor_hex'].startswith('1b') and 'incoming_408' not in entry:
                missing+=1; continue
            handle=rows[0]['roster_handles'][(entry['actor']-rows[0]['roster_base'])//0x14D4]
            owns=int(handle in entry['record_handles'])
            enabled=phase[0]['service_value'] if phase else 0
            if original_steering and enabled and entry['actor_mode_hex'][2:4]=='00' and not field:
                missing+=1; continue
            field_hex=field[0]['field_hex'] if field else '00'*12
            result=invoke(a.event_executable,[entry['event_hex'],hex(entry['flags']),entry['request_hex'],
                entry['control_hex'],str(owns),entry['descriptor_hex'],entry['incoming_descriptor_hex'],
                entry['geometry_hex'],entry['incoming_geometry_hex'],entry['record_target_hex'],
                entry['actor_mode_hex'],str(enabled),field_hex,
                str(int(entry.get('incoming_header_hex','00000000')[6:8],16)),
                str(entry.get('incoming_408',0))])
            if result.returncode==3:
                if not(stage==2 and entry['incoming_descriptor_hex'].startswith('1b')):
                    raise ValueError('unexpected unsupported early-stage path')
                rejected+=1
            else:
                result.check_returncode()
                expected=f'{row["event_hex"]} {row["flags"]:08x} {row["request_hex"]} {row["control_hex"]}'
                if result.stdout.strip()!=expected or row['result']!=0:
                    raise ValueError(f'event output differs at frame {row["frames"]}:\n{result.stdout.strip()}\n{expected}')
                events+=1; steering+=original_steering
            inside=[]
        elif active_stage is not None:
            inside.append(row)
    if initialized or pair_output or pair_key or active_stage is not None or events+rejected+missing!=rows[-1]['stage_updates'] or setups<3:
        raise ValueError('incomplete verification')
    print(json.dumps({'trace':a.trace.name,'trace_sha256':hashlib.sha256(raw).hexdigest(),
                      'matched_initializers':setups,'matched_lookups':lookups,'matched_events':events,
                      'matched_steering':steering,'unsupported_contact_events':rejected,
                      'matched_pair_requests':pairs,'data_derived_pair_lookups':pair_lookups,
                      'missing_tracking_inputs':missing},sort_keys=True))


if __name__=='__main__': main()
