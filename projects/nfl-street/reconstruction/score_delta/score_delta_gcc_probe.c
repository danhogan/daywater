/*
 * PowerPC code-generation probe for GNNE69 DOL function 0x8002676C.
 * This is not byte-matched. The empty constraint preserves a redundant
 * halfword conversion visible in the DOL, without adding an instruction.
 * The portable semantic model remains score_delta.c.
 */

typedef unsigned short ScoreU16;
typedef unsigned int ScoreU32;

extern void clear_score_unknown_40(void);     /* 0x801042C8 */
extern ScoreU32 score_table_get_for_side(int); /* 0x80053128 */
extern void set_score_for_side(int, int);      /* 0x80104294 */
extern void score_table_set_for_side(int, ScoreU32); /* 0x80053180 */
extern void score_event_update(int, ScoreU32); /* 0x80109030 */
extern void score_event_signal(int);           /* 0x8010A384 */

void add_score_for_side(int side, int amount)
{
    ScoreU32 next;

    clear_score_unknown_40();
    next = score_table_get_for_side(side) + (ScoreU32)amount;
    if (next > 0xffu)
        next = 0xffu;

    __asm__ volatile ("" : "+r"(next));
    set_score_for_side(side, (int)(ScoreU16)next);
    score_table_set_for_side(side, next);
    if (amount != 0) {
        score_event_update(side, next);
        score_event_signal(1);
    }
}
