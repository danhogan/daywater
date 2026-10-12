#include "team_player_labels_callback.h"

#include <stdio.h>
#include <string.h>

#define SCREEN_BASE 0x81100000u
#define STACK_BASE 0x817c0000u
#define SCREEN_SIZE 512u

static void put_be(uint8_t *p, uint32_t value) {
    p[0] = (uint8_t)(value >> 24); p[1] = (uint8_t)(value >> 16);
    p[2] = (uint8_t)(value >> 8); p[3] = (uint8_t)value;
}
static uint32_t get_be(const uint8_t *p) {
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 |
           (uint32_t)p[2] << 8 | p[3];
}
static int stack_byte(const uint32_t words[256], uint32_t address, uint8_t *value) {
    if (address < STACK_BASE || address - STACK_BASE >= 1024u) return 0;
    size_t offset = address - STACK_BASE;
    unsigned shift = 24u - (unsigned)(offset & 3u) * 8u;
    *value = (uint8_t)(words[offset / 4] >> shift); return 1;
}
static void fixed(uint8_t *dst, size_t size, const char *value) {
    memset(dst, 0, size); size_t n = strlen(value); if (n > size) n = size; memcpy(dst, value, n);
}
static void setup(nflstreet_team_player_labels_bridge *bridge, uint8_t *screen,
                  uint32_t words[256], nflstreet_ui_service_request *q,
                  uint32_t stats_dimensions) {
    memset(screen, 0xa5, SCREEN_SIZE); memset(words, 0xa5, 256 * sizeof *words);
    memset(q, 0, sizeof *q);
    q->kind = 1; q->property = 0x80000011u; q->argc = 6;
    q->args[0] = 0; q->args[1] = 0xdeadbeefu;
    q->args[2] = SCREEN_BASE + 32; q->args[3] = STACK_BASE + 64;
    q->args[4] = SCREEN_BASE + 180; q->args[5] = STACK_BASE + 100;
    put_be(screen + 32, 0); put_be(screen + 36, 10); put_be(screen + 40, SCREEN_BASE + 96);
    words[16] = 0; words[17] = 16; words[18] = STACK_BASE + 256;
    words[25] = stats_dimensions;
    if (stats_dimensions == 1) words[26] = NFLSTREET_LABEL_RATING_COUNT;
    else if (stats_dimensions == 2) { words[26] = 2; words[27] = 5; }
    bridge->cursor[0] = 0; bridge->mode[0] = 0;
}

static int callback_success_cases(void) {
    nflstreet_team_label_row rows[2] = {0};
    const char *role_texts[] = {"fixture role zero", "fixture role one"};
    nflstreet_team_player_labels_bridge bridge = {0};
    uint8_t screen[SCREEN_SIZE]; uint32_t words[256];
    nflstreet_ui_service_request q; uint32_t result = 0xabcdef01u;
    fixed(rows[0].first_name, sizeof rows[0].first_name, "Ava");
    fixed(rows[0].last_name, sizeof rows[0].last_name, "Stone");
    rows[0].pbpo = 1;
    for (unsigned i = 0; i < NFLSTREET_LABEL_RATING_COUNT; ++i)
        rows[0].ratings_by_original_ordinal[i] = 200u + i;
    fixed(rows[1].first_name, sizeof rows[1].first_name, "NoLast");
    rows[1].pbpo = 0;
    bridge.rows[0] = rows; bridge.count[0] = 2;
    bridge.role_texts = role_texts; bridge.role_text_count = 2;
    bridge.selected_team_id[0] = bridge.supplied_team_id[0] = 77;
    bridge.supplied_mask = 1;
    setup(&bridge, screen, words, &q, 1);
    if (!nflstreet_team_player_labels_callback_memory(&bridge, screen, sizeof screen,
            SCREEN_BASE, &q, &result, STACK_BASE, words)) return 0;
    if (memcmp(screen + 96, "Ava Stone\0", 10) != 0 ||
        memcmp(screen + 180, "\0", 1) != 0 || get_be(screen + 180) != 1) return 0;
    static const uint32_t expected[10] = {206, 204, 207, 200, 203, 202, 201, 205, 208, 209};
    for (unsigned i = 0; i < 10; ++i) if (words[27 + i] != expected[i]) return 0;
    static const char role_expected[] = "fixture role one";
    for (unsigned i = 0; i <= sizeof role_expected - 1; ++i) {
        uint8_t actual;
        if (!stack_byte(words, STACK_BASE + 256 + i, &actual) || actual != role_expected[i]) return 0;
    }

    bridge.cursor[0] = 1; bridge.mode[0] = 1; setup(&bridge, screen, words, &q, 1);
    bridge.cursor[0] = 1; bridge.mode[0] = 1;
    if (!nflstreet_team_player_labels_callback_memory(&bridge, screen, sizeof screen,
            SCREEN_BASE, &q, &result, STACK_BASE, words)) return 0;
    if (screen[96] != 0 || screen[180] != 0 || get_be(screen + 180) != 1) return 0;
    for (unsigned i = 0; i < 10; ++i) if (words[27 + i] != UINT32_MAX) return 0;

    setup(&bridge, screen, words, &q, 2);
    if (!nflstreet_team_player_labels_callback_memory(&bridge, screen, sizeof screen,
            SCREEN_BASE, &q, &result, STACK_BASE, words)) return 0;
    for (unsigned i = 0; i < 10; ++i) if (words[28 + i] != expected[i]) return 0;
    return 1;
}

