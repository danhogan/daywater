#ifndef NFLSTREET_STARTUP_H
#define NFLSTREET_STARTUP_H
#include "archive.h"
#include "../actor_target_input/asset_loader.h"
typedef struct {
    nflstreet_asset_store store;
    uint8_t *arena, *registry;
    unsigned count;
    unsigned entry_index[256];
    size_t bank_size[256];
} nflstreet_native_banks;
/* Discover direct context-one banks at TERF directory entry boundaries.
   This catalog is not the original match bank-selection policy. */
int nflstreet_native_banks_open(const nflstreet_archive *,nflstreet_native_banks *);
void nflstreet_native_banks_close(nflstreet_native_banks *);
#endif
