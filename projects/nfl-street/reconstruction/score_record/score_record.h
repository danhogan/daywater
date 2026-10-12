/* Observed input layout for GNNE69 function 0x80025ECC. Names are ours. */
#ifndef NFLSTREET_SCORE_RECORD_H
#define NFLSTREET_SCORE_RECORD_H

#include "../score_fields.h"

typedef struct ScoreEventRecord {
    ScoreU32 field_00;
    unsigned char unknown_04_17[0x14];
    ScoreU32 flags_18;
    unsigned char side_1c;
    signed char amount_1d;
} ScoreEventRecord;

typedef char score_record_flags_offset_check[
    (offsetof(ScoreEventRecord, flags_18) == 0x18) ? 1 : -1];
typedef char score_record_side_offset_check[
    (offsetof(ScoreEventRecord, side_1c) == 0x1c) ? 1 : -1];
typedef char score_record_amount_offset_check[
    (offsetof(ScoreEventRecord, amount_1d) == 0x1d) ? 1 : -1];

void apply_score_record(const ScoreEventRecord *record, int control);

#endif
