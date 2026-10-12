/* Minimal Windows/host storage for the two researched FNIG score fields. */
#ifndef NFLSTREET_NATIVE_FNIG_STORE_H
#define NFLSTREET_NATIVE_FNIG_STORE_H

#include "../reconstruction/score_fields.h"

void native_fnig_reset(void);
ScoreU32 native_fnig_field_value(ScoreU32 field);

#endif
