#ifndef NFLSTREET_PAIR_REQUESTS_H
#define NFLSTREET_PAIR_REQUESTS_H
#include <stddef.h>
#include <stdint.h>
/* 0x8005FA04 selects first type-1 and type-0 rows from an already selected
   original asset. Asset selection/position matching is not implemented. */
int nflstreet_pair_requests(const uint8_t *asset,size_t asset_size,
                            const uint8_t params[60],uint16_t choice,
                            uint8_t current[16],uint8_t incoming[16]);
#endif
