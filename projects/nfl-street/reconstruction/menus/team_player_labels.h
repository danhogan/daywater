#ifndef NFLSTREET_TEAM_PLAYER_LABELS_H
#define NFLSTREET_TEAM_PLAYER_LABELS_H

#include <stddef.h>
#include <stdint.h>

#define NFLSTREET_LABEL_FIRST_BYTES 12u
#define NFLSTREET_LABEL_LAST_BYTES 14u
#define NFLSTREET_LABEL_RATING_COUNT 10u

typedef struct {
    /* Raw fixed-width PLAY fields consumed by 8011D408/8014E9A0. */
    uint8_t first_name[NFLSTREET_LABEL_FIRST_BYTES]; /* PFNA */
    uint8_t last_name[NFLSTREET_LABEL_LAST_BYTES];   /* PLNA */
    uint32_t pbpo;                                  /* raw role-table index */
    uint32_t ratings_by_original_ordinal[NFLSTREET_LABEL_RATING_COUNT];
} nflstreet_team_label_row;

typedef enum {
    NFLSTREET_LABEL_OK = 0,
    NFLSTREET_LABEL_INVALID_ARGUMENT = -1,
    NFLSTREET_LABEL_RANGE_ERROR = -2,
    NFLSTREET_LABEL_ROLE_INDEX_OUT_OF_RANGE = -3
} nflstreet_label_status;

/*
 * Builds the team-menu display name using the 8011D408/8014E9A0 order:
 * PFNA + one space + PLNA, with the separator omitted when PLNA is empty.
 * Output follows the original capacity-limited, NUL-terminated behavior.
 */
nflstreet_label_status nflstreet_team_label_name(
    const uint8_t first_name[NFLSTREET_LABEL_FIRST_BYTES],
    const uint8_t last_name[NFLSTREET_LABEL_LAST_BYTES],
    char *output, size_t output_capacity);

/*
 * Native data adapter for the 8011DBD0 label callback's resolved record.
 * `rows` must already be the PLAY rows for the requested side. `cursor + mode`
 * is adjusted once exactly as the original callback does: if >= count, subtract
 * count + 1; the resulting -1 is the empty-label sentinel. Other invalid
 * indices are rejected.
 *
 * `role_texts` is the caller's copy of the original DOL pointer table rooted at
 * 80280D58; no role strings are synthesized here. PBPO must index within that
 * supplied table. Nonempty 32-bit ratings are reordered by the original
 * ordinals; the verified empty branch fills each with UINT32_MAX.
 */
nflstreet_label_status nflstreet_team_label_build(
    int32_t cursor, int32_t mode,
    const nflstreet_team_label_row *rows, size_t count,
    const char *const *role_texts, size_t role_text_count,
    char *name, size_t name_capacity,
    char *role_text, size_t role_capacity,
    uint32_t *scalar,
    uint32_t ratings[NFLSTREET_LABEL_RATING_COUNT]);

#endif
