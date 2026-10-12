/*
 * Semantic reconstruction candidate for GNNE69 DOL function 0x80104274.
 * DOL SHA-1: 46e3de854998a0f66c9be8c72a1164d22cd29eb4
 *
 * This source has not been compiled with the original PowerPC compiler or
 * shown to match the eight target instructions byte for byte.
 */

#include "../score_fields.h"

ScoreU16 score_for_side(int side)
{
    if (side != 1) {
        return g_match_score_object->right_score;
    }
    return g_match_score_object->left_score;
}
