#include "effects_draw.h"
#include "scenery_shared.h"
#define rd32 nflstreet_rd32
#define GX NFLSTREET_GX
#define WRITE NFLSTREET_GX_WRITE
static float f32(const uint8_t *p){return nflstreet_as_float(rd32(p));}
static uint8_t *at(nflstreet_draw *d,uint32_t token,uint32_t size){return nflstreet_draw_at(d,token,size);}
static int put4(nflstreet_draw *d,float v){return GX(d,WRITE,4,nflstreet_float_bits(v));}
/* A color byte: the low byte of fctiwz(v * 255), as the original's stb. */
static int put1(nflstreet_draw *d,float v,float scale){return GX(d,WRITE,1,(uint32_t)nflstreet_fctiwz(v*scale)&0xffu);}
/* The top's 3x4 matrix as the original's 80189ED8 copies it. */
static const uint8_t *top3x4(nflstreet_draw *d,const nflstreet_matrix_stacks *k){
 uint32_t top;return nflstreet_matrix_top(d,k,&top)?at(d,top,0x30):NULL;}
/* 801C7A3C for the two effect shaders: the shader object {definition,
   setup +04, teardown +08}; the setup callbacks 80158580 (textured) and
   80158648 (flat) are these emitters' own. */
static int effect_shader(nflstreet_draw *d,uint32_t shader){
 nflstreet_render_state *s=d->s;if(!shader)return 0;
 if(shader!=s->shader){uint32_t teardown=0,def,unused,callback;s->override=0;
  if(s->shader&&(!nflstreet_draw_word(d,s->shader+8,&teardown)||teardown))return 0;
  if(!nflstreet_draw_word(d,shader,&def)||!nflstreet_draw_word(d,def+0x14,&unused)||!nflstreet_draw_word(d,shader+4,&callback))return 0;
  if(callback==0x80158580u){ /* textured: raster color times texture, one stage */
   if(!GX(d,NFLSTREET_GX_SET_NUM_CHANS,1)||!GX(d,NFLSTREET_GX_SET_CHAN_CTRL,4,0,0,1,0,0,2)||!GX(d,NFLSTREET_GX_SET_NUM_TEV_STAGES,1)||
      !GX(d,NFLSTREET_GX_SET_TEV_ORDER,0,0,0,4)||!GX(d,NFLSTREET_GX_SET_TEV_COLOR_IN,0,0xf,8,10,0xf)||
      !GX(d,NFLSTREET_GX_SET_TEV_COLOR_OP,0,0,0,0,1,0)||!GX(d,NFLSTREET_GX_SET_TEV_ALPHA_IN,0,7,4,5,7)||
      !GX(d,NFLSTREET_GX_SET_TEV_ALPHA_OP,0,0,0,0,1,0))return 0;}
  else if(callback==0x80158648u){ /* flat: raster color only */
   if(!GX(d,NFLSTREET_GX_SET_NUM_CHANS,1)||!GX(d,NFLSTREET_GX_SET_CHAN_CTRL,4,0,0,1,0,0,2)||!GX(d,NFLSTREET_GX_SET_NUM_TEV_STAGES,1)||
      !GX(d,NFLSTREET_GX_SET_TEV_ORDER,0,0xff,0xff,4)||!GX(d,NFLSTREET_GX_SET_TEV_OP,0,4))return 0;}
  else return 0;}
 s->shader=shader;return 1;}

