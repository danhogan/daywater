"""Stage or revert live side-by-side checks in private generated GNNE69 chunks.

The candidate runs on a private MEM1 copy; the original generated code runs
on live state. This is a diagnostic DLL, not a faster module or default build.
"""

from __future__ import annotations

import argparse
from pathlib import Path
import shutil


NATIVE = Path(__file__).resolve().parent
RECONSTRUCTION = NATIVE.parent / "reconstruction"
SPECS = (
    dict(name="odd", chunk="chunk_0098_text1_80187480.c", func="func_80187480",
         entry="80188BB0", ret="80188C20", end="80188C24", stack=16,
         candidate="nflstreet_odd_polynomial_cpu_candidate",
         source=RECONSTRUCTION / "odd_polynomial_pair/full_state_candidate.c",
         header=RECONSTRUCTION / "odd_polynomial_pair/full_state_candidate.h",
         staged="odd_fastpath.inc", staged_header="full_state_candidate.h",
         outputs="ctx->gpr[5], 4u, ctx->gpr[6], 4u"),
    dict(name="product", chunk="chunk_0128_text1_801FF480.c", func="func_801FF480",
         entry="801FFDF0", ret="801FFEB8", end="801FFEBC", stack=64,
         candidate="nflstreet_paired_product_cpu_candidate",
         source=RECONSTRUCTION / "paired_product/full_state_candidate.c",
         header=RECONSTRUCTION / "paired_product/paired_product_full_state_candidate.h",
         staged="product_fastpath.inc",
         staged_header="paired_product_full_state_candidate.h",
         outputs="ctx->gpr[5], 48u, 0u, 0u"),
)


def replace_once(data: bytes, old: bytes, new: bytes) -> bytes:
    if data.count(old) != 1:
        raise RuntimeError(f"expected one marker {old[:70]!r}; found {data.count(old)}")
    return data.replace(old, new, 1)


