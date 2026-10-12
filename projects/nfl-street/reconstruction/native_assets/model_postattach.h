#ifndef NFLSTREET_MODEL_POSTATTACH_H
#define NFLSTREET_MODEL_POSTATTACH_H
#include <stddef.h>
#include <stdint.h>

/* A caller-owned token range. The view, relocated model chunks,
   target skeleton and allocation storage all use the same token namespace. */
typedef struct {
    uint32_t token;
    uint8_t *bytes;
    size_t size;
} nflstreet_model_postattach_segment;

/* Original 801C9784(count, width) and 801C97C4(token) equivalents.
   Successful allocations provide count*width bytes and a unique token. */
typedef int (*nflstreet_model_postattach_allocate)(void *opaque,
    uint32_t count, uint32_t width, nflstreet_model_postattach_segment *out);
typedef void (*nflstreet_model_postattach_release)(void *opaque, uint32_t token);

/* Original 801C6B38(view, target_skeleton). The view is a 52-byte processed
   model view at view_token. Segments cover the view and all referenced chunks;
   allocator-returned ranges are also resolved. Invalid spans reject; failed
   later allocations retain earlier mutations. The no-Skel/no-Clst and
   already-cleared name-table paths succeed unchanged. */
int nflstreet_model_postattach_bind(uint32_t view_token, uint32_t target_token,
    const nflstreet_model_postattach_segment *segments, size_t segment_count,
    nflstreet_model_postattach_allocate allocate,
    nflstreet_model_postattach_release release, void *opaque);
#endif