/* ---- 801590F4: the four strips of one actor's strip object ---- */
static int strip_state(nflstreet_draw *d,const nflstreet_strips_context *c,const uint8_t *obj){
 const uint8_t *t=top3x4(d,c->stacks);if(!t)return 0;uint32_t m[12];for(unsigned i=0;i<12;++i)m[i]=rd32(t+4*i);
 nflstreet_draw_reset(d);
 return nflstreet_draw_gx(d,NFLSTREET_GX_CLEAR_VTX_DESC,0,NULL,NULL,0)&&GX(d,NFLSTREET_GX_SET_VTX_DESC,9,1)&&GX(d,NFLSTREET_GX_SET_VTX_DESC,0xb,1)&&
  GX(d,NFLSTREET_GX_SET_VTX_ATTR_FMT,0,9,1,4,0)&&GX(d,NFLSTREET_GX_SET_VTX_ATTR_FMT,0,0xb,1,5,0)&&GX(d,NFLSTREET_GX_SET_NUM_TEV_STAGES,1)&&
  GX(d,NFLSTREET_GX_SET_TEV_ORDER,0,0xff,0xff,4)&&GX(d,NFLSTREET_GX_SET_TEV_OP,0,4)&&GX(d,NFLSTREET_GX_SET_NUM_CHANS,1)&&
  GX(d,NFLSTREET_GX_SET_CHAN_CTRL,4,0,0,1,1,2,1)&&GX(d,NFLSTREET_GX_SET_NUM_TEX_GENS,1)&&GX(d,NFLSTREET_GX_SET_TEX_COORD_GEN,0,0,4,0x3c,0,0x7d)&&
  GX(d,NFLSTREET_GX_SET_CURRENT_MTX,0)&&nflstreet_draw_gx(d,NFLSTREET_GX_LOAD_POS_MTX,1,(uint32_t[]){0},m,12)&&
  nflstreet_draw_blend(d,obj[0x23c]?6:2)&&GX(d,NFLSTREET_GX_SET_ALPHA_COMPARE,7,0,0,7,0)&&GX(d,NFLSTREET_GX_SET_CULL_MODE,0);}
/* One strip (0x8C bytes at obj + 0x8C * i); rgb carries from strip to strip
   like the original's stack locals. */
static int strip(nflstreet_draw *d,const nflstreet_strips_context *c,uint32_t obj,uint32_t base,float rgb[3]){
 const float k=c->byte_scale;uint8_t *s=at(d,base,0x8c);const uint8_t *a=at(d,obj,0x250);if(!s||!a)return 0;
 const uint32_t count=rd32(s+0x7c);if(count<=1)return 1;
 const float alpha0=f32(a+0x24c);
 if(!GX(d,NFLSTREET_GX_BEGIN,0,0x98,((count&0xffffu)+2u)&0xffffu))return 0;
 for(unsigned e=0;e<2;++e){const uint32_t sel=rd32(s+0x80),edge=e?(sel==0)*12u:sel*12u;
  const uint8_t *p=at(d,base+0x60+edge,12);if(!p)return 0;
  for(unsigned i=0;i<3;++i)if(!put4(d,f32(p+4*i)))return 0;
  for(unsigned i=0;i<3;++i)if(!put1(d,rgb[i],k))return 0;
  if(!put1(d,alpha0,k))return 0;}
 for(uint32_t i=0;i<count;++i){
  float alpha;
  if(a[0x23c]){const uint8_t *h=at(d,c->history+0x10u*i,16);if(!h)return 0;for(unsigned q=0;q<3;++q)rgb[q]=f32(h+4*q);alpha=f32(h+12);}
  else alpha=alpha0;
  const float fade=(alpha-(float)i*(alpha/(float)count))*f32(a+0x230);
  const uint32_t head=rd32(s+0x78);const uint8_t *v=at(d,base+head*12u,12);if(!v)return 0;
  if(!put4(d,f32(v))||!put4(d,f32(v+4))||!put4(d,f32(v+8)))return 0;
  for(unsigned q=0;q<3;++q)if(!put1(d,rgb[q],k))return 0;
  if(!put1(d,fade,k))return 0;
  const int32_t next=(int32_t)(head+1u);const uint32_t whole=(uint32_t)(next<0?(int32_t)head+8:next)&~7u;
  nflstreet_wr32(s+0x78,(uint32_t)next-whole);}
 return 1;}
