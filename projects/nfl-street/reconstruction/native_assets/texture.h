#ifndef NFLSTREET_TEXTURE_H
#define NFLSTREET_TEXTURE_H
#include <stddef.h>
#include <stdint.h>
typedef struct { unsigned width, height; uint32_t *bgra; } nflstreet_texture;
/* Probable version-2 MMAP layout, GX tiled texture and palette encodings.
   open selects image/palette zero; open_image selects a base mip explicitly.
   Caller frees with close. Animated name records and mip chains are unsupported. */
int nflstreet_texture_open(const uint8_t *,size_t,nflstreet_texture *);
int nflstreet_texture_open_image(const uint8_t *,size_t,unsigned,unsigned,nflstreet_texture *);
/* Static one-frame name record. Outputs image and palette selectors.
   Original 12-byte name records and 16-byte fixed names; ambiguous names fail. */
int nflstreet_texture_find(const uint8_t *,size_t,const char *,unsigned *,unsigned *);
void nflstreet_texture_close(nflstreet_texture *);
int nflstreet_texture_bmp(const nflstreet_texture *,const char *);
#endif
