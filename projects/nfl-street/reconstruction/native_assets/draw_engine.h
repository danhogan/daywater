#ifndef NFLSTREET_DRAW_ENGINE_H
#define NFLSTREET_DRAW_ENGINE_H
/* Shared render-engine routines of the original 801C7xxx-801CDxxx model
   drawer, used by the player draw (80161E10) and the scenery instance draw
   (801EC8E4). Internal to the draw modules; the public types are in
   player_draw.h. */
#include "player_draw.h"
#include <string.h>
typedef struct {const nflstreet_player_draw_context *x;nflstreet_render_state *s;} nflstreet_draw;
static inline uint32_t nflstreet_rd32(const uint8_t *p){return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static inline uint16_t nflstreet_rd16(const uint8_t *p){return(uint16_t)((unsigned)p[0]<<8|p[1]);}
static inline void nflstreet_wr32(uint8_t *p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
static inline void nflstreet_wr16(uint8_t *p,uint32_t v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static inline float nflstreet_as_float(uint32_t v){float f;memcpy(&f,&v,4);return f;}
static inline uint32_t nflstreet_float_bits(float f){uint32_t v;memcpy(&v,&f,4);return v;}
uint8_t *nflstreet_draw_at(nflstreet_draw *,uint32_t token,uint32_t size);
int nflstreet_draw_word(nflstreet_draw *,uint32_t token,uint32_t *);
int nflstreet_draw_gx(nflstreet_draw *,nflstreet_gx_op,unsigned n,const uint32_t *args,const uint32_t *data,unsigned words);
#define NFLSTREET_GX(d,op,...) nflstreet_draw_gx(d,op,sizeof((uint32_t[]){__VA_ARGS__})/4u,(uint32_t[]){__VA_ARGS__},NULL,0)
float nflstreet_fmadds(float a,float c,float b);
int32_t nflstreet_fctiwz(float);
int nflstreet_draw_select_model(nflstreet_draw *,uint32_t view);              /* 801C7914 */
void nflstreet_draw_reset(nflstreet_draw *);                                  /* 801C7648 */
int nflstreet_draw_palette(nflstreet_draw *,uint32_t palette);                /* 801C79E4 */
int nflstreet_draw_blend(nflstreet_draw *,uint32_t preset);                   /* 801C7DC0 */
int nflstreet_draw_depth_range(nflstreet_draw *,float near_z,float far_z);    /* 801C7EAC */
int nflstreet_draw_shader(nflstreet_draw *,uint32_t shader);                  /* 801C7A3C */
int nflstreet_draw_texture(nflstreet_draw *,uint32_t map,uint32_t texture,uint32_t tlut); /* 801C7AD4 */
int nflstreet_draw_list(nflstreet_draw *,uint32_t list,uint32_t count);       /* 801C8460 */
int nflstreet_draw_all(nflstreet_draw *);                                     /* 801C84D4 */
int nflstreet_draw_variant(nflstreet_draw *,uint32_t groups,int32_t group,int32_t variant,uint32_t *entry); /* 801CD43C */
int nflstreet_draw_group(nflstreet_draw *,int32_t group,int32_t variant);     /* 801C8544 */
#endif
