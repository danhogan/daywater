#ifndef NFLSTREET_HUD_SHARED_H
#define NFLSTREET_HUD_SHARED_H
/* HUD text helpers shared with the uis screen draw (internal). */
#include "hud_draw.h"
#include "draw_engine.h"
/* 80186B1C with the position given as floats (the original passes a
   stack vector). */
int nflstreet_text_draw_at(nflstreet_draw *,const nflstreet_hud_context *,uint32_t font,uint32_t text,uint32_t flags,float limit,
 const float position[3]);
/* 80186CD8: the text GX setup. */
int nflstreet_text_setup(nflstreet_draw *);
/* 80187CB4/80187CC4: switch the profile bucket. */
int nflstreet_hud_profile(nflstreet_draw *,const nflstreet_hud_context *,uint32_t bucket);
/* 80186264: the default text state. */
int nflstreet_text_defaults(nflstreet_draw *,const nflstreet_hud_context *);
/* 801AF57C without an error callback. */
int nflstreet_hud_error(nflstreet_draw *,const nflstreet_hud_context *,uint32_t code);
#endif
