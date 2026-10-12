#ifndef NFL_STREET_ODD_POLYNOMIAL_FULL_STATE_CANDIDATE_H
#define NFL_STREET_ODD_POLYNOMIAL_FULL_STATE_CANDIDATE_H

#include "core/cpu.h"

/* Process a deliberately narrow normal-float domain; false leaves ctx alone. */
bool nflstreet_odd_polynomial_cpu_candidate(CPUState* ctx);

#endif
