#ifndef NFLSTREET_CONTACT_SELECTION_H
#define NFLSTREET_CONTACT_SELECTION_H
#include <stddef.h>
#include <stdint.h>
typedef struct {
    uint32_t identity;
    uint16_t choice, key;
    const uint8_t *asset;
    size_t size;
} nflstreet_contact_candidate;
int nflstreet_contact_parameters(uint32_t actor_identity,uint32_t incoming_identity,
                                 const uint8_t actor[0x14d4],const uint8_t incoming[0x14d4],
                                 const uint8_t descriptor[6],const uint8_t incoming_descriptor[6],
                                 const uint8_t manager[256],uint8_t params[60],unsigned *eligible);
/* Source tables and opaque identities are supplied data, never RAM readers.
   Supported scope: one incoming actor, ordinary descriptor eligibility,
   no optional positioning solver. Failure is transactional. */
int nflstreet_contact_select(const uint8_t params[60], const uint8_t actor[0x14d4],
                            const uint8_t incoming[0x14d4], const uint8_t motion[44],
                            float angular_scale, const nflstreet_contact_candidate *candidates,
                            size_t count, uint8_t rng[24], uint8_t generator[256],
                            uint32_t *identity, uint16_t *choice, unsigned *accepted);
int nflstreet_contact_complete(uint8_t actor[0x14d4],uint8_t incoming[0x14d4],
                              uint8_t manager[256],const uint8_t current_request[16],
                              const uint8_t incoming_request[16]);
#endif
