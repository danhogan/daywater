#include "team_player_labels.h"

#include <stdio.h>
#include <string.h>

#define CHECK(condition, label) do { \
    if (!(condition)) { \
        fprintf(stderr, "team player label check failed: %s\n", label); \
        return 1; \
    } \
} while (0)

static void fixed_field(uint8_t *out, size_t capacity, const char *text) {
    memset(out, 0, capacity);
    size_t n = strlen(text);
    if (n > capacity) n = capacity;
    memcpy(out, text, n);
}

int main(void) {
    nflstreet_team_label_row rows[2] = {0};
    const char *role_table[] = {"fixture role zero", "fixture role one"};
    char name[32], role[32];
    uint32_t scalar = 0;
    uint32_t ratings[NFLSTREET_LABEL_RATING_COUNT];

    fixed_field(rows[0].first_name, sizeof rows[0].first_name, "Ava");
    fixed_field(rows[0].last_name, sizeof rows[0].last_name, "Stone");
    rows[0].pbpo = 1;
    for (unsigned i = 0; i < NFLSTREET_LABEL_RATING_COUNT; ++i)
        rows[0].ratings_by_original_ordinal[i] = (uint16_t)(100 + i);

    fixed_field(rows[1].first_name, sizeof rows[1].first_name, "NoLast");
    rows[1].pbpo = 0;

    CHECK(nflstreet_team_label_name(rows[0].first_name, rows[0].last_name,
                                    name, sizeof name) == NFLSTREET_LABEL_OK,
          "name helper status");
    CHECK(strcmp(name, "Ava Stone") == 0, "PFNA, separator, PLNA order");
    CHECK(nflstreet_team_label_name(rows[1].first_name, rows[1].last_name,
                                    name, sizeof name) == NFLSTREET_LABEL_OK,
          "empty last-name helper status");
    CHECK(strcmp(name, "NoLast") == 0, "no trailing separator for empty PLNA");
    memset(rows[1].last_name, 0, sizeof rows[1].last_name);
    fixed_field(rows[1].last_name, sizeof rows[1].last_name, "OnlyLast");
    memset(rows[1].first_name, 0, sizeof rows[1].first_name);
    CHECK(nflstreet_team_label_name(rows[1].first_name, rows[1].last_name,
                                    name, sizeof name) == NFLSTREET_LABEL_OK,
          "empty first-name helper status");
    CHECK(strcmp(name, "OnlyLast") == 0, "no leading separator for empty PFNA");
    CHECK(nflstreet_team_label_name(rows[0].first_name, rows[0].last_name,
                                    name, 6) == NFLSTREET_LABEL_OK,
          "capacity-limited name status");
    CHECK(strcmp(name, "Ava S") == 0, "name truncates and terminates at capacity");

    CHECK(nflstreet_team_label_build(0, 0, rows, 2, role_table, 2,
                                     name, sizeof name, role, sizeof role,
                                     &scalar, ratings) == NFLSTREET_LABEL_OK,
          "nonempty callback status");
    CHECK(strcmp(name, "Ava Stone") == 0, "nonempty callback name");
    CHECK(strcmp(role, "fixture role one") == 0, "supplied DOL role-text slot");
    CHECK(scalar == 1, "callback scalar");
    static const uint32_t expected[NFLSTREET_LABEL_RATING_COUNT] = {
        106, 104, 107, 100, 103, 102, 101, 105, 108, 109
    };
    CHECK(memcmp(ratings, expected, sizeof expected) == 0,
          "original rating ordinal order");

    memset(name, 'x', sizeof name); memset(role, 'x', sizeof role);
    scalar = 0;
    CHECK(nflstreet_team_label_build(1, 1, rows, 2, role_table, 2,
                                     name, sizeof name, role, sizeof role,
                                     &scalar, ratings) == NFLSTREET_LABEL_OK,
          "empty callback status");
    CHECK(name[0] == '\0' && role[0] == '\0', "empty callback strings");
    CHECK(scalar == 1, "empty callback scalar");
    for (unsigned i = 0; i < NFLSTREET_LABEL_RATING_COUNT; ++i)
        CHECK(ratings[i] == UINT32_MAX, "empty callback stats use 0xffffffff");

    rows[0].pbpo = 2;
    CHECK(nflstreet_team_label_build(0, 0, rows, 2, role_table, 2,
                                     name, sizeof name, role, sizeof role,
                                     &scalar, ratings) == NFLSTREET_LABEL_ROLE_INDEX_OUT_OF_RANGE,
          "out-of-range PBPO rejected");

    puts("team player labels checks passed: name formatting, empty branch, supplied role table, and rating order");
    return 0;
}
