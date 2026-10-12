#ifndef NFLSTREET_FIELD_RENDER_H
#define NFLSTREET_FIELD_RENDER_H
#include "model.h"
#include "texture.h"
/* Bounded independent renderer for measured one-stage EARS draw states.
   The private profile supplies camera/order/state, not geometry or RAM.
   Does not implement original camera updates, mip LOD or complete GX pixels. */
typedef struct {
    uint32_t state[16];
    float camera[12],projection[6],viewport[6],texmatrix[12];
    /* Optional native actor textures. Zero for all existing NFR1/NFR2 draws. */
    const nflstreet_texture *actor_texture,*actor_overlay;
    uint32_t overlay_mode;
} nflstreet_field_draw;
typedef struct {unsigned commands,triangles,clipped;size_t shaded;} nflstreet_field_stats;
typedef struct {nflstreet_texture image;float *depth;} nflstreet_field_surface;
/* Cache a static field's color and depth; actor overlays preserve occlusion. */
int nflstreet_field_prepare(const nflstreet_mesh [6],const nflstreet_mesh *,const nflstreet_field_draw *,unsigned,
                           unsigned,unsigned,nflstreet_field_surface *,nflstreet_field_stats *);
int nflstreet_field_overlay(const nflstreet_field_surface *,const nflstreet_mesh *,const nflstreet_field_draw *,unsigned,
                           nflstreet_texture *,nflstreet_field_stats *);
/* Retain color and depth for the next independently posed actor. Outputs own
   allocations and must be distinct from the input surface. */
int nflstreet_field_layer(const nflstreet_field_surface *,const nflstreet_mesh *,const nflstreet_field_draw *,unsigned,
                         nflstreet_field_surface *,nflstreet_field_stats *);
void nflstreet_field_surface_close(nflstreet_field_surface *);
int nflstreet_field_render(const nflstreet_mesh [6],const nflstreet_field_draw *,unsigned,
                           unsigned,unsigned,nflstreet_texture *,nflstreet_field_stats *);
int nflstreet_field_render_sky(const nflstreet_mesh [6],const nflstreet_mesh *,const nflstreet_field_draw *,unsigned,
                               unsigned,unsigned,nflstreet_texture *,nflstreet_field_stats *);
int nflstreet_field_render_player(const nflstreet_mesh [6],const nflstreet_mesh *,const nflstreet_mesh *,
                                 const nflstreet_field_draw *,unsigned,unsigned,unsigned,
                                 nflstreet_texture *,nflstreet_field_stats *);
#endif