def changes(spec: dict[str, object]) -> tuple[tuple[bytes, bytes], ...]:
    name = str(spec["name"])
    entry = str(spec["entry"])
    ret = str(spec["ret"])
    end = str(spec["end"])
    func = str(spec["func"])
    include_old = b'#include "../generated.h"\r\n'
    diagnostic = (b'#define NFLSTREET_PRODUCT_GUARD_DIAGNOSTIC 1\r\n'
                  if name == "product" else b'')
    include_new = (include_old + diagnostic +
                   f'#include "{spec["staged"]}"\r\n'.encode() +
                   b'#include "live_diff_probe.inc"\r\n')
    func_old = f"void {func}(CPUState* ctx) {{\r\n    switch (ctx->pc) {{".encode()
    func_new = (f"void {func}(CPUState* ctx) {{\r\n"
                f"    NFLStreetLiveProbe nfl_probe;\r\n"
                f"    nfl_probe.active = false;\r\n"
                f"    static unsigned nfl_seen, nfl_start, nfl_attempts, nfl_matches, "
                f"nfl_skips, nfl_mismatches;\r\n"
                f"    static bool nfl_configured;\r\n"
                f"    static bool nfl_reported;\r\n"
                f"    switch (ctx->pc) {{").encode()
    entry_old = f"label_{entry}:\r\n    ctx->pc = 0x{entry}u;".encode()
    entry_new = (f"label_{entry}:\r\n"
                 f"    if (!nfl_configured) {{\r\n"
                 f"        const char* text = getenv(\"NFLSTREET_LIVE_DIFF_START\");\r\n"
                 f"        if (text) {{\r\n"
                 f"            unsigned long parsed = strtoul(text, NULL, 10);\r\n"
                 f"            if (parsed <= 1000000000ul) nfl_start = (unsigned)parsed;\r\n"
                 f"        }}\r\n"
                 f"        nfl_configured = true;\r\n"
                 f"    }}\r\n"
                 f"    ++nfl_seen;\r\n"
                 f"    if (nfl_seen > nfl_start && nfl_attempts < 32u) {{\r\n"
                 f"        ++nfl_attempts;\r\n"
                 f"        if (!nflstreet_live_probe_prepare(&nfl_probe, ctx, "
                 f"{spec['candidate']}, ctx->gpr[1] - {spec['stack']}u, "
                 f"{spec.get('stack_bytes', 4 if name == 'odd' else 48)}u, "
                 f"{spec['outputs']}))\r\n"
                 f"            ++nfl_skips;\r\n"
                 f"    }}\r\n"
                 f"    ctx->pc = 0x{entry}u;").encode()
    return_old = (f"label_{ret}:\r\n"
                  f"    ctx->pc = 0x{ret}u;\r\n"
                  f"    // {ret}: blr\r\n"
                  f"    {{\r\n"
                  f"        u32 target = ctx->lr & ~3u;\r\n"
                  f"        bool ctr_ok = true;\r\n"
                  f"        bool cr_ok = true;\r\n"
                  f"        if (ctr_ok && cr_ok) {{\r\n"
                  f"            ctx->pc = target;\r\n"
                  f"            goto return_dispatch_{func.removeprefix('func_')};\r\n"
                  f"        }}\r\n"
                  f"    }}\r\n\r\n"
                  f"label_{end}:").encode()
    return_new = return_old.replace(
        f"            goto return_dispatch_{func.removeprefix('func_')};\r\n".encode(),
        (f"            if (nfl_probe.active) {{\r\n"
         f"                if (nflstreet_live_probe_equal(&nfl_probe, ctx, 0x{entry}u, "
         f"nfl_attempts)) ++nfl_matches;\r\n"
         f"                else ++nfl_mismatches;\r\n"
         f"            }}\r\n"
         f"            if (nfl_attempts == 32u && !nfl_reported) {{\r\n"
         f"                nfl_reported = true;\r\n"
         f"                fprintf(stderr, \"[nflstreet-live-diff] summary pc={entry.lower()} "
         f"start=%u seen=%u attempts=%u matches=%u skips=%u mismatches=%u\\n\", "
         f"nfl_start, nfl_seen, nfl_attempts, nfl_matches, nfl_skips, "
         f"nfl_mismatches);\r\n"
         f"            }}\r\n"
         f"            goto return_dispatch_{func.removeprefix('func_')};\r\n").encode())
    return ((include_old, include_new), (func_old, func_new),
            (entry_old, entry_new), (return_old, return_new))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("apply", "revert"))
    parser.add_argument("generated", type=Path)
    args = parser.parse_args()
    generated = args.generated.resolve()
    chunks = generated / "chunks"
    helper = chunks / "live_diff_probe.inc"
    if not chunks.is_dir() or not (generated / "generated.h").is_file():
        parser.error("expected known generated GNNE69 C module layout")
    if args.action == "apply" and helper.exists():
        parser.error("probe already staged")
    if args.action == "revert" and helper.read_bytes() != (NATIVE / helper.name).read_bytes():
        parser.error("staged probe missing or modified")
    updates = []
    for spec in SPECS:
        chunk = chunks / str(spec["chunk"])
        staged = chunks / str(spec["staged"])
        staged_header = chunks / str(spec["staged_header"])
        if args.action == "apply":
            if staged.exists() or staged_header.exists():
                parser.error(f"{spec['name']} candidate already staged")
        elif (staged.read_bytes() != Path(spec["source"]).read_bytes() or
              staged_header.read_bytes() != Path(spec["header"]).read_bytes()):
            parser.error(f"{spec['name']} staged files missing or modified")
        data = chunk.read_bytes()
        for original, replacement in changes(spec):
            data = replace_once(data, original, replacement) if args.action == "apply" else \
                   replace_once(data, replacement, original)
        updates.append((chunk, data, staged, staged_header, spec))
    for chunk, data, staged, staged_header, spec in updates:
        chunk.write_bytes(data)
        if args.action == "apply":
            shutil.copyfile(Path(spec["source"]), staged)
            shutil.copyfile(Path(spec["header"]), staged_header)
        else:
            staged.unlink()
            staged_header.unlink()
    if args.action == "apply":
        shutil.copyfile(NATIVE / helper.name, helper)
    else:
        helper.unlink()
    print(f"{args.action}: both live-differential probes")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
