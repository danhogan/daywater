#ifndef NFLSTREET_SELECT_RINGS_H
#define NFLSTREET_SELECT_RINGS_H
/* The player-select rings' per-frame update on saved RAM: the original
   800E4A24 (called from the game-update step 8004EDFC, after the player
   marks 800DA6C0 and before 800E5520) with its pool animation 800E44E4,
   the pool allocation 800E4F34, the position copy 800E41A8 and the target
   lookup 800E3C48. The ring objects are 0x104 bytes apart in two nine-entry
   tables (8027BE8C, 8027BEB0), a five-entry animation pool (802AC020) and
   the controlled player's own ring (r13-71E0):
   - 80DDFCD0 (table entry 0), 80DE01E4 (pool entry 0) and 80DE02E8 are the
     three objects the scene draws (nflstreet_ring_draw, 800E3F7C).
   Each frame, for the nine tracked slots, the ring takes the position
   (+1A8, +1AC; negated when the side record r13-7078 has +39 = 1) of the
   gameplay actor it follows (+FC), so a ring shows where its player stood
   when the update ran. Not modeled (no evidence needed in the checked
   cases; the update returns 0 instead of reading them): null object or
   actor references where the original would read low memory. */
#include "draw_engine.h"
typedef struct {
 const nflstreet_player_draw_context *x; /* memory */
 uint32_t r13;                           /* 803A9A40 */
} nflstreet_rings_context;
/* 800E4A24. Returns 0 when RAM is unreachable; RAM may then be partly written. */
int nflstreet_rings_update(const nflstreet_rings_context *);
#endif
