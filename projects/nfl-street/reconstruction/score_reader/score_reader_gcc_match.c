/*
 * Byte-matching reconstruction of GNNE69 DOL function 0x80104274.
 * GCC 15.2.1 with the recorded flags emits the observed 32 bytes. Empty
 * assembly constraints guide branch layout and register allocation but add
 * no PowerPC instruction. This does not identify the game's original source
 * or compiler. score_reader.c remains the portable semantic candidate.
 */

typedef unsigned int ScoreU32;
typedef unsigned short ScoreU16;

typedef struct MatchScoreObject {
    unsigned char unknown_00_3b[0x3c];
    ScoreU16 right_score;
    ScoreU16 left_score;
} MatchScoreObject;

/* r13 - 0x7078 addresses the verified pointer slot in this GNNE69 DOL. */
register ScoreU32 sda_base __asm__("r13");

ScoreU32 score_for_side(int side)
{
    if (side == 1) {
        __asm__ volatile ("" : : "i"(1) : "memory");
        register MatchScoreObject *match __asm__("r9") =
            *(MatchScoreObject **)(sda_base - 0x7078u);
        register ScoreU32 result __asm__("r3") = match->left_score;
        __asm__ volatile ("" : "+r"(match), "+r"(result));
        return result;
    }

    __asm__ volatile ("" : : "i"(0) : "memory");
    register MatchScoreObject *match __asm__("r9") =
        *(MatchScoreObject **)(sda_base - 0x7078u);
    register ScoreU32 result __asm__("r3") = match->right_score;
    __asm__ volatile ("" : "+r"(match), "+r"(result));
    return result;
}
