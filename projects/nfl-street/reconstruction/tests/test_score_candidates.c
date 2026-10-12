/* Host-side semantic check only; this does not establish a PowerPC match. */

#include "../score_fields.h"

static MatchScoreObject object;
MatchScoreObject *g_match_score_object = &object;

int main(void)
{
    set_score_for_side(1, 7);
    if (score_for_side(1) != 7 || score_for_side(0) != 0 || object.unknown_40 != 1) {
        return 1;
    }

    clear_score_unknown_40();
    if (score_for_side(1) != 7 || object.unknown_40 != 0) {
        return 2;
    }

    /* The original compares the full input even though sth stores only 16 bits. */
    set_score_for_side(2, 0x10000);
    if (score_for_side(2) != 0 || object.unknown_40 != 1) {
        return 3;
    }

    clear_score_unknown_40();
    set_score_for_side(0, 17);
    if (score_for_side(0) != 17 || score_for_side(1) != 7 || object.unknown_40 != 1) {
        return 4;
    }
    return 0;
}
