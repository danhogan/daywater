#ifndef NFLSTREET_PROFILE_STORE_H
#define NFLSTREET_PROFILE_STORE_H
#include <stdint.h>
#include <stddef.h>
#define NFLSTREET_OPTION_COUNT 22
#define NFLSTREET_OPTION_SAVE_SIZE 196
/* GNNE69-r0 Options keys/defaults reconstructed from80285B34 and8014DFF0.
   File envelope is native DWNSOPT1, not the original card database format. */
extern const uint32_t nflstreet_option_keys[NFLSTREET_OPTION_COUNT];
void nflstreet_option_defaults(uint32_t values[NFLSTREET_OPTION_COUNT]);
int nflstreet_option_encode(const uint32_t values[NFLSTREET_OPTION_COUNT],uint8_t bytes[NFLSTREET_OPTION_SAVE_SIZE]);
int nflstreet_option_decode(const uint8_t*,size_t,uint32_t values[NFLSTREET_OPTION_COUNT]);
/* Windows implementation: load returns1, missing2, invalid/I/O failure0.
   Save uses a same-directory temporary, flush, then atomic replacement.
   Parent directory must already exist. No raw addresses or original assets. */
int nflstreet_option_load(const char*,uint32_t values[NFLSTREET_OPTION_COUNT]);
int nflstreet_option_save(const char*,const uint32_t values[NFLSTREET_OPTION_COUNT]);
#endif
