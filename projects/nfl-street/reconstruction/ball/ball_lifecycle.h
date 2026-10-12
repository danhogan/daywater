#ifndef NFLSTREET_BALL_LIFECYCLE_H
#define NFLSTREET_BALL_LIFECYCLE_H
#include <stddef.h>
#include <stdint.h>
/* GNNE69 r0 bounded 800C25FC. State numbers remain opaque.
   Supported exits: 0,1,2,3,5,6,8,9; supported entries: 0,1,2,3.
   elapsed_word is the uint32 modular difference produced by 801EE7F8;
   its upstream clock is supplied, not reconstructed here.
   Unsupported callbacks/truncated input reject without changing any bytes. */
int nflstreet_ball_transition(uint8_t *record,size_t bytes,uint32_t state,
                             uint32_t argument,uint32_t elapsed_word);
/* Original 800BE204 and 800BE264 forward caller state/argument registers.
   The first clears both component triples +54/+7C; the second only +7C.
   Both clear association +B4 AFTER exit/entry callbacks. */
int nflstreet_ball_release(uint8_t *record,size_t bytes,uint32_t state,
                          uint32_t argument,uint32_t elapsed_word,
                          unsigned clear_first_components);
#endif
