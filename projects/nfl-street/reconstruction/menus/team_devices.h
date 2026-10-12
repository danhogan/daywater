#ifndef NFLSTREET_TEAM_DEVICES_H
#define NFLSTREET_TEAM_DEVICES_H

#include <stddef.h>
#include <stdint.h>

#define NFLSTREET_TEAM_DEVICE_SLOTS 4u
#define NFLSTREET_TEAM_DEVICE_MAPPING_COUNT 10u
#define NFLSTREET_TEAM_DEVICE_MAX_ITERATOR_RECORDS 64u

/* Supplied record from the original device iterator. This API does not
   discover physical devices or assign meanings to port handles. */
typedef struct {
 uint32_t port_handle;
 uint32_t side;
} nflstreet_team_device_record;

/* Reproduces 8010C078's four initialized/updated side words. mapping is the
   ten-word table read by 80129AB8. Unmapped handles (UINT32_MAX) are ignored;
   any other mapping outside slots 0..3 is unsupported and fails closed. */
int nflstreet_team_devices_project(
 const nflstreet_team_device_record *records,size_t record_count,
 const uint32_t mapping[NFLSTREET_TEAM_DEVICE_MAPPING_COUNT],
 uint32_t side_words[NFLSTREET_TEAM_DEVICE_SLOTS]);

#endif