static int actor_strips(nflstreet_draw *d,const nflstreet_strips_context *c,uint32_t actor){
 const uint32_t obj=actor+0x1368u;const uint8_t *a=at(d,obj,0x250),*g=at(d,c->gate,1);float rgb[3];
 if(!a||!g)return 0;if(!*g)return 1; /* 800D1BC0 */
 if(!strip_state(d,c,a))return 0;
 for(unsigned i=0;i<3;++i)rgb[i]=f32(a+0x240+4*i);
 for(unsigned i=0;i<4;++i)if(!strip(d,c,obj,obj+0x8cu*i,rgb))return 0;
 return nflstreet_draw_gx(d,NFLSTREET_GX_CLEAR_VTX_DESC,0,NULL,NULL,0)&&nflstreet_draw_blend(d,1);}
int nflstreet_strips_draw(const nflstreet_strips_context *c,nflstreet_render_state *s){
 if(!c||!c->draw||!c->draw->span||!c->draw->emit||!c->stacks||!s)return 0;
 nflstreet_draw dd={c->draw,s},*d=&dd;
 for(uint32_t i=0;;++i){ /* 80041E08 each iteration */
  const uint8_t *n=at(d,c->actor_count,2);if(!n)return 0;if(i>=nflstreet_rd16(n))return 1;
  uint32_t actor,record,rflags,flags;
  if(!c->actors||!nflstreet_draw_word(d,c->actors+4*i,&actor)||!actor||!nflstreet_draw_word(d,actor+0x4a8,&record)||
     !nflstreet_draw_word(d,record+4,&rflags)||!nflstreet_draw_word(d,actor+0x14,&flags))return 0;
  if(rflags&1)continue;
  if(flags&8){const uint8_t *g=at(d,c->scaled_gate,1);if(!g||*g)return 0;} /* 800DFAF0's scaled form is not modeled */
  if((flags&0x10000u)&&!actor_strips(d,c,actor))return 0;}}

/* ---- 80158798: one emitter ---- */
/* 80197594: color and alpha of a particle (the gradient 80198D74 is not modeled). */
static int fade_color(const nflstreet_particles_context *c,const uint8_t *e,const uint8_t *p,float out[4]){
 if(e[0x3d]&&rd32(e+0x8c))return 0;
 out[0]=f32(e+0x7c);out[1]=f32(e+0x80);out[2]=f32(e+0x84);out[3]=f32(e+0x88);
 const float one=c->fade_one,in=f32(e+0xec),span=one-in;const int kind2=rd32(e+0x60)==2;
 if(e[0x42]){const float age=f32(p+0x48);
  if(!(age<in)){const float s=(one-age)/span;if(kind2){for(unsigned i=0;i<3;++i)out[i]*=s;}else out[3]=out[3]*s;}
  const float out_start=f32(e+0xe8);
  if(!(age>=out_start)){const float s=age/out_start;if(kind2){for(unsigned i=0;i<3;++i)out[i]*=s;}else out[3]=out[3]*s;}}
 return 1;}
/* 8017A7C8: the sprite sheet cell of a particle. */
static int sheet_cell(const nflstreet_particles_context *c,const uint8_t *t,uint32_t age,uint32_t frame,uint32_t rate,float out[8]){
 const uint32_t frames=nflstreet_rd16(t+0x4a);
 if(frames<2){static const unsigned pick[8]={0,0,1,0,1,1,0,1};for(unsigned i=0;i<8;++i)out[i]=c->cell[pick[i]];return 1;}
 if(t[0x48]&&rate){const uint32_t q=age/rate;frame+=q-(q/frames)*frames;}
 const uint32_t w=nflstreet_rd16(t+0x4e),rows=nflstreet_rd16(t+0x4c);if(!w||!rows)return 0;
 uint32_t row=frame/w;const uint32_t col=frame-row*w;if(row>=rows)row-=(row/rows)*rows;
 const float u=f32(t+0x70),v=f32(t+0x74),x0=(float)col,x1=(float)(col+1),y0=(float)row,y1=(float)(row+1);
 const float r[8]={u*x0,v*y0,u*x1,v*y0,u*x1,v*y1,u*x0,v*y1};for(unsigned i=0;i<8;++i)out[i]=r[i];return 1;}
