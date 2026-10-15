#ifndef NFLSTREET_LOCOMOTION_PREDICATES_H
#define NFLSTREET_LOCOMOTION_PREDICATES_H
#include "draw_engine.h"
/* GNNE69 r0, DOL 46e3de854998a0f66c9be8c72a1164d22cd29eb4. Operates on
   big-endian saved RAM, not the live runtime's host arrays. See the adjacent
   doc for the original-module check boundary and evidence status. */
typedef struct {
 uint32_t manager,set,channels,key,actor;float weight;
} nflstreet_locomotion_request;
typedef int (*nflstreet_locomotion_request_fn)(void *,const nflstreet_locomotion_request *);
/* 8004F214 can call 80076C48 / 80076E18. These are gameplay event services,
   not predicates or clip callbacks. Their internals are NOT reconstructed.
   The provider must preserve their effects, or explicitly reject the call. */
typedef int (*nflstreet_locomotion_effect_fn)(void *,uint32_t function,uint32_t actor,uint32_t mode);
typedef struct {
 nflstreet_draw_span span;void *memory;
 nflstreet_locomotion_request_fn request;
 nflstreet_locomotion_effect_fn effect;void *user;
 int ok;uint32_t unsupported;
} nflstreet_locomotion_context;
/* 800150E8 and its eleven table entries. Invokes request at the original
   80177EF4 boundary AFTER selector writes; no request for unchanged/blocked
   keys unless 80014750 asks to restart. A missing invoked provider fails.
   1 success, 0 memory/provider failure, -1 unsupported table entry. Earlier
   writes remain on failure. No host speed/heading based fallback. */
int nflstreet_locomotion_select(nflstreet_locomotion_context *,uint32_t actor);
/* Exact predicate entry points, also exercised independently by the check. */
int nflstreet_locomotion_gameplay(nflstreet_locomotion_context *,uint32_t actor,uint32_t *value); /* 8006510C */
int nflstreet_locomotion_suppression(nflstreet_locomotion_context *,uint32_t actor,uint32_t *value); /* 8004F214 */
#endif
