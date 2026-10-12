/* Semantic candidate for GNNE69 DOL function 0x80025ECC. */

#include "score_record.h"

extern ScoreU32 score_record_implicit_code(void); /* 0x80103CA0 */
extern void score_record_followup(int, int, ScoreU32, int); /* 0x80102F0C */

void apply_score_record(const ScoreEventRecord *record, int control)
{
    int side = record->side_1c;
    int amount = record->amount_1d;

    if (amount == 2) {
        if ((record->flags_18 & 4U) == 0) {
            amount = 4;
        } else if ((record->flags_18 & 0x80000U) == 0) {
            amount = 1;
        }
    }
    if (amount < 0) {
        side ^= 1;
        /* PPC neg followed by extsb retains -128 as -128. */
        amount = amount == -128 ? -128 : -amount;
    }
    add_score_for_side(side, amount);

    if (control == 0 && record->amount_1d == -2) {
        ScoreU32 code = record->field_00;
        if (code == 0) {
            code = ((score_record_implicit_code() & 0xFFU) << 8) | 1U;
        }
        score_record_followup(9, 0, code, 0);
    }
}