static int rejection_is_atomic(void) {
    nflstreet_team_label_row row = {0}; const char *role_texts[] = {"fixture role"};
    nflstreet_team_player_labels_bridge bridge = {0}; uint8_t screen[SCREEN_SIZE], before[SCREEN_SIZE];
    uint32_t words[256], words_before[256], result = 0xabcdef01u;
    nflstreet_ui_service_request q;
    fixed(row.first_name, sizeof row.first_name, "First");
    fixed(row.last_name, sizeof row.last_name, "Last"); row.pbpo = 0;
    bridge.rows[0] = &row; bridge.count[0] = 1; bridge.role_texts = role_texts;
    bridge.role_text_count = 1; bridge.supplied_mask = 1;
    bridge.selected_team_id[0] = bridge.supplied_team_id[0] = 4;
    setup(&bridge, screen, words, &q, 1);
    bridge.selected_team_id[0] = 5;
    memcpy(before, screen, sizeof screen); memcpy(words_before, words, sizeof words);
    if (nflstreet_team_player_labels_callback_memory(&bridge, screen, sizeof screen,
            SCREEN_BASE, &q, &result, STACK_BASE, words) ||
        memcmp(screen, before, sizeof screen) || memcmp(words, words_before, sizeof words)) return 0;

    bridge.selected_team_id[0] = bridge.supplied_team_id[0]; row.pbpo = 1;
    if (nflstreet_team_player_labels_callback_memory(&bridge, screen, sizeof screen,
            SCREEN_BASE, &q, &result, STACK_BASE, words) ||
        memcmp(screen, before, sizeof screen) || memcmp(words, words_before, sizeof words)) return 0;

    row.pbpo = 0; put_be((uint8_t *)words + 25u * sizeof words[0], 9);
    memcpy(words_before, words, sizeof words);
    if (nflstreet_team_player_labels_callback_memory(&bridge, screen, sizeof screen,
            SCREEN_BASE, &q, &result, STACK_BASE, words) ||
        memcmp(screen, before, sizeof screen) || memcmp(words, words_before, sizeof words)) return 0;
    return 1;
}

int main(void) {
    if (!callback_success_cases()) {
        fputs("team label callback successful memory cases failed\n", stderr); return 1;
    }
    if (!rejection_is_atomic()) {
        fputs("team label callback ownership/validation guard failed\n", stderr); return 2;
    }
    puts("team player label Q11 memory bridge checks passed: mixed screen/stack outputs, NUL strings, scalar, 10 32-bit ratings with 1D/2D stats headers, empty sentinel and atomic ownership/PBPO guards");
    return 0;
}
