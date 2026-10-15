#ifndef NFLSTREET_CLIP_REQUESTS_H
#define NFLSTREET_CLIP_REQUESTS_H
#include "draw_engine.h"
/* Saved big-endian MEM1, GNNE69 r0 / DOL 46e3de854998a0f66c9be8c72a1164d22cd29eb4.
   Candidate reconstruction: compile checked; oracle execution is pending.
   Providers have the same boundary as actor_channels, plus the scan event
   callback r13-6730 (actor=key=0, scanning=1). They may consume operands
   with nflstreet_clip_request_operand. NULL events return 1; NULL groups
   return without effects. Actual gameplay group callbacks are NOT modeled.
   Set ok=0 from a provider to reject an unsupported callback. */
typedef struct nflstreet_clip_requests nflstreet_clip_requests;
typedef uint32_t (*nflstreet_clip_event)(void *,nflstreet_clip_requests *,uint32_t actor,uint32_t command,uint32_t key,int scanning);
typedef void (*nflstreet_clip_group)(void *,nflstreet_clip_requests *,uint32_t function,uint32_t resources,uint32_t key,uint32_t set,uint32_t channels,uint32_t actor,uint32_t mode);
struct nflstreet_clip_requests {
 nflstreet_draw_span span;void *memory;
 nflstreet_clip_event event;nflstreet_clip_group group;void *user;
 uint32_t stack_top; /* PPC r1 at API entry; 0 selects 80F46FC0 (oracle fixture).
                        Scan helpers publish their local stream address in r13-4F58. */
 int ok,scanning;uint32_t scan_address;uint8_t scan[0x30]; /* internal local stream */
};
/* Return 1 on supported memory accesses, 0 on rejection. Writes before rejection
   remain. Original 80177EF4 itself returns r3=0, NOT this API's status.
   Arguments after context follow original GPR order; f1 is explicit below. */
int nflstreet_clip_request_replace(nflstreet_clip_requests *,uint32_t set,uint32_t channels,uint32_t index,uint32_t bank,uint32_t clip,uint32_t key,uint32_t actor); /* 8017413C */
int nflstreet_clip_request_group(nflstreet_clip_requests *,float weight,uint32_t manager,uint32_t set,uint32_t channels,uint32_t key,uint32_t actor); /* 80177EF4 */
int nflstreet_clip_request_transition(nflstreet_clip_requests *,uint32_t manager,uint32_t set,uint32_t channels,uint32_t new_key,uint32_t old_key,uint32_t state,uint32_t actor); /* 80178780 */
/* 80173E78 / 80173FCC: direct clip start, also used by transition type 3.
   *index gets the original signed return (-1 when all channels are protected).
   Weight initializes both current/target; group row override=FFFF means derive
   from key (or FFFF for high-bit key). Clip flags come from program byte +2. */
int nflstreet_clip_request_start(nflstreet_clip_requests *,float weight,uint32_t set,uint32_t channels,uint32_t bank,uint32_t clip,uint32_t key,uint32_t actor,uint32_t row_key,int32_t *index);
int nflstreet_clip_request_operand(nflstreet_clip_requests *,uint32_t *value);
#endif
