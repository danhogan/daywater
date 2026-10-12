/* Semantic candidates for GNNE69 DOL functions 0x80053128/0x80053180.
 * The query interface is a native portability boundary, not original ABI.
 */

#include "score_table.h"

extern void fnig_query_select(ScoreU32 field, ScoreU32 *result);
extern void fnig_query_update(ScoreU32 field, ScoreU32 value);

ScoreU32 score_table_get_for_side(int side)
{
    ScoreU32 result = 0;
    ScoreU32 field = side == 0 ? SCORE_FIELD_CSHG : SCORE_FIELD_CSAG;
    fnig_query_select(field, &result);
    return result;
}

void score_table_set_for_side(int side, ScoreU32 value)
{
    ScoreU32 field = side == 0 ? SCORE_FIELD_CSHG : SCORE_FIELD_CSAG;
    fnig_query_update(field, value);
}
