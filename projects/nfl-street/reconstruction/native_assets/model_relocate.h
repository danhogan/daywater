#ifndef NFLSTREET_MODEL_RELOCATE_H
#define NFLSTREET_MODEL_RELOCATE_H
#include <stddef.h>
#include <stdint.h>
typedef int (*nflstreet_model_chunk_handler)(uint8_t *,size_t,uint32_t identity,
    uint32_t tag,size_t offset,size_t size);
typedef struct {
    uint32_t tag, version;
    nflstreet_model_chunk_handler handler;
} nflstreet_model_chunk_registration;
/* Owns original TMdl directory dispatch/rebasing and processed flags. Caller
   provides the registry and exact chunk handlers. Namespace identities are
   stored tokens, never host pointers. Rejects invalid bounds without rollback
   of earlier processed chunks. A processed file is preserved unchanged. */
int nflstreet_model_relocate(uint8_t *,size_t,uint32_t identity,
    const nflstreet_model_chunk_registration *,size_t count);
/* Registration metadata from original801C6904. Lite and MAnm handlers are
   unsupported and reject processing; other handler coverage is limited to
   the asset/branch comparisons documented. */
const nflstreet_model_chunk_registration *nflstreet_model_default_registry(size_t *count);
#endif
