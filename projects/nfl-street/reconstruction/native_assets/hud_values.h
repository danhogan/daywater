#ifndef NFLSTREET_HUD_VALUES_H
#define NFLSTREET_HUD_VALUES_H
#include <stdint.h>
/* Original-layout MEM1 access. All constants and pointers are read from the
   supplied image. No game assets, captured values or host gameplay policy. */
typedef uint8_t *(*nflstreet_hud_values_span)(void *,uint32_t,uint32_t);
typedef struct {
 nflstreet_hud_values_span span;void *memory;uint32_t r13;
 uint32_t rejected_address; /* first inaccessible token / unsupported routine */
} nflstreet_hud_values;
/* 8005D96C -> 8005D768 / 80020944 / 8003A4A0 / 801784B8.
   Controlled actors accept neutral input only: 8003DCAC's nonneutral input
   side effects are not reconstructed here. Rejection happens before writes. */
int nflstreet_hud_turbo_update(nflstreet_hud_values *);
/* 8005D418, 80059920: original normalized float, including special endpoints.
   These return success separately from the floating-point result. */
int nflstreet_hud_turbo_fraction(nflstreet_hud_values *,uint32_t slot,float *);
int nflstreet_hud_gamebreaker_fraction(nflstreet_hud_values *,uint32_t side,float *);
/* 80119590: writes the four turbo floats to a UIS one-dimensional array's
   payload, in the original order [slot2,slot3,slot0,slot1]. Supply the payload,
   not the UIS array descriptor (the callback adds dimensions+1 first). */
int nflstreet_hud_turbo_publish(nflstreet_hud_values *,uint32_t payload);
/* 801D15B0: remove completed UIS timers, activate newly registered timers.
   The script interpreter / 801CF610 tick is not implemented by this API. */
int nflstreet_hud_timer_cleanup(nflstreet_hud_values *,uint32_t screen);
#endif
