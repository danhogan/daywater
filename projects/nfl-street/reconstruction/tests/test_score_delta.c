/* Host/Windows semantic check of the reconstructed score slice. */

#include "../score_fields.h"
#include "../score_record/score_record.h"
#include "../score_table/score_table.h"
#include "../../native/fnig_store.h"
#include <stdio.h>
#include <string.h>

static MatchScoreObject object;
MatchScoreObject *g_match_score_object = &object;
static int update_calls;
static int signal_calls;
static int last_signal;
static int order_error;
static int followup_calls;
static int implicit_calls;
static ScoreU32 last_followup_code;

void score_event_update(int side, ScoreU32 value)
{
    ++update_calls;
    if (score_table_get_for_side(side) != value ||
        score_for_side(side) != (ScoreU16)value) {
        order_error = 1;
    }
}

void score_event_signal(int value)
{
    ++signal_calls;
    last_signal = value;
}

ScoreU32 score_record_implicit_code(void)
{
    ++implicit_calls;
    return 0x1234U;
}

void score_record_followup(int a, int b, ScoreU32 code, int d)
{
    ++followup_calls;
    last_followup_code = code;
    if (a != 9 || b != 0 || d != 0) {
        order_error = 1;
    }
}

static void reset_state(void)
{
    memset(&object, 0, sizeof(object));
    native_fnig_reset();
    update_calls = signal_calls = last_signal = 0;
    order_error = 0;
    followup_calls = implicit_calls = 0;
    last_followup_code = 0;
}

int main(void)
{
    ScoreEventRecord record;

    reset_state();
    score_table_set_for_side(1, 6);
    if (native_fnig_field_value(0x43534147U) != 6 ||
        native_fnig_field_value(0x43534847U) != 0) {
        return 13;
    }
    add_score_for_side(1, 1);
    if (score_table_get_for_side(1) != 7 || score_for_side(1) != 7 ||
        object.unknown_40 != 1 ||
        update_calls != 1 || signal_calls != 1 || last_signal != 1 || order_error) {
        return 1;
    }

    /* The original table and RAM helpers choose differently for side 2. */
    reset_state();
    add_score_for_side(2, 3);
    if (native_fnig_field_value(SCORE_FIELD_CSAG) != 3 ||
        native_fnig_field_value(SCORE_FIELD_CSHG) != 0 ||
        score_for_side(2) != 3 || score_for_side(1) != 0 || order_error) {
        return 14;
    }

    reset_state();
    score_table_set_for_side(0, 250);
    add_score_for_side(0, 10);
    if (score_table_get_for_side(0) != 255 || score_for_side(0) != 255 ||
        update_calls != 1 || signal_calls != 1 || order_error) {
        return 2;
    }

    reset_state();
    score_table_set_for_side(0, 17);
    object.unknown_40 = 1;
    add_score_for_side(0, 0);
    if (score_table_get_for_side(0) != 17 || score_for_side(0) != 17 ||
        object.unknown_40 != 1 || update_calls != 0 || signal_calls != 0 || order_error) {
        return 3;
    }

    reset_state();
    object.unknown_40 = 1;
    add_score_for_side(0, 0);
    if (score_table_get_for_side(0) != 0 || score_for_side(0) != 0 ||
        object.unknown_40 != 0 || update_calls != 0 || signal_calls != 0 || order_error) {
        return 4;
    }

    reset_state();
    add_score_for_side(0, -1);
    if (score_table_get_for_side(0) != 255 || score_for_side(0) != 255 ||
        update_calls != 1 || signal_calls != 1 || order_error) {
        return 5;
    }

    reset_state();
    memset(&record, 0, sizeof(record));
    record.amount_1d = 2;
    apply_score_record(&record, 1);
    if (score_table_get_for_side(0) != 4 || score_for_side(0) != 4 ||
        followup_calls != 0 || order_error) {
        return 6;
    }

    reset_state();
    record.flags_18 = 4;
    apply_score_record(&record, 1);
    if (score_table_get_for_side(0) != 1 || score_for_side(0) != 1 || order_error) {
        return 7;
    }

    reset_state();
    record.flags_18 = 4U | 0x80000U;
    apply_score_record(&record, 1);
    if (score_table_get_for_side(0) != 2 || score_for_side(0) != 2 || order_error) {
        return 8;
    }

    reset_state();
    memset(&record, 0, sizeof(record));
    record.amount_1d = -3;
    apply_score_record(&record, 1);
    if (score_table_get_for_side(1) != 3 || score_for_side(1) != 3 ||
        score_table_get_for_side(0) != 0 || followup_calls != 0 || order_error) {
        return 9;
    }

    reset_state();
    record.amount_1d = -2;
    record.field_00 = 0x456U;
    apply_score_record(&record, 0);
    if (score_table_get_for_side(1) != 2 || followup_calls != 1 ||
        last_followup_code != 0x456U || implicit_calls != 0 || order_error) {
        return 10;
    }

    reset_state();
    record.field_00 = 0;
    apply_score_record(&record, 0);
    if (score_table_get_for_side(1) != 2 || followup_calls != 1 ||
        last_followup_code != 0x3401U || implicit_calls != 1 || order_error) {
        return 11;
    }

    reset_state();
    record.amount_1d = -128;
    apply_score_record(&record, 0);
    if (score_table_get_for_side(1) != 255 || followup_calls != 0 || order_error) {
        return 12;
    }

    puts("reconstructed score path passed");
    return 0;
}
