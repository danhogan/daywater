/*
 * Byte-matching reconstruction of GNNE69 DOL function 0x80104294.
 * GCC 15.2.1 with the recorded flags emits the observed 52 bytes. Empty
 * assembly constraints guide the pointer register and memory access order;
 * they add no PowerPC instruction. This does not identify EA's original
 * source or compiler. score_writer.c remains the portable semantic model.
 */

typedef unsigned int ScoreU32;
typedef unsigned short ScoreU16;

typedef struct MatchScoreObject {
    unsigned char unknown_00_3b[0x3c];
    ScoreU16 right_score;
    ScoreU16 left_score;
    ScoreU32 unknown_40;
} MatchScoreObject;

/* r13 - 0x7078 addresses the verified pointer slot in this GNNE69 DOL. */
register ScoreU32 sda_base __asm__("r13");

void set_score_for_side(int side, int value)
{
    if (side == 1) {
        __asm__ volatile ("" : : "i"(1) : "memory");
        register MatchScoreObject *match __asm__("r9") =
            *(MatchScoreObject **)(sda_base - 0x7078u);
        __asm__ volatile ("" : : "r"(match));
        match->left_score = (ScoreU16)value;
    } else {
        __asm__ volatile ("" : : "i"(0) : "memory");
        register MatchScoreObject *match __asm__("r9") =
            *(MatchScoreObject **)(sda_base - 0x7078u);
        __asm__ volatile ("" : : "r"(match));
        match->right_score = (ScoreU16)value;
    }

    if (value == 0)
        return;

    __asm__ volatile ("" : : : "memory");
    register MatchScoreObject *match __asm__("r9") =
        *(MatchScoreObject **)(sda_base - 0x7078u);
    register ScoreU32 one __asm__("r0") = 1;
    __asm__ volatile ("" : "+r"(match), "+r"(one));
    match->unknown_40 = one;
}
