# Locked-cache product candidate

**Date:** 2026-10-05  
**Game:** NFL Street, GNNE69 revision 0, ISO SHA-1
`813c4511ba3135059a13de424555647e8252254b`  
**Confidence:** `verified` for the specified synthetic and live comparisons;
other states, memory maps, and exceptional floating-point inputs remain open.

The first [live differential probe](live_differential_probe.md) found that the
guarded product candidate declined 16 of its first 32 calls and 29 of 32
calls after its millionth invocation. A follow-up diagnostic DLL logged the
16 early declines as guard precondition failures. Every one had an `r3`
input and `r5` output in the `0xE0000000` address range. Their GQR0 and
exception fields were zero. The log is
`W:\Daywater\research-temp\nflstreet-guard-reasons-early.log`.

The local ModernGekko/Dolphin runtime maps this range to its 256 KiB L1
locked-cache backing. Its `HookExternalPointer` exposes a host pointer only
for ranges fully inside that cache; generated paired-single loads and stores
go through `mem_read32` and `mem_write32`, which use the runtime's external
memory hooks for this range. The product candidate now permits that exact
range when read, write, and pointer callbacks are available and the pointer
callback validates the whole requested span. Other unsupported addresses
still decline the candidate.

The [offline locked-cache checker](../reconstruction/paired_product/check_locked_cache.c)
uses separate fake locked caches for the original Linux whole-DOL module and
candidate. It includes output that overlaps the input cache area on half the
trials. It compared full CPU state, stack bytes, and locked-cache bytes:
**10,000 matches, zero skips, zero mismatches, zero dispatch failures**. The
existing MEM1 checker still reports **100,000 full-state matches**, zero
skips and zero mismatches.

The experimental [live probe helper](live_diff_probe.inc) cloned MEM1 and
locked cache, redirected the candidate's external callbacks to those private
copies, then ran the original generated instructions on live state. The
locked-cache candidate matched full CPU state and touched memory in **32/32
early** product calls and **32/32 later** calls after the millionth
invocation. The polynomial candidate also matched 32/32 calls in each
window. The 120-frame and 1,804-frame runs both exited cleanly with zero
interpreter fallback and failed chunks. Logs are
`W:\Daywater\research-temp\nflstreet-lc-live-diff-{early,late}.log`; the
diagnostic DLL is `gGNNE69-recomp-lc-live-diff.dll` in the same private
directory, SHA-1 `734cb2fd484628aaa371677ad94936e5015584fc`.

The normal guarded [product candidate](../reconstruction/paired_product/full_state_candidate.c)
now includes this access path. It was built with the polynomial candidate as
a separate Windows DLL,
`W:\Daywater\research-temp\gGNNE69-recomp-lc-two-fastpaths.dll`, SHA-1
`a843d8dd9afdb912a9d590abaa352622b3a83947`. A 120-frame saved-match
smoke run logged a hit for both candidates, clean exit, zero fallback steps,
and zero failed chunks. The bundled baseline remains unchanged.

## Paired performance trial

Three alternating pairs of 600-frame, headless saved-match trials compared
the known baseline DLL with the new combined candidate. Every run exited
cleanly with zero interpreter fallback and failed chunks. The candidate
logged hits for both functions. Median observed FPS was **24.11 baseline**
and **23.60 candidate** in this batch. The six observed rates were baseline
26.03, 24.11, 23.51 and candidate 23.53, 23.60, 24.14. Both variants shifted
substantially during the batch; these data do not demonstrate a speed gain.
Logs and `summary.json` are at
`W:\Daywater\research-temp\nflstreet-lc-two-fastpaths-600x3`.

A second alternating batch compared the new DLL directly with the previous
two-function DLL, whose product guard still declined locked-cache calls.
Median observed FPS was **24.14 previous** and **23.73 new**. Per-round
rates were 24.14/26.01, 24.87/22.71, and 21.96/23.73 FPS (previous/new).
The direction reversed between rounds as host throughput changed. These
trials also do not isolate a reliable performance effect from the added
coverage. Logs and `summary.json` are at
`W:\Daywater\research-temp\nflstreet-lc-vs-old-two-fastpaths-600x3`.
