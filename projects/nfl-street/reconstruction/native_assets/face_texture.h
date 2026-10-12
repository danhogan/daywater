#ifndef NFLSTREET_FACE_TEXTURE_H
#define NFLSTREET_FACE_TEXTURE_H
#include "archive.h"
#include "texture.h"
typedef struct {int32_t index;uint8_t fallback;char key[40];} nflstreet_face_texture_plan;
/* Original named face lookup; a template miss remains index-1. */
int nflstreet_face_texture_plan_make(const nflstreet_archive *,const char *,nflstreet_face_texture_plan *);
/* Literal original800E2E48 24-byte record writes; preserves unrelated bytes. */
int nflstreet_texture_request(uint8_t *,size_t,int32_t,uint32_t palette,uint32_t force);
/* Original801C5610 supported MMAPv2 relocation; malformed spans/other versions
   reject. Repeat relocated calls are no-ops; a processed buffer cannot be
   retokenized. The caller retains its first relocation identity. */
int nflstreet_mmap_relocate(uint8_t *,size_t,uint32_t);
typedef struct {
 nflstreet_face_texture_plan plan;uint8_t *raw,*processed;size_t size;uint32_t token;
 nflstreet_texture inspection_plane;
} nflstreet_face_texture_resource;
/* Own raw+processed MMAP and decoded plane0; plane0 is inspection data, not
   original material/palette selection or an original GPU result. */
int nflstreet_face_texture_resource_open(const nflstreet_archive *,const char *,uint32_t,
 nflstreet_face_texture_resource *);
void nflstreet_face_texture_resource_close(nflstreet_face_texture_resource *);
#endif