/* 801DEBE8: a point through the top matrix, rows (m0 x + m1 y) + m2 z + m3. */
static int to_view(nflstreet_draw *d,const nflstreet_particles_context *c,const uint8_t *v,float out[3]){
 const uint8_t *t=top3x4(d,c->stacks);if(!t)return 0;const float x=f32(v),y=f32(v+4),z=f32(v+8);
 for(unsigned r=0;r<3;++r){const uint8_t *q=t+16*r;
  out[r]=nflstreet_fmadds(f32(q+8),z,nflstreet_fmadds(f32(q),x,f32(q+4)*y))+f32(q+12);}
 return 1;}
/* One particle's quad. */
static int particle(nflstreet_draw *d,const nflstreet_particles_context *c,const uint8_t *e,uint32_t particle_at,const uint8_t *rec,int textured){
 const uint8_t *p=at(d,particle_at,0x54);float pos[3],color[4],cells[8]={0};if(!p)return 0;
 const float k=c->byte_scale;
 if(textured){
  if(!to_view(d,c,p+0xc,pos)||!fade_color(c,e,p,color))return 0;
  const float unit=(float)e[0x35b]*c->color_unit;
  for(unsigned i=0;i<3;++i){const float x=color[i]*unit;color[i]=x<c->zero?c->zero:(x<=c->one?x:c->one);}
  if(!sheet_cell(c,rec,rd32(p+0x40),nflstreet_rd16(p+0x3a),nflstreet_rd16(e+0x48),cells))return 0;}
 else if(!fade_color(c,e,p,color)||!to_view(d,c,p+0xc,pos))return 0;
 const float size=f32(p+(e[0x41]?0x4c:0x30));const nflstreet_matrix_stacks *K=c->stacks;
 if(!nflstreet_matrix_push_identity(d,K)||!nflstreet_matrix_rotate(d,K,2,(uint32_t)nflstreet_fctiwz(f32(p+0x50)*c->angle_scale)))return 0;
 for(unsigned i=0;i<4;++i){const uint8_t *q=at(d,c->corners+12u*i,12);float v[3],r[3],w[3];if(!q)return 0;
  for(unsigned a=0;a<3;++a)v[a]=f32(q+4*a)*size;
  if(!nflstreet_matrix_rotate_vector(d,K,v,r))return 0;
  for(unsigned a=0;a<3;++a)w[a]=pos[a]+r[a];
  for(unsigned a=0;a<3;++a)if(!put4(d,w[a]))return 0;
  for(unsigned a=0;a<4;++a)if(!put1(d,color[a],k))return 0;
  if(textured&&(!put4(d,cells[2*i])||!put4(d,cells[2*i+1])))return 0;}
 return nflstreet_matrix_pop(d,K);}
