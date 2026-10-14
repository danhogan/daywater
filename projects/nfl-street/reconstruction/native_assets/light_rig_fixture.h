#ifndef NFLSTREET_LIGHT_RIG_FIXTURE_H
#define NFLSTREET_LIGHT_RIG_FIXTURE_H
/* The saved match's light update context. The including file defines R13,
   ram32 and ramf and includes venue_fixture.h (venue_stacks). */
#include "light_rig.h"
static inline nflstreet_light_rig_context light_rig_context(uint8_t *ram,const nflstreet_player_draw_context *x){
 nflstreet_light_rig_context c;memset(&c,0,sizeof c);c.x=x;c.stacks=venue_stacks(ram);
 c.rig=0x8034fd88u;c.shadows=0x8034fd8cu;c.shadow_colors=0x8034fe8cu;c.light_sets=0x8034fe9cu;c.shadow_limit=R13-0x6228u;c.sun=0x802a3030u;
 return c;}
#endif
