#ifndef NFLSTREET_CONTROLLER_SELECT_H
#define NFLSTREET_CONTROLLER_SELECT_H
#include <stdint.h>
/* Owned state for the local branch of8011B87C. Profiles/names are supplied by
   the caller; this is not a profile database or a hardware port discovery API. */
typedef struct {
 uint32_t slots[4],sides[4],profile_count;
 int8_t mapping[4];
 char profiles[4][20],special[3][20];
} nflstreet_controller_select;
int nflstreet_controller_remap(nflstreet_controller_select*,const int8_t[4]);
int nflstreet_controller_set(nflstreet_controller_select*,const int8_t[4],uint32_t port,uint32_t value);
int nflstreet_controller_get(const nflstreet_controller_select*,uint32_t port,int32_t*);
/* Atomic bounded local profile cycling, including original duplicate rules.
   Invalid state or nonterminating source branches reject without mutation. */
int nflstreet_controller_cycle(nflstreet_controller_select*,uint32_t port,int32_t direction,char name[20]);
typedef struct {uint32_t function,argc,args[3];} nflstreet_controller_effect;
typedef struct {uint32_t choice,count;nflstreet_controller_effect effects[7];} nflstreet_controller_commit;
/* Original8011B728 commit, with logical controller handles supplied. Captures
   binding/profile/audio requests for a later native executor; performs no I/O. */
int nflstreet_controller_confirm(const nflstreet_controller_select*,const uint32_t handles[4],uint32_t choice,nflstreet_controller_commit*);
#endif
