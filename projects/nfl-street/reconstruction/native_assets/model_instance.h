#ifndef NFLSTREET_MODEL_INSTANCE_H
#define NFLSTREET_MODEL_INSTANCE_H
#include <stdint.h>
#define NFLSTREET_MODEL_INSTANCE_BYTES 240u
/* Original80162FC4/801EBF7C initialization then801EBFE0 model selection.
   Preserve bytes not owned by those routines. Tokens are supplied host identities. */
int nflstreet_model_instance_initialize(uint8_t instance[240],uint32_t model,
 uint32_t palette,uint32_t materials);
#endif
