/*
 * Byte-matching reconstruction of GNNE69 DOL function 0x801042C8.
 * The register constraints reproduce the observed register allocation with
 * Clang 21.1.0; they do not identify the game's original compiler or source.
 * This file is PowerPC-specific. clear_unknown_40.c remains the portable
 * semantic candidate used by host-side tests.
 */

#include "../score_fields.h"

/* The verified DOL initializes r13 to 0x803A9A40. The pointer slot is at
 * r13 - 0x7078 = 0x803A29C8 for this GNNE69 revision. */
register ScoreU32 sda_base __asm__("r13");

void clear_score_unknown_40(void)
{
    register MatchScoreObject *match __asm__("r9") =
        *(MatchScoreObject **)(sda_base - 0x7078u);
    register ScoreU32 zero __asm__("r0") = 0;

    /* Constrain allocation without supplying any PowerPC instruction. */
    __asm__ volatile ("" : "+r"(match), "+r"(zero));
    match->unknown_40 = zero;
}
