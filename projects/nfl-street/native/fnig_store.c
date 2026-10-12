/* Host implementation of the FNIG query boundary for the score slice.
 * This is not a reconstruction of the game's database engine.
 */

#include "fnig_store.h"
#include "../reconstruction/score_table/score_table.h"

static ScoreU32 cshg;
static ScoreU32 csag;

void native_fnig_reset(void)
{
    cshg = 0;
    csag = 0;
}

ScoreU32 native_fnig_field_value(ScoreU32 field)
{
    if (field == SCORE_FIELD_CSHG) {
        return cshg;
    }
    if (field == SCORE_FIELD_CSAG) {
        return csag;
    }
    return 0;
}

void fnig_query_select(ScoreU32 field, ScoreU32 *result)
{
    *result = native_fnig_field_value(field);
}

void fnig_query_update(ScoreU32 field, ScoreU32 value)
{
    if (field == SCORE_FIELD_CSHG) {
        cshg = value;
    } else if (field == SCORE_FIELD_CSAG) {
        csag = value;
    }
}
