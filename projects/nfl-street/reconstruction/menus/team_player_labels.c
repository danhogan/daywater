#include "team_player_labels.h"

#include <limits.h>
#include <string.h>

static size_t fixed_text_length(const uint8_t *text, size_t capacity) {
    size_t length = 0;
    while (length < capacity && text[length] != 0) length++;
    return length;
}

static void copy_limited(char *destination, size_t capacity,
                         const char *source, size_t source_length) {
    size_t copy_length;
    if (!capacity) return;
    copy_length = source_length < capacity - 1 ? source_length : capacity - 1;
    if (copy_length) memcpy(destination, source, copy_length);
    destination[copy_length] = '\0';
}

nflstreet_label_status nflstreet_team_label_name(
    const uint8_t first_name[NFLSTREET_LABEL_FIRST_BYTES],
    const uint8_t last_name[NFLSTREET_LABEL_LAST_BYTES],
    char *output, size_t output_capacity) {
    size_t first_length, last_length, at = 0;

    if (!first_name || !last_name || !output || output_capacity == 0)
        return NFLSTREET_LABEL_INVALID_ARGUMENT;
    first_length = fixed_text_length(first_name, NFLSTREET_LABEL_FIRST_BYTES);
    last_length = fixed_text_length(last_name, NFLSTREET_LABEL_LAST_BYTES);
    if (first_length) {
        size_t n = first_length < output_capacity - 1 ? first_length : output_capacity - 1;
        memcpy(output, first_name, n);
        at = n;
    }
    if (last_length) {
        if (first_length && at < output_capacity - 1) output[at++] = ' ';
        size_t remaining = output_capacity - 1 - at;
        size_t n = last_length < remaining ? last_length : remaining;
        if (n) memcpy(output + at, last_name, n);
        at += n;
    }
    output[at] = '\0';
    return NFLSTREET_LABEL_OK;
}

nflstreet_label_status nflstreet_team_label_build(
    int32_t cursor, int32_t mode,
    const nflstreet_team_label_row *rows, size_t count,
    const char *const *role_texts, size_t role_text_count,
    char *name, size_t name_capacity,
    char *role_text, size_t role_capacity,
    uint32_t *scalar,
    uint32_t ratings[NFLSTREET_LABEL_RATING_COUNT]) {
    static const uint8_t rating_ordinal[NFLSTREET_LABEL_RATING_COUNT] = {
        6, 4, 7, 0, 3, 2, 1, 5, 8, 9
    };
    int64_t effective_index;

    if (!name || !name_capacity || !role_text || !role_capacity ||
        !scalar || !ratings || count > INT32_MAX || (count && !rows))
        return NFLSTREET_LABEL_INVALID_ARGUMENT;

    effective_index = (int64_t)cursor + (int64_t)mode;
    if (effective_index >= (int64_t)count)
        effective_index -= (int64_t)count + 1;

    if (effective_index == -1) {
        name[0] = '\0';
        role_text[0] = '\0';
        for (size_t i = 0; i < NFLSTREET_LABEL_RATING_COUNT; ++i)
            ratings[i] = UINT32_MAX;
        *scalar = 1;
        return NFLSTREET_LABEL_OK;
    }
    if (effective_index < 0 || effective_index >= (int64_t)count)
        return NFLSTREET_LABEL_RANGE_ERROR;

    const nflstreet_team_label_row *row = &rows[(size_t)effective_index];
    if (!role_texts || row->pbpo >= role_text_count || !role_texts[row->pbpo])
        return NFLSTREET_LABEL_ROLE_INDEX_OUT_OF_RANGE;

    nflstreet_label_status status = nflstreet_team_label_name(
        row->first_name, row->last_name, name, name_capacity);
    if (status != NFLSTREET_LABEL_OK) return status;
    size_t role_length = strlen(role_texts[row->pbpo]);
    copy_limited(role_text, role_capacity, role_texts[row->pbpo], role_length);
    for (size_t i = 0; i < NFLSTREET_LABEL_RATING_COUNT; ++i)
        ratings[i] = row->ratings_by_original_ordinal[rating_ordinal[i]];
    *scalar = 1;
    return NFLSTREET_LABEL_OK;
}
