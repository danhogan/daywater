#ifndef NFLSTREET_PLAYER_CONSTRUCTOR_H
#define NFLSTREET_PLAYER_CONSTRUCTOR_H
#include "actor_model_init.h"
#include "actor_tail_init.h"
/* Borrowed original constructor input. The first six model tokens must agree
   with the semantic resource views. Appearance consumes bytes through+44. */
typedef struct {
    const uint8_t *bytes;
    size_t size;
    nflstreet_actor_model_descriptor models;
    nflstreet_actor_tail_defaults tail;
} nflstreet_player_constructor_input;
typedef int (*nflstreet_player_appearance_initializer)(void *,uint8_t *,size_t,
    const uint8_t *,size_t);
/* Owns800E1F7C model/tail orchestration and+135C write. Appearance is a required
   external owner; rejecting it returns0 without undoing earlier construction.
   The constructor does not clear actor storage: the source's first memset has
   length0. Allocation/factory header setup remains with the caller. */
int nflstreet_player_construct(uint8_t *,size_t,
    const nflstreet_player_constructor_input *,nflstreet_actor_model_allocator,
    void *,nflstreet_player_appearance_initializer,void *);
#endif
