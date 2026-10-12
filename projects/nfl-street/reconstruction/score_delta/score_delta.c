/* Semantic candidate for GNNE69 DOL function 0x8002676C.
 * The service names are placeholders for still-unreconstructed calls.
 */

#include "../score_fields.h"
#include "../score_table/score_table.h"

extern void score_event_update(int side, ScoreU32);      /* 0x80109030 */
extern void score_event_signal(int value);               /* 0x8010A384 */

void add_score_for_side(int side, int amount)
{
    ScoreU32 next;

    clear_score_unknown_40();
    next = score_table_get_for_side(side) + (ScoreU32)amount;
    if (next > 0xFFU) {
        next = 0xFFU;
    }
    set_score_for_side(side, (int)(ScoreU16)next);
    score_table_set_for_side(side, next);
    if (amount != 0) {
        score_event_update(side, next);
        score_event_signal(1);
    }
}
