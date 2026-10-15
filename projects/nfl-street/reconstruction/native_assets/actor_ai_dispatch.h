#ifndef NFLSTREET_ACTOR_AI_DISPATCH_H
#define NFLSTREET_ACTOR_AI_DISPATCH_H
#include "actor_ai.h"
#include "locomotion_predicates.h"
/* GNNE69 r0. Candidate until verify_actor_ai_dispatch.py passes. Policy and
   animation services are explicit: missing services fail, never idle/fallback.
   A service must implement the addressed ORIGINAL routine's effects on this
   same RAM. The production implementation must not use the oracle DLL. */
typedef struct {uint32_t pc,args[6];} nflstreet_ai_call;
typedef int (*nflstreet_ai_service)(void *,const nflstreet_ai_call *,uint32_t *result);
typedef struct {
 nflstreet_actor_ai ram;
 nflstreet_ai_service service;void *user;
 uint32_t scratch; /* caller-owned eight-byte target; disjoint from actors */
 int ok;uint32_t unsupported;
} nflstreet_ai_dispatch;
/* 8007E0B4; table index normally zero, commands is actor+BE4. controlled
   normally means actor+8 != FF; pass zero for non-controlled gameplay actors.
   Reads table *(*(r13-7860)+4*index), stride 24; does NOT choose a policy. */
int nflstreet_ai_dispatch_step(nflstreet_ai_dispatch *,uint32_t index,uint32_t commands,uint32_t actor,int controlled);
/* 8007DDE8 / 8007E650. command is the four-byte big-endian command VALUE,
   rather than an emulated pointer to a temporary stack argument. */
int nflstreet_ai_dispatch_enter(nflstreet_ai_dispatch *,uint32_t index,uint32_t commands,uint32_t actor);
int nflstreet_ai_dispatch_replace(nflstreet_ai_dispatch *,uint32_t index,uint32_t commands,uint32_t command,uint32_t actor);
/* 8008D988, 8008D654, 8008D548/8005335C, and 80021EF0. The state callback
   calls external 8009AD80 (out-of-bounds policy) and on transitions invokes
   the destination state's actual table callbacks. Interaction creation/removal
   remains a service, with native cone eligibility and +124/+128 lifecycle. */
int nflstreet_ai_follow(nflstreet_ai_dispatch *,uint32_t actor,uint32_t *result);
int nflstreet_ai_eligible(nflstreet_ai_dispatch *,uint32_t actor,uint32_t other,uint32_t *result);
int nflstreet_ai_face(nflstreet_ai_dispatch *,uint32_t actor,uint32_t heading,float weight,uint32_t mode,unsigned slot);
int nflstreet_ai_interact(nflstreet_ai_dispatch *,uint32_t actor,uint32_t other,uint32_t *result);
/* 800150E8: use the existing verified locomotion selector to commit +200
   into +210, with its original clip/event providers. Call AFTER AI, before
   movement. This wrapper does not cap strength or advance movement. */
int nflstreet_ai_commit(nflstreet_locomotion_context *,uint32_t actor);
/* 800F8214: run after selector commit and before movement. Normalizes
   transient request codes/timers and caps the actual strength as the code
   requires. Contact admission 8005DCF8 and reached 800F8088 are services. */
int nflstreet_ai_input_finish(nflstreet_ai_dispatch *,uint32_t actor);
/* All return 1 on supported success, 0 on missing RAM/service/arithmetic.
   Writes preceding failure remain: stop the update, do not advance movement.
   unsupported identifies the missing original entry point. */
#endif
