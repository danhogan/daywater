#include "match_settings.h"
#include <stdint.h>
#include <string.h>

static int32_t issue(nflstreet_match_settings_query_fn query, void *context,
    nflstreet_match_settings_query kind, uint32_t argument,
    uint32_t *values, uint32_t count, double float_argument) {
    return query(context, kind, argument, values, count, float_argument);
}

static uint32_t *at(uint32_t words[NFLSTREET_MATCH_SETTINGS_WORDS],
    nflstreet_match_setting_field field) {
    return &words[(unsigned)field];
}

static void normalize(uint32_t v[NFLSTREET_MATCH_SETTINGS_WORDS]) {
    if (v[NFLSTREET_SETTING_GTYG] == 0x4479ff5cu)
        v[NFLSTREET_SETTING_GTYG] = 0x41200000u;
    if (v[NFLSTREET_SETTING_DOTG] == 2047u) v[NFLSTREET_SETTING_DOTG] = 780u;
    if (v[NFLSTREET_SETTING_HTWG] == 15u) v[NFLSTREET_SETTING_HTWG] = 0;
    if (v[NFLSTREET_SETTING_GPWG] == 127u) v[NFLSTREET_SETTING_GPWG] = 0;
    if (v[NFLSTREET_SETTING_GTWG] == 127u) v[NFLSTREET_SETTING_GTWG] = 78;
    if (v[NFLSTREET_SETTING_GFWG] == 127u) v[NFLSTREET_SETTING_GFWG] = 0;
    if (v[NFLSTREET_SETTING_GWWG] == UINT32_MAX) v[NFLSTREET_SETTING_GWWG] = 0x01680000u;
    if (v[NFLSTREET_SETTING_OTHG] == 4u) v[NFLSTREET_SETTING_OTHG] = 3;
    if (v[NFLSTREET_SETTING_OTAG] == 4u) v[NFLSTREET_SETTING_OTAG] = 3;
    if (v[NFLSTREET_SETTING_HCHG] == 3u) v[NFLSTREET_SETTING_HCHG] = 2;
    if (v[NFLSTREET_SETTING_HCAG] == 3u) v[NFLSTREET_SETTING_HCAG] = 2;
    if (v[NFLSTREET_SETTING_CSHG] == 255u) v[NFLSTREET_SETTING_CSHG] = 0;
    if (v[NFLSTREET_SETTING_CSAG] == 255u) v[NFLSTREET_SETTING_CSAG] = 0;
    if (v[NFLSTREET_SETTING_MITG] == UINT32_MAX) v[NFLSTREET_SETTING_MITG] = 0;
    if (v[NFLSTREET_SETTING_RTQG] == 15u) v[NFLSTREET_SETTING_RTQG] = 0;
    if (v[NFLSTREET_SETTING_WODG] == 15u) v[NFLSTREET_SETTING_WODG] = 0;
    if (v[NFLSTREET_SETTING_PNIG] == 255u) v[NFLSTREET_SETTING_PNIG] = 0;
}

int nflstreet_match_settings_resolve(uint32_t mode,
    nflstreet_match_settings_query_fn query, void *context,
    nflstreet_match_settings_result *result) {
    if (!query || !result) return 0;
    nflstreet_match_settings_result owned;
    memset(&owned, 0, sizeof owned);
    owned.mode = mode;
    owned.side_context[0] = 1023u;
    owned.side_context[1] = 1023u;
    owned.status = issue(query, context, NFLSTREET_SETTING_INITIAL, 0,
        owned.query_values, NFLSTREET_MATCH_SETTINGS_WORDS, 0.0);
    if (owned.status) goto done;
    memcpy(owned.normalized_values, owned.query_values,
        sizeof owned.normalized_values);
    owned.normalized_values[NFLSTREET_SETTING_ISFG] = mode;

    /* 801E51D4 enters the fresh-match defaults path only for mode zero.
       Nonzero modes skip these clamps and the dependent-default query;
       side-pair resolution and insertion still follow below. */
    if (mode == 0) {
        normalize(owned.normalized_values);
        if (owned.normalized_values[NFLSTREET_SETTING_RTQG] == 1023u) {
            uint32_t dependent_default = 0;
            ++owned.dependent_default_requests;
            owned.status = issue(query, context, NFLSTREET_SETTING_DEPENDENT_DEFAULT,
                0, &dependent_default, 1, 0.0);
            if (owned.status) goto done;
            owned.normalized_values[NFLSTREET_SETTING_RTQG] = dependent_default;
        }
    }

    /* Side pair defaults and the final insert are outside the mode-zero
       normalization block in 801E51D4. Each pair query only replaces fields
       that still carry the source sentinel. */
    for (unsigned side = 0; side != 2; ++side) {
        uint32_t *first = at(owned.normalized_values, side ?
            NFLSTREET_SETTING_IPAG : NFLSTREET_SETTING_IPHG);
        uint32_t *second = at(owned.normalized_values, side ?
            NFLSTREET_SETTING_PDAG : NFLSTREET_SETTING_PDHG);
        uint32_t first_missing = *first == 63u;
        uint32_t second_missing = *second == 63u;
        uint32_t team = owned.normalized_values[side ?
            NFLSTREET_SETTING_GTAG : NFLSTREET_SETTING_GTHG];
        if (first_missing || second_missing) {
            uint32_t pair[2] = {*first, *second};
            ++owned.side_pair_requests;
            owned.status = issue(query, context, NFLSTREET_SETTING_SIDE_PAIR,
                team, pair, 2, 0.0);
            if (owned.status) goto done;
            if (first_missing) *first = pair[0];
            if (second_missing) *second = pair[1];
        }
    }

    memcpy(owned.session_insert_values, owned.normalized_values,
        sizeof owned.session_insert_values);
    uint32_t float_bits = owned.session_insert_values[NFLSTREET_SETTING_GTYG];
    float float_value;
    memcpy(&float_value, &float_bits, sizeof float_value);
    owned.session_insert_float_argument = (double)float_value;
    owned.status = issue(query, context, NFLSTREET_SETTING_SESSION_INSERT,
        0, owned.session_insert_values, NFLSTREET_MATCH_SETTINGS_WORDS,
        owned.session_insert_float_argument);
    if (owned.status) goto done;
    owned.insert_descriptor_ready = 1;
    owned.side_context[0] = owned.normalized_values[NFLSTREET_SETTING_GTHG];
    owned.side_context[1] = owned.normalized_values[NFLSTREET_SETTING_GTAG];
done:
    *result = owned;
    return 1;
}
