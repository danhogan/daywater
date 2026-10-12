#ifndef NFLSTREET_LZH1_H
#define NFLSTREET_LZH1_H
#include <stddef.h>
#include <stdint.h>
/* DOL-derived codec 5 (LZH1). MSB bitstream, bounded output, Adler checksum.
   Returns success only after an end marker and the exact decoded size. */
int nflstreet_lzh1_decode(const uint8_t *,size_t,uint8_t *,size_t);
#endif
