/* Replay one captured GNNE69 score-handler input through curated C candidates. */

#include "../reconstruction/score_fields.h"
#include "../reconstruction/score_record/score_record.h"
#include "../reconstruction/score_table/score_table.h"
#include "fnig_store.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static MatchScoreObject score_object;
MatchScoreObject *g_match_score_object = &score_object;
static unsigned updates, signals, followups;

void score_event_update(int side, ScoreU32 value)
{
    (void)side;
    (void)value;
    ++updates;
}

void score_event_signal(int value)
{
    (void)value;
    ++signals;
}

ScoreU32 score_record_implicit_code(void)
{
    return 0;
}

void score_record_followup(int a, int b, ScoreU32 code, int d)
{
    (void)a;
    (void)b;
    (void)code;
    (void)d;
    ++followups;
}

static int hex_digit(char value)
{
    if (value >= '0' && value <= '9') return value - '0';
    if (value >= 'a' && value <= 'f') return value - 'a' + 10;
    if (value >= 'A' && value <= 'F') return value - 'A' + 10;
    return -1;
}

static int parse_record(const char *text, unsigned char bytes[0x20])
{
    unsigned i;
    if (strlen(text) != 0x40) return 0;
    for (i = 0; i < 0x20; ++i) {
        int high = hex_digit(text[2 * i]);
        int low = hex_digit(text[2 * i + 1]);
        if (high < 0 || low < 0) return 0;
        bytes[i] = (unsigned char)((high << 4) | low);
    }
    return 1;
}

static int parse_int(const char *text, int minimum, int maximum, int *result)
{
    char *end;
    long value = strtol(text, &end, 0);
    if (!text[0] || *end || value < minimum || value > maximum) return 0;
    *result = (int)value;
    return 1;
}

int main(int argc, char **argv)
{
    unsigned char bytes[0x20];
    ScoreEventRecord record;
    int control, side_0, side_1;
    if (argc != 5 || !parse_record(argv[1], bytes) ||
        !parse_int(argv[2], -128, 255, &control) ||
        !parse_int(argv[3], 0, 255, &side_0) ||
        !parse_int(argv[4], 0, 255, &side_1)) {
        fputs("usage: score_event_replay RECORD_HEX CONTROL SIDE_0 SIDE_1\n", stderr);
        return 2;
    }
    memset(&record, 0, sizeof(record));
    memset(&score_object, 0, sizeof(score_object));
    native_fnig_reset();
    record.field_00 = ((ScoreU32)bytes[0] << 24) | ((ScoreU32)bytes[1] << 16) |
                      ((ScoreU32)bytes[2] << 8) | bytes[3];
    record.flags_18 = ((ScoreU32)bytes[0x18] << 24) |
                      ((ScoreU32)bytes[0x19] << 16) |
                      ((ScoreU32)bytes[0x1A] << 8) | bytes[0x1B];
    record.side_1c = bytes[0x1C];
    record.amount_1d = (signed char)bytes[0x1D];
    if (control == 0 && record.amount_1d == -2) {
        fputs("record requires an unmodeled follow-up service\n", stderr);
        return 3;
    }
    score_object.right_score = (ScoreU16)side_0;
    score_object.left_score = (ScoreU16)side_1;
    score_table_set_for_side(0, (ScoreU32)side_0);
    score_table_set_for_side(1, (ScoreU32)side_1);
    apply_score_record(&record, control);
    printf("{\"side_0\":%u,\"side_1\":%u,\"named_side_0\":%u,"
           "\"named_side_1\":%u,\"updates\":%u,\"signals\":%u,"
           "\"followups\":%u}\n",
           (unsigned)score_object.right_score, (unsigned)score_object.left_score,
           (unsigned)native_fnig_field_value(SCORE_FIELD_CSHG),
           (unsigned)native_fnig_field_value(SCORE_FIELD_CSAG),
           updates, signals, followups);
    return 0;
}
