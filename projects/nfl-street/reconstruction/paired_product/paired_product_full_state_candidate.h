#ifndef NFL_STREET_PAIRED_PRODUCT_FULL_STATE_CANDIDATE_H
#define NFL_STREET_PAIRED_PRODUCT_FULL_STATE_CANDIDATE_H

#include "core/cpu.h"

/* Process a guarded normal-float domain; false leaves ctx unchanged. */
bool nflstreet_paired_product_cpu_candidate(CPUState* ctx);

#endif
