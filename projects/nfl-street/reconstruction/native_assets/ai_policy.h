#ifndef NFLSTREET_AI_POLICY_H
#define NFLSTREET_AI_POLICY_H
#include "actor_ai_dispatch.h"
/* GNNE69 r0, candidate until the source-pinned original-module gate passes.
   Instruction-ordered C; no DLL, runtime opcode decoder or emulator required.
   This context owns private call temporaries, never gameplay scratch in MEM1.
   Fixed stack tokens match the original-check harness, not a discovered field. */
#define NFLSTREET_AI_POLICY_STACK_LO 0x80f38000u
#define NFLSTREET_AI_POLICY_STACK_HI 0x80f47000u
typedef struct {
 nflstreet_actor_ai ram;
 uint8_t stack[NFLSTREET_AI_POLICY_STACK_HI-NFLSTREET_AI_POLICY_STACK_LO];
 uint32_t failure_pc,failure_address,unsupported;
 unsigned calls,entries[5];int ok,failure_budget_exhausted;
} nflstreet_ai_policy;
void nflstreet_ai_policy_init(nflstreet_ai_policy *,nflstreet_actor_ai);
/* Install as dispatch.service with dispatch.user pointing to the context.
   Missing native targets/instructions/spans return 0, retaining exact failure
   PC/address. No original-backed stand-in or idle fallback is installed. */
int nflstreet_ai_policy_service(void *,const nflstreet_ai_call *,uint32_t *result);
/* Same source-derived native call, with original f1 for clip/group requests.
   GPR arguments remain call.args[0..5], corresponding to original r3..r8. */
int nflstreet_ai_policy_call(nflstreet_ai_policy *,const nflstreet_ai_call *,double f1,uint32_t *result);
#endif
