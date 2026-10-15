#ifndef NFLSTREET_LIVE_CLIPS_H
#define NFLSTREET_LIVE_CLIPS_H
#include "channel_callbacks.h"
#include "clip_requests.h"
/* Live-only adapters and the bounded movement-selector handoff described in
   clip_requests.md. Candidate integration; no new oracle verification. */
typedef struct {
 nflstreet_clip_requests requests;
 nflstreet_actor_channels *channels;
 nflstreet_channel_callbacks *callbacks;
} nflstreet_live_clips;
void nflstreet_live_clips_init(nflstreet_live_clips *,nflstreet_actor_channels *,nflstreet_channel_callbacks *);
/* 1: selected/unchanged/suppressed; 0: memory/callback failure;
   -1: selector needs an unreconstructed gameplay predicate (no request). */
int nflstreet_live_clips_movement(nflstreet_live_clips *,uint32_t actor);
#endif
