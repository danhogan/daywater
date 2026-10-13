#ifndef NFLSTREET_GX_TEXOBJ_H
#define NFLSTREET_GX_TEXOBJ_H
/* The game's GX SDK texture-object encoders (memory only), reconstructed
   from its code: GXInitTexObj 80205754 (no mipmaps), GXInitTexObjCI
   802059C8, GXInitTexObjLOD 80205A10 and GXInitTlutObj 80205DF4. Objects
   are big-endian byte images. */
#include <stdint.h>
/* SDK constants (r13-4BF0 .. -4BCC, r13-6090 filter map). */
typedef struct {
 float lod_scale;        /* r13-4BF0: 16, LOD to fixed point */
 float bias_min,bias_cap,bias_max,bias_scale; /* r13-4BE0, -4BDC, -4BD8, -4BD4 */
 float lod_min,lod_max;  /* r13-4BD0, r13-4BCC */
 uint8_t filters[6];     /* r13-6090: GX filter to register encoding */
} nflstreet_gx_texobj_constants;
void nflstreet_gx_init_tex_obj(uint8_t obj[0x20],uint32_t image,uint16_t width,uint16_t height,uint32_t format,
 uint32_t wrap_s,uint32_t wrap_t);
void nflstreet_gx_init_tex_obj_ci(uint8_t obj[0x20],uint32_t image,uint16_t width,uint16_t height,uint32_t format,
 uint32_t wrap_s,uint32_t wrap_t,uint32_t tlut_name);
/* Returns 0 for a filter outside the map. */
int nflstreet_gx_init_tex_obj_lod(uint8_t obj[0x20],const nflstreet_gx_texobj_constants *,uint32_t min_filter,uint32_t mag_filter,
 float min_lod,float max_lod,float lod_bias,uint8_t bias_clamp,uint8_t edge_lod,uint32_t anisotropy);
void nflstreet_gx_init_tlut_obj(uint8_t obj[12],uint32_t lut,uint32_t format,uint16_t entries);
#endif
