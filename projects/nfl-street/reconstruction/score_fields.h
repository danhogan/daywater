/* Observed field layout for the tested GNNE69 match object. Names are ours. */
#ifndef NFLSTREET_SCORE_FIELDS_H
#define NFLSTREET_SCORE_FIELDS_H

#include <stddef.h>

typedef unsigned short ScoreU16;
typedef unsigned int ScoreU32;

typedef struct MatchScoreObject {
    unsigned char unknown_00_3b[0x3c];
    ScoreU16 right_score; /* side 0, object +0x3c */
    ScoreU16 left_score;  /* side 1, object +0x3e */
    ScoreU32 unknown_40; /* set to 1 for nonzero writer arguments */
} MatchScoreObject;

typedef char score_u16_width_check[(sizeof(ScoreU16) == 2) ? 1 : -1];
typedef char score_u32_width_check[(sizeof(ScoreU32) == 4) ? 1 : -1];
typedef char score_right_offset_check[(offsetof(MatchScoreObject, right_score) == 0x3c) ? 1 : -1];
typedef char score_left_offset_check[(offsetof(MatchScoreObject, left_score) == 0x3e) ? 1 : -1];
typedef char score_unknown_40_offset_check[(offsetof(MatchScoreObject, unknown_40) == 0x40) ? 1 : -1];

/* Placeholder symbol: the original DOL accesses this pointer at r13-0x7078. */
extern MatchScoreObject *g_match_score_object;

ScoreU16 score_for_side(int side);
void set_score_for_side(int side, int value);
void clear_score_unknown_40(void);
void add_score_for_side(int side, int amount);

#endif
