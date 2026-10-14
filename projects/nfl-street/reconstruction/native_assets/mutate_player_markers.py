#!/usr/bin/env python3
"""Mutation test for the player-marker checks: plant one deliberate bug at a time in a scratch copy of the
native sources, rebuild the original-module check and require that it fails. Private scratch stays on W:."""

import shutil
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
PRIVATE = Path("/mnt/w/Daywater/research-temp")
SCRATCH = PRIVATE / "nflstreet-player-markers-20261014" / "mutants"
DLL = "W:\\Daywater\\research-temp\\nflstreet-native-windows-local\\gGNNE69_recomp.dll"
FILES = (
    "select_rings.c",
    "select_rings.h",
    "camera_slots.c",
    "camera_slots.h",
    "name_tag_project.c",
    "name_tag_project.h",
    "marker_ram.h",
    "player_markers_original_check.c",
)
# (name, file, old, new)
MUTANTS = (
    ("ring turn step", "select_rings.c", "(angle+0x16c16u)&0xffffffu)&&\n  mk_put(&r->d,ring+8", None),
    (
        "mirror ignored",
        "select_rings.c",
        "const uint32_t sign=negate?0x80000000u:0;",
        "const uint32_t sign=(negate&0)?0x80000000u:0;",
    ),
    ("flag 0x4000000 kept", "select_rings.c", "mk_mask(&r.d,actor+0xc,0x4000000u,0)", "mk_mask(&r.d,actor+0xc,0,0)"),
    (
        "pool scan short",
        "select_rings.c",
        "for(unsigned i=0;i<5;++i){uint32_t o,target;",
        "for(unsigned i=0;i<4;++i){uint32_t o,target;",
    ),
    ("slot 8 pad test", "select_rings.c", "if(team==0xff)a=controller;", "if(team!=0xff)a=controller;"),
    ("fade constant address", "select_rings.c", "#define K_FADE 0x80253710u", "#define K_FADE 0x80253714u"),
    (
        "size boundary 1.0",
        "select_rings.c",
        "if(v<=one){if(!mk_putf(&r->d,o+0x18,one)||!mk_put(&r->d,o+0xfc,0))return 0;}",
        "if(v<one){if(!mk_putf(&r->d,o+0x18,one)||!mk_put(&r->d,o+0xfc,0))return 0;}",
    ),
    ("grow keeps ring shown", "select_rings.c", "mk_put(&r->d,o+0x20,flags&~2u)", "mk_put(&r->d,o+0x20,flags)"),
    ("follow to wrong form", "select_rings.c", "(flags&~0x20u)|0x40u", "(flags&~0x20u)|0x20u"),
    ("cap boundary", "select_rings.c", "if(!(v<=cap)){", "if(!(v<cap)){"),
    ("ring z constant", "select_rings.c", "!mk_copy(&r->d,ring+0xc,K_ZERO)", None),
    ("pad beyond two", "select_rings.c", "if(n>1)return 1;", "if(n>2)return 1;"),
    ("controller entry stride", "select_rings.c", "base+n*0xf4u+8u", "base+n*0xf0u+8u"),
    ("controlled ring compare", "select_rings.c", "if(actor==controller){", "if(actor!=controller){"),
    ("first ring flag clear", "select_rings.c", "mk_mask(&r.d,ring2+0x20,2,0)", "mk_mask(&r.d,ring2+0x20,0,0)"),
    ("slot 4 store target", "camera_slots.c", "nflstreet_matrix_store(d,k,s4)", "nflstreet_matrix_store(d,k,s4+0x10)"),
    ("slot concat unfused", "camera_slots.c", "acc=nflstreet_fmadds(b[12+i],q[3],acc);", "acc=acc+b[12+i]*q[3];"),
    ("slot 3 on a copy", "camera_slots.c", "nflstreet_matrix_push_identity(d,k)&&", "nflstreet_matrix_push(d,k)&&"),
    ("unit lane", "camera_slots.c", "mk_float(d,UNIT+4,&unit[1])", "mk_float(d,UNIT,&unit[1])"),
    (
        "slot 3 reload skipped",
        "camera_slots.c",
        "nflstreet_matrix_load(d,k,s3)&&nflstreet_matrix_push(d,k)&&",
        "nflstreet_matrix_push(d,k)&&",
    ),
    ("depth scale", "name_tag_project.c", "mk_putf(d,tag+0x20,z*depth_scale)", "mk_putf(d,tag+0x20,z)"),
    ("lift address", "name_tag_project.c", "#define K_LIFT 0x80252b7cu", "#define K_LIFT 0x80252b78u"),
    (
        "hidden tag stays shown",
        "name_tag_project.c",
        "return mk_putb(d,tag+0x1c,0)&&nflstreet_matrix_pop(d,k);",
        "return nflstreet_matrix_pop(d,k);",
    ),
    ("clip boundary", "name_tag_project.c", "o[1]<=nw||o[1]>=w", "o[1]<nw||o[1]>=w"),
    ("clip boundary w", "name_tag_project.c", "o[2]>=w", "o[2]>w"),
    ("node aspect offset", "name_tag_project.c", "node+0xd0", "node+0xd4"),
    (
        "unshown tags projected",
        "name_tag_project.c",
        "return shown?nflstreet_name_tag_project(c,tag,actor+4):1;",
        "return nflstreet_name_tag_project(c,tag,actor+4);",
    ),
    ("unit constant", "name_tag_project.c", "const float inv=one/w;", "const float inv=1.0f/w;"),
    (
        "stack not selected",
        "name_tag_project.c",
        "if(!nflstreet_matrix_select(d,k)||!nflstreet_matrix_push(d,k)||",
        "if(!nflstreet_matrix_push(d,k)||",
    ),
)
# Mutants written as (None) replacements need a computed edit.
COMPUTED = {
    "ring turn step": lambda s: s.replace("(angle+0x16c16u)&0xffffffu", "(angle+0x16c17u)&0xffffffu", 1),
    "ring z constant": lambda s: s.replace("!mk_put(&r.d,ring+0xc,z)", "!mk_put(&r.d,ring+0xc,0)", 1),
}


def main() -> int:
    ctx = sys.argv[1] if len(sys.argv) > 1 else "14"
    SCRATCH.mkdir(parents=True, exist_ok=True)
    missed = []
    for name, file, old, new in MUTANTS:
        for f in FILES:
            shutil.copy(HERE / f, SCRATCH / f)
        text = (SCRATCH / file).read_text()
        if new is None:
            mutated = COMPUTED[name](text)
        else:
            if old not in text:
                raise SystemExit(f"mutant {name!r}: text not found")
            mutated = text.replace(old, new, 1)
        if mutated == text:
            raise SystemExit(f"mutant {name!r}: no change")
        (SCRATCH / file).write_text(mutated)
        build = subprocess.run(
            ["bash", str(HERE / "build_windows_player_markers_original_check.sh"), str(SCRATCH), str(SCRATCH)],
            capture_output=True,
            text=True,
        )
        if build.returncode:
            raise SystemExit(f"mutant {name!r} does not build:\n{build.stderr}")
        run = subprocess.run(
            [
                str(SCRATCH / "player-markers-original-check.exe"),
                f"W:\\Daywater\\research-temp\\position-neutral-{ctx}.mem1",
                DLL,
            ],
            capture_output=True,
            text=True,
        )
        caught = run.returncode != 0
        print(("caught " if caught else "MISSED ") + name)
        if not caught:
            missed.append(name)
    print(f"{len(MUTANTS) - len(missed)} of {len(MUTANTS)} mutants caught")
    return 1 if missed else 0


if __name__ == "__main__":
    sys.exit(main())
