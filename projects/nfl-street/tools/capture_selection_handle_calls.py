"""Capture original GNNE69 record-handle clear/copy calls and their returns.

Integration tool: requires Dolphin 2603a and the private first-down state.
Breakpoints normally observe original calls. The explicit transition experiment
changes one source row in an ephemeral Dolphin process; it never saves that state.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import socket
import subprocess
import sys
import tempfile
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[3]))
sys.path.insert(0, str(Path(__file__).resolve().parent))
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "reconstruction/actor_target_input"))

import src.core.dolphin.runner as runner  # noqa: E402
import src.core.dolphin.session as session_module  # noqa: E402
from src.core.dolphin.debugger import GDBError  # noqa: E402
from src.core.dolphin.input import InputCommand, InputSequence  # noqa: E402
from trace_score_path import verified_iso  # noqa: E402
from verify_handle_writer import CALLS, check_dol  # noqa: E402

MANAGER_SLOT = 0x803A9A40 - 0x73D0
FIRST_DOWN_SHA256 = "e2faeb8f9481096ff87ae261c8c2943452fbc023cd27510f641808e307603d93"


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("iso", "savestate", "dol", "dolphin-binary", "output"):
        parser.add_argument(f"--{name}", required=True, type=Path)
    parser.add_argument("--seconds", type=float, default=45)
    parser.add_argument("--captures", type=int, default=6)
    parser.add_argument("--stick-x", type=float, default=1.0)
    parser.add_argument("--stick-y", type=float, default=0.5)
    parser.add_argument("--gdb-port", type=int, default=2493)
    parser.add_argument("--trace-handoff", action="store_true",
                        help="also capture actor-to-record reassignment arguments")
    parser.add_argument("--trace-event-build", action="store_true",
                        help="capture descriptor-to-event construction for slots 12 and 7")
    parser.add_argument("--trace-event-process", action="store_true",
                        help="also stop at event processing entries (slower)")
    parser.add_argument("--trace-event-flags", action="store_true",
                        help="capture the command stores setting event flag bits 1 and 4")
    parser.add_argument("--trace-event-gate", action="store_true",
                        help="capture first waiting and ready stage-three handoff gates")
    parser.add_argument("--trace-command-timing", action="store_true",
                        help="capture timing updates after a watched readiness command")
    parser.add_argument("--trace-stream-setup", action="store_true")
    parser.add_argument("--trace-event-stages", action="store_true")
    parser.add_argument("--trace-contact-service", action="store_true")
    parser.add_argument("--trace-animation", action="store_true",
                        help="capture allocation/state calls after successful contact selection")
    parser.add_argument("--trace-animation-details", action="store_true",
                        help="also capture callback assets, numeric sources and transition rows")
    parser.add_argument("--trace-loaded-assets", action="store_true", help="capture raw loaded bank arena and registry")
    parser.add_argument("--experiment-transition-clip", action="store_true", help="force one source row to exercise original mode-3 spawning; controlled experiment")
    parser.add_argument("--experiment-transition-row", type=int, default=0, help="source transition row used for the controlled experiment")
    args = parser.parse_args()
    if not 0 <= args.experiment_transition_row < 256:
        parser.error("invalid transition experiment row")
    if (args.trace_loaded_assets or args.experiment_transition_clip) and not args.trace_animation_details:
        parser.error("asset capture/transition experiment requires animation details")
    if args.trace_event_process and not args.trace_event_build:
        parser.error("trace-event-process requires trace-event-build")
    if args.trace_command_timing and not args.trace_event_flags:
        parser.error("trace-command-timing requires trace-event-flags")
    if args.trace_contact_service and not args.trace_event_stages:
        parser.error("trace-contact-service requires trace-event-stages")
    if args.trace_animation and not args.trace_contact_service:
        parser.error("trace-animation requires trace-contact-service")
    if args.trace_animation_details and not args.trace_animation:
        parser.error("trace-animation-details requires trace-animation")
    if (args.output.exists() or not 0 < args.seconds <= 360 or
            not 1 <= args.captures <= 30 or not 0 < args.gdb_port < 65536 or
            not 0 <= args.stick_x <= 1 or not 0 <= args.stick_y <= 1):
        parser.error("invalid output, duration, captures, port, or input")
    state_hash = hashlib.sha256(args.savestate.read_bytes()).hexdigest()
    if state_hash != FIRST_DOWN_SHA256:
        parser.error("expected the documented first-down preplay savestate")
    check_dol(args.dol)
    verified_iso(args.iso)
    version = subprocess.run([str(args.dolphin_binary), "--version"],
                             capture_output=True, text=True, timeout=10,
                             env={**os.environ, "QT_QPA_PLATFORM": "offscreen"})
    if version.returncode or "2603a" not in version.stdout + version.stderr:
        parser.error("the saved state requires Dolphin 2603a")
    runner._find_linux_dolphin = lambda: str(args.dolphin_binary)
    session_module._find_linux_dolphin = lambda: str(args.dolphin_binary)

    with args.output.open("x", encoding="utf-8") as output:
        def emit(**row: object) -> None:
            output.write(json.dumps(row, sort_keys=True) + "\n")
            output.flush()

        with tempfile.TemporaryDirectory(prefix="nflstreet-handle-calls-") as root:
            with session_module.DolphinSession.start(
                args.iso, savestate=args.savestate, pipe_input=True,
                gdb_port=args.gdb_port, video_backend="OGL", user_dir=Path(root),
            ) as session:
                if not session.wait_for_first_frame(90):
                    raise RuntimeError("no completed frame")
                time.sleep(2)
                gdb = session._gdb
                assert gdb is not None
                # Capture-only transport setting: ACK + next request should not
                # wait for Nagle's timer between every inspected instruction.
                assert gdb._sock is not None
                gdb._sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
                gdb.interrupt()
                gdb.read_pc()
                manager = session.read_u32(MANAGER_SLOT)
                if manager != 0x803B6F00:
                    raise RuntimeError("unexpected selection manager")
                roster_manager = session.read_u32(0x803A9A40 - 0x7C30)
                roster_base = session.read_u32(roster_manager)
                actors_per_team = int.from_bytes(session.read_bytes(roster_manager + 8, 2), "big")
                if roster_base != 0x803B7320 or actors_per_team != 7:
                    raise RuntimeError("unexpected first-down roster")

                def selection() -> dict[str, int]:
                    index = session.read_bytes(manager + 1, 1)[0]
                    record = manager + 8 + index * 0xF4
                    return {"index": index, "record": record,
                            "handle": session.read_u32(record + 0xB4)}

                def frame_count() -> int:
                    return len(list((Path(root) / "Dump/Frames").glob("*.png")))

                emit(event="ready", savestate_sha256=state_hash,
                     dolphin="2603a", dol_sha1=hashlib.sha1(args.dol.read_bytes()).hexdigest(),
                     selection=selection(), frames=frame_count(),
                     roster_base=roster_base, actors_per_team=actors_per_team,
                     roster_handles=[session.read_u32(roster_base + i * 0x14D4)
                                     for i in range(2 * actors_per_team)],
                     stick=[args.stick_x, args.stick_y])
                for pc in CALLS:
                    gdb.set_execution_breakpoint(pc)
                if args.trace_handoff:
                    gdb.set_execution_breakpoint(0x8007C7B8)
                if args.trace_event_build:
                    gdb.set_execution_breakpoint(0x8007C000)
                if args.trace_event_process:
                    gdb.set_execution_breakpoint(0x8007C180)
                if args.trace_event_flags:
                    for pc in (0x800157F0, 0x80015804):
                        gdb.set_execution_breakpoint(pc)
                if args.trace_event_gate:
                    gdb.set_execution_breakpoint(0x8007C758)
                if args.trace_stream_setup:
                    for pc in (0x8017724C, 0x80174200, 0x80173F8C):
                        gdb.set_execution_breakpoint(pc)
                if args.trace_event_stages:
                    gdb.set_execution_breakpoint(0x8007C2D8)
                snap_held = args.trace_contact_service
                if snap_held:
                    # Wall-clock release can happen while GDB has paused play.
                    session.send_input(f"SET MAIN {args.stick_x} {args.stick_y}")
                    session.send_input("PRESS A")
                gdb.continue_execution()
                if not snap_held:
                    session.play_sequence_async(InputSequence([
                        InputCommand(0, "PRESS A"), InputCommand(0.15, "RELEASE A"),
                        InputCommand(0.2, f"SET MAIN {args.stick_x} {args.stick_y}"),
                        InputCommand(args.seconds, "SET MAIN 0.5 0.5"),
                    ]))
                started = time.monotonic()
                captured = 0
                unrelated = 0
                pending = None
                building_actor = None
                processed_events = 0
                gate_actor = None
                gate_exit_pc = None
                gate_seen = set()
                timing_streams = {}
                timing_active = None
                timing_count = 0
                setup_active = None
                stage_active = None
                stage_count = 0
                stage_seen = set()
                contact_active = None
                contact_succeeded = False
                asset_captured = False
                transition_patched = False
                animation_calls = {}
                animation_returns = {}
                animation_entries = (0x80173C24,0x80173E78,0x80177ACC,0x80177EF4,
                                     0x80178780,0x8017869C,0x80179288,0x80068E8C,
                                     0x80068310,0x800618FC,0x80175F48,0x80174444)
                if args.trace_animation_details:
                    animation_entries += (0x80175C18,0x8017859C,0x80178D8C,0x801786D0,
                                          0x80175E00,0x80060764,0x8004F0D4,0x80103E64)
                if args.experiment_transition_clip:
                    animation_entries += (0x80175D18,0x8017749C,0x80179310,0x80178608,0x80174330)
                def source_table(ptr):
                    count=int.from_bytes(session.read_bytes(ptr,2),'big')
                    if count>64:raise RuntimeError('source table exceeds bound')
                    return session.read_bytes(ptr,8+count*8).hex()
                def animation_snapshot():
                    return dict(actor_hex=session.read_bytes(contact_active,0x14D4).hex(),
                                incoming_hex=session.read_bytes(contact_incoming,0x14D4).hex(),
                                motion_globals_hex=(session.read_bytes(0x803A9A40-0x5BB8,4)+
                                                    session.read_bytes(0x803A9A40-0x5BBC,4)).hex())
                while time.monotonic() - started < args.seconds:
                    try:
                        stop_reason = gdb.wait_for_stop(0.5)
                        pc = gdb.read_pc()
                    except GDBError:
                        continue
                    if args.trace_animation and contact_succeeded and pc in animation_entries:
                        regs=[gdb.read_register(n) for n in range(3,10)]
                        if args.trace_loaded_assets and not asset_captured:
                            count=session.read_u32(0x803A9A40-0x4F70)
                            if count>4096:raise RuntimeError('asset registry exceeds bound')
                            registry=session.read_bytes(session.read_u32(0x803A9A40-0x4F6C),count*28)
                            pointers=[int.from_bytes(registry[i*28+12:i*28+16],'big') for i in range(count)
                                      if registry[i*28+18]==1 and int.from_bytes(registry[i*28+20:i*28+22],'big')==1]
                            if not pointers:raise RuntimeError('missing context-one asset banks')
                            base=min(pointers);end=max(pointers)+0x100000
                            if base<0x80000000 or end>0x81800000:raise RuntimeError('asset arena outside MEM1')
                            emit(event='loaded_assets',base=base,arena_hex=session.read_bytes(base,end-base).hex(),registry_hex=registry.hex())
                            asset_captured=True
                        if args.experiment_transition_clip and pc==0x80178780 and not transition_patched:
                            table=session.read_bytes(0x80389E20,8)
                            if int.from_bytes(table[:2],'big'):
                                ptr=session.read_u32(0x80389E20+12)
                                if args.experiment_transition_row>=session.read_u32(ptr):raise RuntimeError('experiment row outside table')
                                row=ptr+4+16*args.experiment_transition_row;original=session.read_bytes(row,16)
                                target=int.from_bytes(original[12:16],'big')
                                if int.from_bytes(session.read_bytes(target+4,2),'big')!=3:raise RuntimeError('first transition row is not mode three')
                                channels=session.read_bytes(regs[2],1240)
                                def strongest(group):
                                    candidates=[channels[i*124:(i+1)*124] for i in range(10)
                                                if channels[i*124+1]==2 and int.from_bytes(channels[i*124+8:i*124+10],'big')==group]
                                    import struct
                                    return max(candidates,key=lambda c:struct.unpack('>f',c[48:52])[0])
                                old=strongest(regs[4]);new=strongest(regs[3])
                                changed=(int.from_bytes(new[4:6],'big')|0x8000).to_bytes(2,'big')+new[2:4]+old[2:4]+old[100:102]*2+original[10:]
                                if gdb._command(f'M{row:x},10:{changed.hex()}')!='OK':raise RuntimeError('transition experiment write failed')
                                if session.read_bytes(row,16)!=changed:raise RuntimeError('transition experiment readback failed')
                                emit(event='transition_experiment',row=row,before_hex=original.hex(),after_hex=changed.hex(),target=target,
                                     limitation='Controlled source-row mutation; not an observed natural transition')
                                transition_patched=True
                        ret=gdb.read_register(67)
                        call=len(animation_calls)
                        animation_calls[call]=dict(pc=pc,regs=regs)
                        animation_returns.setdefault(ret,[]).append(call)
                        gdb.set_execution_breakpoint(ret)
                        extra={}
                        if pc==0x80173C24:
                            count=int.from_bytes(session.read_bytes(regs[0]+4,2),'big')
                            if count>16:raise RuntimeError('animation channel count exceeds bound')
                            extra=dict(header_hex=session.read_bytes(regs[0],16).hex(),
                                       channels_hex=session.read_bytes(regs[1],count*0x7C).hex())
                        if pc==0x80174444:
                            extra=dict(program=regs[1],program_hex=session.read_bytes(regs[1],4096).hex())
                        if pc==0x8017749C:
                            extra=dict(program=regs[0],program_hex=session.read_bytes(regs[0],4096).hex())
                        if pc==0x80178D8C:
                            extra=dict(r10=gdb.read_register(10),stack_bank_hex=session.read_bytes(gdb.read_register(1)+10,2).hex(),
                                       transition_hex=source_table(regs[0]))
                        if pc==0x80060764:
                            extra=dict(row_hex=session.read_bytes(regs[0],20).hex(),
                                       partner_hex=session.read_bytes(regs[3],0x14D4).hex())
                        if pc==0x8004F0D4:
                            profile=session.read_u32(regs[0]+0x30C)
                            extra=dict(profile_hex=session.read_bytes(profile,16).hex() if profile else None,
                                       speed_table_hex=session.read_bytes(0x8026E028,64).hex())
                        if pc==0x80103E64:
                            ptr=session.read_u32(0x803A9A40-0x7078)
                            extra=dict(side_source_hex=session.read_bytes(ptr+0x39,1).hex() if ptr else None)
                        if pc==0x80175E00:
                            count=session.read_u32(0x803A9A40-0x4F70)
                            ptr=session.read_u32(0x803A9A40-0x4F6C)
                            if count>4096:raise RuntimeError(f'bank registry count {count} exceeds bound')
                            registry=session.read_bytes(ptr,count*28);banks=[]
                            for i in range(count):
                                entry=registry[i*28:i*28+28]
                                if entry[18]!=1 or int.from_bytes(entry[20:22],'big')!=regs[0]:
                                    banks.append(None);continue
                                bank_ptr=int.from_bytes(registry[i*28+12:i*28+16],'big')
                                b=session.read_bytes(bank_ptr,32);gp=int.from_bytes(b[8:12],'big')
                                gc=session.read_u32(gp)
                                if gc>512:raise RuntimeError('registry group table exceeds bound')
                                banks.append(dict(bank_hex=b.hex(),groups_hex=session.read_bytes(gp,4+gc*16).hex()))
                            extra=dict(registry_hex=registry.hex(),registry_banks=banks)
                        emit(event='animation_entry',call=call,pc=pc,return_pc=ret,regs=regs,
                             f1_hex=f'{gdb.read_register(33):016x}',f2_hex=f'{gdb.read_register(34):016x}',
                             **animation_snapshot(),**extra)
                        gdb.remove_execution_breakpoint(pc)
                        gdb.step_instruction()
                        gdb.set_execution_breakpoint(pc)
                    elif args.trace_animation and contact_succeeded and pc in animation_returns:
                        call=animation_returns[pc].pop()
                        source=animation_calls[call];extra={};result=gdb.read_register(3)
                        if source['pc']==0x80175F48 and result:
                            extra=dict(bank=result,bank_hex=session.read_bytes(result,32).hex())
                            groups=session.read_u32(result+8);count=session.read_u32(groups)
                            if count>512:raise RuntimeError('bank group table exceeds bound')
                            extra['groups_hex']=session.read_bytes(groups,4+count*16).hex()
                            if args.trace_animation_details:
                                programs=session.read_u32(result+12);pcnt=session.read_u32(programs)
                                if pcnt>4096:raise RuntimeError('bank program table exceeds bound')
                                extra['program_table_hex']=session.read_bytes(programs,4+pcnt*4).hex()
                        if source['pc']==0x8017869C:
                            count=int.from_bytes(session.read_bytes(0x80389E20,2),'big')
                            if count>64:raise RuntimeError('transition table exceeds bound')
                            extra['transition_hex']=session.read_bytes(0x80389E20,8+count*8).hex()
                            if args.trace_animation_details:
                                data=bytes.fromhex(extra['transition_hex']);assets=[]
                                for i in range(count):
                                    ptr=int.from_bytes(data[12+8*i:16+8*i],'big');rows=session.read_u32(ptr)
                                    if rows>256:raise RuntimeError('transition rows exceed bound')
                                    blob=session.read_bytes(ptr,4+16*rows);targets=[]
                                    for j in range(rows):
                                        target=int.from_bytes(blob[16+16*j:20+16*j],'big')
                                        if target:targets.append(dict(identity=target,data_hex=session.read_bytes(target,16).hex()))
                                    assets.append(dict(identity=ptr,data_hex=blob.hex(),targets=targets))
                                extra['transition_assets']=assets
                        if source['pc']==0x80175C18 and result:
                            count=int.from_bytes(session.read_bytes(result+4,2),'big')
                            if count>127:raise RuntimeError('callback asset rows exceed bound')
                            extra=dict(asset=result,asset_hex=session.read_bytes(result,8+20*count).hex())
                        if source['pc']==0x8017859C:
                            extra=dict(table_hex=source_table(result))
                        emit(event='animation_return',call=call,pc=source['pc'],result=result,
                             f1_hex=f'{gdb.read_register(33):016x}',
                             **animation_snapshot(),**extra)
                        if not animation_returns[pc]:
                            del animation_returns[pc]
                            gdb.remove_execution_breakpoint(pc)
                        else:
                            gdb.remove_execution_breakpoint(pc)
                            gdb.step_instruction()
                            gdb.set_execution_breakpoint(pc)
                    elif args.trace_stream_setup and pc in (0x80174200,0x80173F8C):
                        stream = gdb.read_register(31) + 0x4C
                        actor = stream - 0x5AC
                        if actor - roster_base in (7 * 0x14D4, 12 * 0x14D4):
                            bank, index = gdb.read_register(3), gdb.read_register(4)
                            if not 0x80000000 <= bank <= 0x817FFFF0 or index > 4095:
                                raise RuntimeError("program lookup outside bounded bank")
                            table = session.read_u32(bank + 0xC)
                            size = 4 + 4 * (index + 1)
                            if not 0x80000000 <= table <= 0x81800000 - size:
                                raise RuntimeError("program pointer table outside MEM1")
                            emit(event="stream_lookup", actor=actor, stream=stream,
                                 bank=bank, index=index, table=table,
                                 table_hex=session.read_bytes(table, size).hex(),
                                 frames=frame_count())
                        gdb.remove_execution_breakpoint(pc)
                        gdb.step_instruction()
                        gdb.set_execution_breakpoint(pc)
                    elif args.trace_stream_setup and pc == 0x8017724C:
                        stream, program = gdb.read_register(3), gdb.read_register(4)
                        actor = stream - 0x5AC
                        if actor - roster_base in (7 * 0x14D4, 12 * 0x14D4):
                            if setup_active is not None or not 0x80000000 <= program <= 0x817FFF00:
                                raise RuntimeError("invalid or nested stream setup")
                            setup_active = stream
                            setup_program = program
                            emit(event="stream_setup_entry", stream=stream, actor=actor,
                                 program=program, alternate=gdb.read_register(5),
                                 state_hex=session.read_bytes(stream,64).hex(),
                                 data_hex=session.read_bytes(program,256).hex(),
                                 flags=session.read_u32(actor+0xC), frames=frame_count())
                            gdb.set_execution_breakpoint(0x80177338)
                        gdb.remove_execution_breakpoint(pc)
                        gdb.step_instruction()
                        gdb.set_execution_breakpoint(pc)
                    elif args.trace_stream_setup and setup_active is not None and pc == 0x80177338:
                        if gdb.read_register(31) != setup_active:
                            raise RuntimeError("stream initializer changed its object")
                        if session.read_u32(setup_active) != setup_program:
                            raise RuntimeError("initializer memory does not contain its source program")
                        emit(event="stream_setup_return", stream=setup_active,
                             state_hex=session.read_bytes(setup_active,64).hex(),
                             result=gdb.read_register(3),
                             flags=session.read_u32(setup_active-0x5AC+0xC), frames=frame_count())
                        gdb.remove_execution_breakpoint(pc)
                        setup_active = None
                    elif args.trace_event_stages and pc == 0x8007C2D8:
                        actor, incoming = gdb.read_register(31), gdb.read_register(28)
                        if actor - roster_base in (7*0x14D4,12*0x14D4):
                            if session.read_u32(actor) == 0:
                                raise RuntimeError("watched roster actor header unexpectedly zero")
                            event = session.read_bytes(actor+0x150,12)
                            flags = session.read_u32(actor+0xC)
                            stage = int.from_bytes(event[4:6],'big')
                            code = int.from_bytes(event[8:12],'big')
                            key = (actor,code,stage)
                            if stage < 3 and code in (4,14) and (key not in stage_seen or flags&4 or stage==2):
                                if stage_active is not None or not 0x80000000 <= incoming <= 0x817FEB28:
                                    raise RuntimeError("invalid or nested event-stage call")
                                stage_seen.add(key)
                                stage_active = actor
                                count = session.read_bytes(manager,1)[0]
                                descriptor = session.read_u32(actor+0x310)
                                incoming_descriptor = session.read_u32(incoming+0x310)
                                emit(event="early_stage_entry", actor=actor,incoming=incoming,
                                     flags=flags,event_hex=event.hex(),
                                     descriptor_hex=session.read_bytes(descriptor,6).hex(),
                                     incoming_descriptor_hex=session.read_bytes(incoming_descriptor,6).hex(),
                                     incoming_header_hex=session.read_bytes(incoming,16).hex(),
                                     incoming_408=session.read_u32(incoming+0x408),
                                     contact_eligibility_callback=session.read_u32(0x803A9A40-0x4E18),
                                     geometry_hex=session.read_bytes(actor+0x1A8,48).hex(),
                                     incoming_geometry_hex=session.read_bytes(incoming+0x1A8,48).hex(),
                                     actor_mode_hex=session.read_bytes(actor+0xB5D,2).hex(),
                                     record_handles=[session.read_u32(manager+8+i*0xF4+0xB4) for i in range(count)],
                                     record_target_hex=session.read_bytes(manager+8+0xC,12).hex(),
                                     request_hex=session.read_bytes(actor+0x3F0,3).hex(),
                                     control_hex=session.read_bytes(actor+0x200,16).hex(),frames=frame_count())
                                for stop in (0x8007C930,0x8007B4F0,0x8007B720,0x8007BEC0,0x8007B910,0x8007B9C0,0x8007BA20):
                                    gdb.set_execution_breakpoint(stop)
                                if args.trace_contact_service and stage==2 and session.read_bytes(incoming_descriptor,1)==b'\x1b':
                                    contact_active=actor
                                    contact_incoming=incoming
                                    for stop in (0x8005FE40,0x8005FE5C,0x8005EEAC,0x8005EF28,0x8005EFA8,0x8005EDC0,0x8005EE44,0x8005F230,0x8005F234,0x8007B8B0,0x8004E3C8,0x80068F58):
                                        gdb.set_execution_breakpoint(stop)
                        gdb.remove_execution_breakpoint(pc)
                        gdb.step_instruction()
                        gdb.set_execution_breakpoint(pc)
                    elif args.trace_contact_service and contact_active is not None and pc in (0x8005EEAC,0x8005EF28,0x8005EFA8,0x8005EDC0,0x8005EE44,0x8005F230,0x8005F234,0x8007B8B0,0x8004E3C8,0x80068F58):
                        if pc in (0x8004E3C8,0x80068F58):
                            actor=gdb.read_register(29 if pc==0x8004E3C8 else 31)
                            emit(event="contact_animation_flag",actor=actor,pc=pc,
                                 before=session.read_u32(actor+0xC),written=gdb.read_register(0),frames=frame_count())
                        elif pc==0x8005EEAC:
                            params=gdb.read_register(3)
                            motion=session.read_u32(session.read_u32(contact_incoming+0xC10)+0x20)
                            emit(event="contact_select_entry",manager_hex=session.read_bytes(manager,256).hex(),motion_hex=session.read_bytes(motion,44).hex(),params_hex=session.read_bytes(params,60).hex(),
                                 actor_hex=session.read_bytes(contact_active,0x14D4).hex(),
                                 incoming_hex=session.read_bytes(contact_incoming,0x14D4).hex(),
                                 angle_tolerance_hex=session.read_bytes(0x803A9A40-0x79E4,4).hex(),
                                 frames=frame_count())
                        elif pc==0x8005EF28:
                            table=gdb.read_register(14)
                            count=int.from_bytes(session.read_bytes(table,2),'big')
                            if count>64: raise RuntimeError("contact candidate count exceeds capture bound")
                            data=session.read_bytes(table,4+count*8)
                            assets=[]
                            for i in range(count):
                                ptr=int.from_bytes(data[8+i*8:12+i*8],'big')
                                row_count=int.from_bytes(session.read_bytes(ptr+4,2),'big')
                                if row_count>256 or not 0x80000000<=ptr<=0x81800000-8-row_count*20:
                                    raise RuntimeError("candidate asset outside MEM1")
                                assets.append(dict(identity=ptr,data_hex=session.read_bytes(ptr,8+row_count*20).hex()))
                            emit(event="contact_candidates",table_hex=data.hex(),assets=assets,frames=frame_count())
                        elif pc==0x8005EFA8:
                            ptr=gdb.read_register(3)
                            row_count=int.from_bytes(session.read_bytes(ptr+4,2),'big')
                            if row_count>256: raise RuntimeError("resolved asset exceeds row bound")
                            emit(event="contact_asset_lookup",identity=ptr,key=gdb.read_register(20),
                                 data_hex=session.read_bytes(ptr,8+20*row_count).hex(),frames=frame_count())
                        elif pc==0x8005EDC0:
                            params=gdb.read_register(3); geom=gdb.read_register(4)
                            emit(event="contact_predicate_entry",params_hex=session.read_bytes(params,60).hex(),
                                 geometry_hex=session.read_bytes(geom,44).hex(),
                                 current_row_hex=session.read_bytes(session.read_u32(geom+0x10),20).hex(),
                                 incoming_row_hex=session.read_bytes(session.read_u32(geom+0x14),20).hex(),frames=frame_count())
                        elif pc==0x8005EE44:
                            emit(event="contact_predicate_return",result=gdb.read_register(29),frames=frame_count())
                        elif pc==0x8005F230:
                            rng=session.read_u32(0x803A9A40-0x61F8)
                            generator=session.read_u32(session.read_u32(rng+0x14))
                            emit(event="contact_rng_entry",count=gdb.read_register(4),rng=rng,
                                 state_hex=session.read_bytes(rng,24).hex(),generator=generator,
                                 generator_hex=session.read_bytes(generator,256).hex(),frames=frame_count())
                        elif pc==0x8005F234:
                            rng=session.read_u32(0x803A9A40-0x61F8)
                            generator=session.read_u32(session.read_u32(rng+0x14))
                            emit(event="contact_rng_return",index=gdb.read_register(3),
                                 state_hex=session.read_bytes(rng,24).hex(),
                                 generator_hex=session.read_bytes(generator,256).hex(),frames=frame_count())
                        else:
                            contact_succeeded=bool(gdb.read_register(3))
                            if args.trace_animation and contact_succeeded:
                                for stop in animation_entries:gdb.set_execution_breakpoint(stop)
                            emit(event="contact_before_apply",matched=gdb.read_register(3),
                                 actor_hex=session.read_bytes(contact_active,0x14D4).hex(),
                                 incoming_hex=session.read_bytes(contact_incoming,0x14D4).hex(),frames=frame_count())
                        gdb.remove_execution_breakpoint(pc)
                        gdb.step_instruction()
                        gdb.set_execution_breakpoint(pc)
                    elif args.trace_contact_service and stage_active is not None and pc in (0x8005FE40,0x8005FE5C):
                        if pc==0x8005FE40:
                            asset=gdb.read_register(3)
                            params=gdb.read_register(31)
                            data=None
                            if asset:
                                count=int.from_bytes(session.read_bytes(asset+4,2),'big')
                                size=8+20*count
                                if count>256 or not 0x80000000 <= asset <= 0x81800000-size:
                                    raise RuntimeError("contact asset outside bounded MEM1")
                                data=session.read_bytes(asset,size).hex()
                            emit(event="contact_asset",actor=stage_active,asset=asset,data_hex=data,
                                 params_hex=session.read_bytes(params,60).hex(),
                                 choice=int.from_bytes(session.read_bytes(gdb.read_register(1)+0x38,2),'big'),frames=frame_count())
                        else:
                            emit(event="contact_pair_outputs",actor=stage_active,
                                 current_hex=session.read_bytes(gdb.read_register(29),16).hex(),
                                 incoming_hex=session.read_bytes(gdb.read_register(28),16).hex(),
                                 matched=gdb.read_register(30),frames=frame_count())
                        gdb.remove_execution_breakpoint(pc)
                        gdb.step_instruction()
                        gdb.set_execution_breakpoint(pc)
                    elif args.trace_event_stages and stage_active is not None and pc in (0x8007B9C0,0x8007BA20):
                        stack=gdb.read_register(1)
                        emit(event="stage_tracking_inputs",pc=pc,actor=stage_active,
                             service_value=gdb.read_register(3),
                             field_hex=session.read_bytes(stack+0x20,12).hex() if pc==0x8007BA20 else None,
                             frames=frame_count())
                        gdb.remove_execution_breakpoint(pc)
                        gdb.step_instruction()
                        gdb.set_execution_breakpoint(pc)
                    elif args.trace_event_stages and stage_active is not None and pc in (0x8007B4F0,0x8007B720,0x8007BEC0,0x8007B910):
                        emit(event="early_stage_service", actor=stage_active,pc=pc,
                             r3=gdb.read_register(3),r5=gdb.read_register(5),
                             flags=session.read_u32(stage_active+0xC),
                             request_hex=session.read_bytes(stage_active+0x3F0,3).hex(),
                             control_hex=session.read_bytes(stage_active+0x200,16).hex(),frames=frame_count())
                        gdb.remove_execution_breakpoint(pc)
                        gdb.step_instruction()
                        gdb.set_execution_breakpoint(pc)
                    elif args.trace_event_stages and stage_active is not None and pc == 0x8007C930:
                        if contact_active is not None:
                            emit(event="contact_after_apply",actor_hex=session.read_bytes(contact_active,0x14D4).hex(),
                                 incoming_hex=session.read_bytes(contact_incoming,0x14D4).hex(),
                                 manager_hex=session.read_bytes(manager,0x100).hex(),frames=frame_count())
                        emit(event="early_stage_return", actor=stage_active,result=gdb.read_register(3),
                             flags=session.read_u32(stage_active+0xC),
                             event_hex=session.read_bytes(stage_active+0x150,12).hex(),
                             request_hex=session.read_bytes(stage_active+0x3F0,3).hex(),
                             control_hex=session.read_bytes(stage_active+0x200,16).hex(),frames=frame_count())
                        for stop in (0x8007C930,0x8007B4F0,0x8007B720,0x8007BEC0,0x8007B910,0x8007B9C0,0x8007BA20):
                            gdb.remove_execution_breakpoint(stop)
                        if contact_active is not None:
                            for stop in (0x8005FE40,0x8005FE5C,0x8005EEAC,0x8005EF28,0x8005EFA8,0x8005EDC0,0x8005EE44,0x8005F230,0x8005F234,0x8007B8B0,0x8004E3C8,0x80068F58):
                                gdb.remove_execution_breakpoint(stop)
                            contact_active=None
                        stage_active = None
                        stage_count += 1
                        if contact_succeeded:
                            break
                    elif args.trace_command_timing and pc in (0x80173E34, 0x8017425C):
                        stream = gdb.read_register(3) + 0x4C
                        relative = stream - 0x5AC - roster_base
                        if relative in (7 * 0x14D4, 12 * 0x14D4):
                            if timing_active is not None:
                                raise RuntimeError("nested command timing update")
                            timing_active = stream
                            data = session.read_u32(stream)
                            if not 0x80000000 <= data <= 0x817FFF00:
                                raise RuntimeError("command data outside MEM1")
                            emit(event="timing_entry", stream=stream,
                                 actor=stream - 0x5AC, call_pc=pc,
                                 state_hex=session.read_bytes(stream, 64).hex(),
                                 data_hex=session.read_bytes(data, 256).hex(),
                                 delta_fpr_hex=f"{gdb.read_register(33):016x}",
                                 constants_hex=session.read_bytes(0x802610F0, 52).hex(),
                                 frames=frame_count())
                            for stop in (0x80177484, 0x80177078, 0x80177090):
                                gdb.set_execution_breakpoint(stop)
                        gdb.remove_execution_breakpoint(pc)
                        gdb.step_instruction()
                        gdb.set_execution_breakpoint(pc)
                    elif args.trace_command_timing and timing_active is not None and pc in (0x80177078, 0x80177090):
                        emit(event="timing_command" if pc == 0x80177078 else "timing_callback_return",
                             stream=timing_active, command=gdb.read_register(30),
                             result=gdb.read_register(3),
                             state_hex=session.read_bytes(timing_active, 64).hex(),
                             frames=frame_count())
                        gdb.remove_execution_breakpoint(pc)
                        gdb.step_instruction()
                        gdb.set_execution_breakpoint(pc)
                    elif args.trace_command_timing and timing_active is not None and pc == 0x80177484:
                        emit(event="timing_return", stream=timing_active,
                             result=gdb.read_register(3),
                             state_hex=session.read_bytes(timing_active, 64).hex(),
                             frames=frame_count())
                        for stop in (0x80177484, 0x80177078, 0x80177090):
                            gdb.remove_execution_breakpoint(stop)
                        timing_active = None
                        timing_count += 1
                    elif args.trace_event_gate and pc == 0x8007C758:
                        actor = gdb.read_register(31)
                        if actor - roster_base in (7 * 0x14D4, 12 * 0x14D4):
                            flags = session.read_u32(actor + 0xC)
                            code = session.read_u32(actor + 0x158)
                            if (actor, code) not in gate_seen or flags & 1:
                                if gate_actor is not None:
                                    raise RuntimeError("nested event gate")
                                gate_seen.add((actor, code))
                                gate_actor = actor
                                count = session.read_bytes(manager, 1)[0]
                                incoming = gdb.read_register(28)
                                dispatch_pointer = session.read_u32(0x803A9A40 - 0x7CF8)
                                if (not 0x80000000 <= incoming <= 0x817FFFF0 or
                                        not 0x80000000 <= dispatch_pointer <= 0x817FFFFC):
                                    raise RuntimeError("gate service input outside MEM1")
                                emit(event="event_gate_entry", pc=pc, actor=actor,
                                     current_handle=session.read_u32(actor), flags=flags,
                                     incoming_actor=incoming,
                                     incoming_flags=session.read_u32(incoming + 0xC),
                                     dispatch_pointer=dispatch_pointer,
                                     dispatch_value=session.read_u32(dispatch_pointer),
                                     event_hex=session.read_bytes(actor + 0x150, 12).hex(),
                                     record_handles=[session.read_u32(manager + 8 + i * 0xF4 + 0xB4)
                                                     for i in range(count)], frames=frame_count())
                                gate_exit_pc = 0x8007C7BC if flags & 1 else 0x8007C808
                                gdb.set_execution_breakpoint(gate_exit_pc)
                                if flags & 1:
                                    gdb.set_execution_breakpoint(0x8003D81C)
                                    gdb.set_execution_breakpoint(0x8007BFEC)
                        gdb.remove_execution_breakpoint(pc)
                        gdb.step_instruction()
                        gdb.set_execution_breakpoint(pc)
                    elif args.trace_event_gate and gate_actor is not None and pc == gate_exit_pc:
                        gdb.remove_execution_breakpoint(pc)
                        if gdb.read_register(31) != gate_actor:
                            raise RuntimeError("event gate changed current actor")
                        count = session.read_bytes(manager, 1)[0]
                        emit(event="event_gate_return", pc=pc, actor=gate_actor,
                             flags=session.read_u32(gate_actor + 0xC),
                             record_handles=[session.read_u32(manager + 8 + i * 0xF4 + 0xB4)
                                             for i in range(count)], frames=frame_count())
                        gate_actor = None
                        gate_exit_pc = None
                        if captured >= args.captures:
                            break
                    elif args.trace_event_gate and gate_actor is not None and pc == 0x8003D81C:
                        gdb.remove_execution_breakpoint(pc)
                        old_actor, new_actor = gdb.read_register(3), gdb.read_register(4)
                        if old_actor != gate_actor or not 0x80000000 <= new_actor <= 0x817FFFF0:
                            raise RuntimeError("unexpected actor-transfer arguments")
                        emit(event="actor_transfer_entry", pc=pc, actor=old_actor,
                             incoming_actor=new_actor,
                             old_header_hex=session.read_bytes(old_actor, 16).hex(),
                             new_header_hex=session.read_bytes(new_actor, 16).hex(),
                             frames=frame_count())
                    elif args.trace_event_gate and gate_actor is not None and pc == 0x8007BFEC:
                        gdb.remove_execution_breakpoint(pc)
                        new_actor = gdb.read_register(31)
                        emit(event="actor_transfer_return", pc=pc, actor=gate_actor,
                             incoming_actor=new_actor,
                             old_header_hex=session.read_bytes(gate_actor, 16).hex(),
                             new_header_hex=session.read_bytes(new_actor, 16).hex(),
                             frames=frame_count())
                    elif args.trace_event_flags and pc in (0x800157F0, 0x80015804):
                        actor = gdb.read_register(31)
                        watched = actor - roster_base in (7 * 0x14D4, 12 * 0x14D4)
                        if watched:
                            stack = gdb.read_register(1)
                            if not 0x80000000 <= stack <= 0x817FFFA8:
                                raise RuntimeError("command-parser stack outside MEM1")
                            if args.trace_command_timing:
                                if stack > 0x817FFF58:
                                    raise RuntimeError("timing caller stack outside MEM1")
                                stream = session.read_u32(0x803A9A40 - 0x4F58)
                                if not 0x80000000 <= stream <= 0x817FFFC0:
                                    raise RuntimeError("command stream outside MEM1")
                                timing_streams[stream] = actor
                                if stream != actor + 0x5AC:
                                    raise RuntimeError("readiness command uses an unexpected actor stream")
                                emit(event="timing_stream_identified", stream=stream, actor=actor,
                                     driver_return=session.read_u32(stack + 0xA4),
                                     state_hex=session.read_bytes(stream, 64).hex())
                                # Arm after the first readiness callback, allowing
                                # the wall-clock input sequence to finish its snap.
                                for stop in (0x80173E34, 0x8017425C):
                                    gdb.set_execution_breakpoint(stop)
                            emit(event="event_flag_command", pc=pc, actor=actor,
                                 command=gdb.read_register(30), context=gdb.read_register(29),
                                 before=session.read_u32(actor + 0xC),
                                 store_value=gdb.read_register(0),
                                 parser_return=session.read_u32(stack + 0x54),
                                 relay_return=session.read_u32(stack + 0x5C),
                                 callback_pointer=session.read_u32(0x803A9A40 - 0x6734),
                                 callback_selector=session.read_bytes(0x803A9A40 - 0x579C, 1)[0],
                                 event_hex=session.read_bytes(actor + 0x150, 12).hex(),
                                 frames=frame_count())
                        gdb.remove_execution_breakpoint(pc)
                        gdb.step_instruction()
                        if watched:
                            emit(event="event_flag_written", pc=gdb.read_pc(), actor=actor,
                                 after=session.read_u32(actor + 0xC), frames=frame_count())
                        gdb.set_execution_breakpoint(pc)
                    elif args.trace_event_build and pc in (0x8007C000, 0x8007C180):
                        if snap_held and pc==0x8007C000:
                            session.send_input("RELEASE A")
                            emit(event="snap_release",pc=pc,frames=frame_count())
                            snap_held=False
                        actor = gdb.read_register(3)
                        relative = actor - roster_base
                        watched = relative in (7 * 0x14D4, 12 * 0x14D4)
                        if watched and pc == 0x8007C000:
                            if building_actor is not None:
                                raise RuntimeError("nested event construction")
                            descriptor = session.read_u32(actor + 0x310)
                            if not 0x80000000 <= descriptor <= 0x817FFFFA:
                                raise RuntimeError("event descriptor outside MEM1")
                            building_actor = actor
                            emit(event="event_build_entry", pc=pc, actor=actor,
                                 descriptor_pointer=descriptor,
                                 descriptor_hex=session.read_bytes(descriptor, 6).hex(),
                                 header_hex=session.read_bytes(actor, 16).hex(),
                                 event_hex=session.read_bytes(actor + 0x150, 12).hex(),
                                 input_mode=session.read_bytes(actor + 0x20E, 1)[0],
                                 frames=frame_count())
                            gdb.set_execution_breakpoint(0x8007C054)
                            gdb.set_execution_breakpoint(0x8007C16C)
                        elif watched and pc == 0x8007C180 and processed_events < 80:
                            emit(event="event_process_entry", pc=pc, actor=actor,
                                 event_pointer=actor + 0x150,
                                 event_hex=session.read_bytes(actor + 0x150, 12).hex(),
                                 flags=session.read_u32(actor + 0xC), frames=frame_count())
                            processed_events += 1
                        gdb.remove_execution_breakpoint(pc)
                        gdb.step_instruction()
                        gdb.set_execution_breakpoint(pc)
                    elif args.trace_event_build and building_actor is not None and pc == 0x8007C054:
                        gdb.remove_execution_breakpoint(pc)
                        source_actor = gdb.read_register(30)
                        emit(event="event_build_prefix", pc=pc, actor=building_actor,
                             source_actor=source_actor,
                             source_handle=session.read_u32(source_actor) if source_actor else 0,
                             event_hex=session.read_bytes(building_actor + 0x150, 12).hex(),
                             frames=frame_count())
                    elif args.trace_event_build and building_actor is not None and pc == 0x8007C16C:
                        gdb.remove_execution_breakpoint(pc)
                        emit(event="event_build_return", pc=pc, actor=building_actor,
                             event_hex=session.read_bytes(building_actor + 0x150, 12).hex(),
                             flags=session.read_u32(building_actor + 0xC),
                             input_mode=session.read_bytes(building_actor + 0x20E, 1)[0],
                             frames=frame_count())
                        building_actor = None
                    elif pending is None and args.trace_handoff and pc == 0x8007C7B8:
                        current_actor = gdb.read_register(3)
                        new_actor = gdb.read_register(4)
                        record = gdb.read_register(5)
                        event = gdb.read_register(6)
                        for address in (current_actor, new_actor, event):
                            if not 0x80000000 <= address <= 0x817FFFEC:
                                raise RuntimeError("handoff argument outside MEM1")
                        count = session.read_bytes(manager, 1)[0]
                        emit(event="actor_handoff", pc=pc,
                             current_actor=current_actor, new_actor=new_actor,
                             record=record, current_handle=session.read_u32(current_actor),
                             new_handle=session.read_u32(new_actor),
                             event_pointer=event,
                             event_type=int.from_bytes(session.read_bytes(event + 4, 2), "big"),
                             event_code=session.read_u32(event + 8),
                             record_handles=[session.read_u32(manager + 8 + i * 0xF4 + 0xB4)
                                             for i in range(count)],
                             selection=selection(), frames=frame_count())
                        gdb.remove_execution_breakpoint(pc)
                        gdb.step_instruction()
                        gdb.set_execution_breakpoint(pc)
                    elif pending is None and pc in CALLS:
                        for entry in CALLS:
                            gdb.remove_execution_breakpoint(entry)
                        source = gdb.read_register(3)
                        destination = gdb.read_register(4)
                        if not 0x80000000 <= destination <= 0x817FFFFC:
                            raise RuntimeError("handle destination outside MEM1")
                        if source and not 0x80000000 <= source <= 0x817FFFFC:
                            raise RuntimeError("handle source outside MEM1")
                        before = session.read_u32(destination)
                        source_value = session.read_u32(source) if source else 0
                        pending = {"pc": pc, "source": source,
                                   "destination": destination, "before": before,
                                   "source_value": source_value}
                        if pc == 0x800BE2F8:
                            stack = gdb.read_register(1)
                            # Original wrapper saves incoming LR at stack +0x14.
                            if not 0x80000000 <= stack <= 0x817FFFE8:
                                raise RuntimeError("wrapper stack outside MEM1")
                            pending["wrapper_return"] = session.read_u32(stack + 0x14)
                        relative = source - roster_base
                        if source and relative >= 0 and relative % 0x14D4 == 0 and relative // 0x14D4 < 2 * actors_per_team:
                            pending["source_actor_slot"] = relative // 0x14D4
                            pending["source_actor_name_hex"] = session.read_bytes(source + 0xB80, 16).split(b"\0", 1)[0].hex()
                        emit(event="handle_call", **pending, selection=selection(),
                             frames=frame_count())
                        gdb.set_execution_breakpoint(pc + 4)
                    elif pending is not None and pc == pending["pc"] + 4:
                        gdb.remove_execution_breakpoint(pc)
                        emit(event="handle_return", pc=pc,
                             destination=pending["destination"],
                             after=session.read_u32(pending["destination"]),
                             selection=selection(), frames=frame_count())
                        captured += 1
                        pending = None
                        if captured >= args.captures and gate_actor is None:
                            break
                        for entry in CALLS:
                            gdb.set_execution_breakpoint(entry)
                    else:
                        unrelated += 1
                        if unrelated > 12:
                            raise RuntimeError("too many unrelated debugger stops")
                    gdb.continue_execution()
                if pending is None and captured < args.captures:
                    gdb.interrupt()
                    gdb.read_pc()
                emit(event="done", captured=captured, incomplete_call=pending is not None,
                     selection=selection(), frames=frame_count(),
                     unrelated_stops=unrelated,
                     timing_updates=timing_count, incomplete_timing=timing_active is not None,
                     stage_updates=stage_count,incomplete_stage=stage_active is not None,
                     incomplete_setup=setup_active is not None,
                     animation_calls=len(animation_calls),
                     incomplete_animation=bool(animation_returns),
                     elapsed_seconds=round(time.monotonic() - started, 3))


if __name__ == "__main__":
    main()
