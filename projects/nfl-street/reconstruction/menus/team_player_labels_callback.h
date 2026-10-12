#ifndef NFLSTREET_TEAM_PLAYER_LABELS_CALLBACK_H
#define NFLSTREET_TEAM_PLAYER_LABELS_CALLBACK_H

#include "script.h"
#include "team_player_labels.h"

typedef struct {
    const nflstreet_team_label_row *rows[2];
    size_t count[2];
    int32_t cursor[2];
    int32_t mode[2];
    const char *const *role_texts;
    size_t role_text_count;
    uint32_t selected_team_id[2];
    uint32_t supplied_team_id[2];
    uint8_t supplied_mask;
} nflstreet_team_player_labels_bridge;

/*
 * Stack-aware native bridge for original callback property 0x80000011.
 * Q args: side, unused, name descriptor, role descriptor, scalar address,
 * stats descriptor. String descriptors contain (opaque, capacity, buffer) at
 * +0/+4/+8. The stats descriptor contains a dimension count at +0, followed
 * by that many dimension sizes; ten 32-bit output words begin at
 * +(dimensions+1)*4. String capacities reserve one extra
 * byte for NUL, matching the original helper's output buffer convention.
 *
 * Rows must be supplied in the selected side's roster order and their team ID
 * must match the selected team. Role strings must come from the original DOL
 * table rooted at 80280D58; the bridge does not translate or synthesize them.
 * The screen buffer and VM stack are separate ownership regions.
 */
int nflstreet_team_player_labels_callback_memory(
    void *context, uint8_t *screen, size_t screen_size, uint32_t screen_base,
    const nflstreet_ui_service_request *request, uint32_t *result,
    uint32_t stack_base, uint32_t stack_words[256]);

#endif