static int emitter(nflstreet_draw *d,const nflstreet_particles_context *c,uint32_t emitter_at){
 const uint8_t *e=at(d,emitter_at,0x3c4);if(!e)return 0;
 const uint32_t n=rd32(e+0x3b8),rec_at=rd32(e+0x2ac);if(!e[0x2b4]||!n)return 1;
 const nflstreet_matrix_stacks *K=c->stacks;
 if(!nflstreet_matrix_push(d,K)||!GX(d,NFLSTREET_GX_SET_Z_MODE,1,3,0))return 0;
 switch(rd32(e+0x60)){ /* 80158508 */
 case 0:if(!nflstreet_draw_blend(d,2))return 0;break;
 case 2:if(!nflstreet_draw_blend(d,0)||!GX(d,NFLSTREET_GX_SET_BLEND_MODE,1,1,3,5))return 0;break;
 default:if(!nflstreet_draw_blend(d,6))return 0;}
 const int textured=e[0x45]&&rec_at;const uint8_t *rec=NULL;
 if(textured){if(!(rec=at(d,rec_at,0x78))||!GX(d,NFLSTREET_GX_LOAD_TLUT,rec_at+0x9c,0)||!GX(d,NFLSTREET_GX_LOAD_TEX_OBJ,rec_at+0x7c,0)||
   !effect_shader(d,c->shader_textured))return 0;}
 else if(!effect_shader(d,c->shader_flat))return 0;
 if(!nflstreet_draw_gx(d,NFLSTREET_GX_CLEAR_VTX_DESC,0,NULL,NULL,0)||!GX(d,NFLSTREET_GX_SET_VTX_DESC,9,1)||!GX(d,NFLSTREET_GX_SET_VTX_DESC,0xb,1)||
    (textured&&!GX(d,NFLSTREET_GX_SET_VTX_DESC,0xd,1))||!GX(d,NFLSTREET_GX_SET_VTX_ATTR_FMT,0,9,1,4,0)||!GX(d,NFLSTREET_GX_SET_VTX_ATTR_FMT,0,0xb,1,5,0)||
    (textured&&!GX(d,NFLSTREET_GX_SET_VTX_ATTR_FMT,0,0xd,1,4,0))||!GX(d,NFLSTREET_GX_BEGIN,0x80,0,(n<<2)&0xffffu))return 0;
 for(uint32_t i=0;(int32_t)i<(int32_t)n;++i){uint32_t pa;
  if(!nflstreet_draw_word(d,rd32(e+0x3c0)+4*i,&pa)||!particle(d,c,e,pa,rec,textured))return 0;}
 return nflstreet_matrix_pop(d,K);}
int nflstreet_particles_draw(const nflstreet_particles_context *c,nflstreet_render_state *s){
 if(!c||!c->draw||!c->draw->span||!c->draw->emit||!c->stacks||!s)return 0;
 nflstreet_draw dd={c->draw,s},*d=&dd;const nflstreet_matrix_stacks *K=c->stacks;
 const uint8_t *on=at(d,c->enable,1);if(!on)return 0;if(!*on)return 1;
 const uint8_t *count=at(d,c->count,2);if(!count)return 0;const uint32_t n=nflstreet_rd16(count);
 /* 80158D24: the top as position and normal matrix 0 (801C6D4C(0, 0)). */
 if(!nflstreet_matrix_push_identity(d,K))return 0;
 {const uint8_t *t=top3x4(d,K);uint32_t m[12],nr[9];if(!t)return 0;
  for(unsigned i=0;i<12;++i)m[i]=rd32(t+4*i);for(unsigned r=0;r<3;++r)for(unsigned q=0;q<3;++q)nr[3*r+q]=m[4*r+q];
  if(!nflstreet_draw_gx(d,NFLSTREET_GX_LOAD_POS_MTX,1,(uint32_t[]){0},m,12)||!nflstreet_draw_gx(d,NFLSTREET_GX_LOAD_NRM_MTX,1,(uint32_t[]){0},nr,9))return 0;}
 if(!nflstreet_matrix_pop(d,K))return 0;
 for(uint32_t i=0;i<n;++i){uint32_t e;if(!nflstreet_draw_word(d,c->list+4*i,&e)||!emitter(d,c,e))return 0;}
 const uint8_t *g72=at(d,c->gate_72fc,1);uint32_t g58;
 if(!g72||*g72==1||!nflstreet_draw_word(d,c->gate_58bc,&g58)||g58)return 0; /* 80158E00 and 80167874 are not modeled */
 return 1;}

/* ---- 801646CC: the actors' textured fans ---- */
static int fan(nflstreet_draw *d,const nflstreet_fans_context *c,uint32_t obj,uint32_t k){
 const uint8_t *rec=at(d,obj+0x60u+0x90u*k,0x90),*o=at(d,obj,0x10);if(!rec||!o)return 0;
 if(!GX(d,NFLSTREET_GX_BEGIN,0xa0,0,rec[0x8c]))return 0;
 float rgba[4];for(unsigned i=0;i<4;++i)rgba[i]=f32(o+4*i);
 for(int32_t j=0;j<(int32_t)rec[0x8c];++j){
  for(unsigned q=0;q<3;++q)if(!put4(d,f32(rec+12*j+4*q)))return 0;
  for(unsigned q=0;q<4;++q)if(!put1(d,rgba[q],c->byte_scale))return 0;
  if(!put4(d,f32(rec+0x54+8*j))||!put4(d,f32(rec+0x58+8*j)))return 0;}
 return 1;}
