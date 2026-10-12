#ifndef NFLSTREET_FIELD_STAT_TEXT_H
#define NFLSTREET_FIELD_STAT_TEXT_H
#include <stdint.h>
/* Bounded 80129B24 signed integer formatting; value comes from the caller's
   statistic owner. INT32_MIN is excluded pending original edge verification. */
int nflstreet_field_stat_text(int32_t value, char output[33]);
#endif
