/*
 * Semantic reconstruction candidate for GNNE69 DOL function 0x80104294.
 * DOL SHA-1: 46e3de854998a0f66c9be8c72a1164d22cd29eb4
 *
 * Static writes are verified. This source has not been shown to match the
 * original compiler output or to execute during a particular scoring event.
 */

#include "../score_fields.h"

void set_score_for_side(int side, int value)
{
    if (side == 1) {
        g_match_score_object->left_score = (ScoreU16)value;
    } else {
        g_match_score_object->right_score = (ScoreU16)value;
    }
    if (value != 0) {
        g_match_score_object->unknown_40 = 1;
    }
}