int nflstreet_fans_draw(const nflstreet_fans_context *c,nflstreet_render_state *s){
 if(!c||!c->scene||!c->scene->draw||!c->scene->draw->span||!c->scene->draw->emit||!c->stacks||!s)return 0;
 nflstreet_draw dd={c->scene->draw,s},*d=&dd;uint32_t previous;
 nflstreet_draw_reset(d);
 if(!nflstreet_scenery_fog_switch(d,c->scene,1,&previous))return 0;
 const uint8_t *t=top3x4(d,c->stacks);uint32_t m[12];
 if(!GX(d,NFLSTREET_GX_SET_CULL_MODE,0)||!GX(d,NFLSTREET_GX_SET_BLEND_MODE,1,4,5,5)||!GX(d,NFLSTREET_GX_SET_Z_COMP_LOC,0)||!GX(d,NFLSTREET_GX_SET_Z_MODE,1,3,0)||
    !nflstreet_draw_gx(d,NFLSTREET_GX_CLEAR_VTX_DESC,0,NULL,NULL,0)||!GX(d,NFLSTREET_GX_SET_VTX_DESC,9,1)||!GX(d,NFLSTREET_GX_SET_VTX_ATTR_FMT,0,9,1,4,0)||
    !GX(d,NFLSTREET_GX_SET_VTX_DESC,0xb,1)||!GX(d,NFLSTREET_GX_SET_VTX_ATTR_FMT,0,0xb,1,5,0)||!GX(d,NFLSTREET_GX_SET_NUM_CHANS,1)||
    !GX(d,NFLSTREET_GX_SET_CHAN_CTRL,4,0,1,1,0,0,2)||!GX(d,NFLSTREET_GX_SET_VTX_DESC,0xd,1)||!GX(d,NFLSTREET_GX_SET_VTX_ATTR_FMT,0,0xd,1,4,0)||
    !GX(d,NFLSTREET_GX_SET_TEV_OP,0,0)||!GX(d,NFLSTREET_GX_SET_NUM_TEX_GENS,1)||!GX(d,NFLSTREET_GX_SET_TEX_COORD_GEN,0,1,4,0x3c,0,0x7d)||
    !GX(d,NFLSTREET_GX_SET_NUM_TEV_STAGES,1)||!GX(d,NFLSTREET_GX_SET_TEV_ORDER,0,0,0,4)||
    !nflstreet_draw_texture(d,0,c->scene->shadow_texture,c->scene->shadow_tlut)||!t)return 0;
 for(unsigned i=0;i<12;++i)m[i]=rd32(t+4*i);
 if(!nflstreet_draw_gx(d,NFLSTREET_GX_LOAD_POS_MTX,1,(uint32_t[]){0},m,12)||!GX(d,NFLSTREET_GX_SET_CURRENT_MTX,0))return 0;
 for(uint32_t i=0;;++i){ /* 80041E08 each iteration */
  const uint8_t *n=at(d,c->scene->actor_count,2);if(!n)return 0;if(i>=nflstreet_rd16(n))break;
  uint32_t actor;if(!c->scene->actors||!nflstreet_draw_word(d,c->scene->actors+4*i,&actor)||!actor)return 0;
  const uint32_t obj=actor+0x109cu;const uint8_t *o=at(d,obj,0x2a2);if(!o)return 0;
  if(!o[0x2a1])continue;
  for(uint32_t k=0;k<o[0x2a0];k=(k+1)&0xffu)if(!fan(d,c,obj,k))return 0;}
 uint32_t unused;
 return nflstreet_draw_depth_range(d,c->depth[0],c->depth[1])&&GX(d,NFLSTREET_GX_SET_Z_MODE,1,3,1)&&nflstreet_scenery_fog_switch(d,c->scene,previous,&unused);}
