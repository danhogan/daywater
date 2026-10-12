#ifndef NFLSTREET_MODEL_VIEW_H
#define NFLSTREET_MODEL_VIEW_H
#include <stddef.h>
#include <stdint.h>
#define NFLSTREET_MODEL_VIEW_BYTES 52u
/* Independently decoded, unrelocated TMdl. The token is caller-owned resource
   identity, not a host pointer. Raw directory words remain relative offsets. */
typedef struct { const uint8_t *bytes;size_t size;uint32_t token; } nflstreet_model_view_source;
typedef struct {
 uint32_t tag,encoded_word,resource_token;
 const uint8_t *bytes;size_t size,directory_index;
} nflstreet_model_view_chunk;
/* Source header/directory validation and bounded chunk views. Returns-1 for
   malformed/relocated input,0 for absent tag,1 for first nonempty matching tag.
   Failure/missing leave the output unchanged. Encoded words are never rebased. */
int nflstreet_model_view_find(const nflstreet_model_view_source *,uint32_t tag,nflstreet_model_view_chunk *);
/* Original801C6644 behavior: unknown found tags return found1 without writing;
   recognized found tags update one word. Returns0 only for invalid inputs;
   *found receives the original success0/1. */
int nflstreet_model_view_apply(uint8_t *,size_t,const nflstreet_model_view_source *,uint32_t tag,unsigned *found);
/* Original801C67E4 sequence: clears+2C/+30 then twelve ordered tag lookups.
   Missing fields preserve prior bytes, including untouched word+1C. Callers
   creating fresh views supply zeroed storage. Validation precedes all writes. */
int nflstreet_model_view_attach(uint8_t *,size_t,const nflstreet_model_view_source *);
#endif
