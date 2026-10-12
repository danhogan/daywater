#ifndef NFLSTREET_MATCH_SETTINGS_H
#define NFLSTREET_MATCH_SETTINGS_H

#include <stdint.h>

#define NFLSTREET_MATCH_SETTINGS_WORDS 25u

typedef enum {
    NFLSTREET_SETTING_INITIAL,
    NFLSTREET_SETTING_DEPENDENT_DEFAULT,
    NFLSTREET_SETTING_SIDE_PAIR,
    NFLSTREET_SETTING_SESSION_INSERT
} nflstreet_match_settings_query;

typedef enum {
    NFLSTREET_SETTING_GTYG, NFLSTREET_SETTING_ISFG, NFLSTREET_SETTING_DOTG,
    NFLSTREET_SETTING_HTWG, NFLSTREET_SETTING_GPWG, NFLSTREET_SETTING_GTWG,
    NFLSTREET_SETTING_GFWG, NFLSTREET_SETTING_GWWG, NFLSTREET_SETTING_GTHG,
    NFLSTREET_SETTING_IPHG, NFLSTREET_SETTING_PDHG, NFLSTREET_SETTING_OTHG,
    NFLSTREET_SETTING_HCHG, NFLSTREET_SETTING_CSHG, NFLSTREET_SETTING_GTAG,
    NFLSTREET_SETTING_IPAG, NFLSTREET_SETTING_PDAG, NFLSTREET_SETTING_OTAG,
    NFLSTREET_SETTING_HCAG, NFLSTREET_SETTING_CSAG, NFLSTREET_SETTING_MITG,
    NFLSTREET_SETTING_RTQG, NFLSTREET_SETTING_ELQG, NFLSTREET_SETTING_WODG,
    NFLSTREET_SETTING_PNIG
} nflstreet_match_setting_field;

typedef int32_t (*nflstreet_match_settings_query_fn)(void *,
    nflstreet_match_settings_query query, uint32_t argument,
    uint32_t *values, uint32_t value_count, double float_argument);

typedef struct {
    /* Words stay in the exact field order of query 80266F44 / update 802671A0. */
    uint32_t query_values[NFLSTREET_MATCH_SETTINGS_WORDS];
    uint32_t normalized_values[NFLSTREET_MATCH_SETTINGS_WORDS];
    uint32_t side_context[2];
    /* Exact payload handed to the 801E577C/session owner. The query engine
       which ultimately inserts it remains external to this module. */
    uint32_t session_insert_values[NFLSTREET_MATCH_SETTINGS_WORDS];
    double session_insert_float_argument;
    uint32_t mode;
    unsigned dependent_default_requests;
    unsigned side_pair_requests;
    unsigned insert_descriptor_ready;
    int32_t status;
} nflstreet_match_settings_result;

/* Runs only original-owned query/normalization work. The query callback must
   preserve each exact query's output ordering and status. This function does
   not pretend to construct the game-owned session record. */
int nflstreet_match_settings_resolve(uint32_t mode,
    nflstreet_match_settings_query_fn query, void *context,
    nflstreet_match_settings_result *result);

#endif
