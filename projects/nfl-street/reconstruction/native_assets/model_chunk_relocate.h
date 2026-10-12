#ifndef NFLSTREET_MODEL_CHUNK_RELOCATE_H
#define NFLSTREET_MODEL_CHUNK_RELOCATE_H
#include <stddef.h>
#include <stdint.h>

/* Relocate one decoded TMdl section in place. token names byte zero in the
   caller's 32-bit resource namespace; it is never a host address. The whole
   decoded asset is supplied because section pointers can target other spans.
   Returns 1 on success, 0 on malformed/unsupported input. Traversal is checked
   before the writing pass; original malformed-input behavior is not claimed.
   Call once per section, while its words are still raw offsets. */
int nflstreet_model_chunk_relocate(uint8_t *asset, size_t size,
                                    uint32_t token, uint32_t tag,
                                    size_t section_offset, size_t section_size);
#endif
