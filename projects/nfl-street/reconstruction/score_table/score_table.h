/* Named score fields selected by GNNE69 DOL functions 0x80053128/0x80053180. */
#ifndef NFLSTREET_SCORE_TABLE_H
#define NFLSTREET_SCORE_TABLE_H

#include "../score_fields.h"

#define SCORE_FIELD_CSHG 0x43534847U
#define SCORE_FIELD_CSAG 0x43534147U

ScoreU32 score_table_get_for_side(int side);
void score_table_set_for_side(int side, ScoreU32 value);

#endif
