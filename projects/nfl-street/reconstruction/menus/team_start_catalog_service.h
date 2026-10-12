#ifndef NFLSTREET_TEAM_START_CATALOG_SERVICE_H
#define NFLSTREET_TEAM_START_CATALOG_SERVICE_H

#include "moderngekko/module_abi.h"
#include "team_catalog.h"

#define NFLSTREET_START_CATALOG_MAX_ROWS 32u
#define NFLSTREET_START_CATALOG_MAX_FIELDS 48u
#define NFLSTREET_START_CATALOG_MAX_CONTEXTS 8u
#define NFLSTREET_START_CONTROLLER_ROWS 1u

typedef struct {
    uint32_t tag, value;
} nflstreet_start_catalog_field;

typedef struct {
    uint32_t id;
    unsigned field_count;
    nflstreet_start_catalog_field fields[NFLSTREET_START_CATALOG_MAX_FIELDS];
} nflstreet_start_catalog_row;

typedef struct {
    unsigned row_count;
    nflstreet_start_catalog_row rows[NFLSTREET_START_CATALOG_MAX_ROWS];
    struct {
        uint32_t address, table_tag, key_tag, cursor;
        uint32_t query_filter, query_options[4];
        unsigned row_count, opens, resets, advances, reads, closes, active;
    } contexts[NFLSTREET_START_CATALOG_MAX_CONTEXTS];
    uint32_t controller_port, controller_enabled;
    unsigned context_count, row_reads, absent_rumt_reads, controller_reads;
    unsigned empty_cursor_reads;
    unsigned profile_query_calls;
    uint32_t profile_query_args[4], profile_query_results[4];
    uint32_t next_handle;
    unsigned unsupported_queries;
    char last_error[512];
} nflstreet_start_catalog_service;

int nflstreet_start_catalog_load(nflstreet_start_catalog_service *, const char *directory);
int nflstreet_start_catalog_step(nflstreet_start_catalog_service *, CPUState *);
void nflstreet_team_manager_set_external_step_hook(int (*)(CPUState *));

#endif
