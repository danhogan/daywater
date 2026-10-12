#ifndef NFLSTREET_INPUT_TRANSLATION_H
#define NFLSTREET_INPUT_TRANSLATION_H

#include <stdint.h>

/* Guarded GNNE69 0x8003DDEC projection for nonzero cardinal vectors. */
int nflstreet_project_cardinal_input(const uint8_t input[0x20],
                                    uint8_t actor[0x30]);

/* Nonzero XY directions, original normal-context projection only. */
int nflstreet_project_directional_input(const uint8_t input[0x20],uint8_t actor[0x30]);
#endif
