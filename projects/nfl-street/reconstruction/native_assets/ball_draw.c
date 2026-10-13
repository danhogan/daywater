#include "ball_draw.h"
#include "hud_shared.h"
#include <math.h>
#include "matrix_stack.h"
#include "scenery_shared.h"
#define rd32 nflstreet_rd32
#define GX NFLSTREET_GX
#define K (&c->hud->stacks)
static int word(nflstreet_draw *d,uint32_t token,uint32_t *v){return nflstreet_draw_word(d,token,v);}
static float f32(uint32_t v){return nflstreet_as_float(v);}
static int load_top(nflstreet_draw *d,const nflstreet_ball_context *c){return nflstreet_matrix_load_top(d,K)&&GX(d,NFLSTREET_GX_SET_CURRENT_MTX,0);}
/* 801ED4D4(0): light 0's direction on stack 0. */
static int light_direction(nflstreet_draw *d,const nflstreet_ball_context *c,float out[3]){
 uint8_t *current=nflstreet_draw_at(d,K->current,4);if(!current)return 0;nflstreet_wr32(current,K->table); /* 801893C8(0) */
 const float v[3]={c->axis[0],c->axis[0],c->axis[1]};float r[3];
 if(!nflstreet_matrix_push(d,K)||!nflstreet_matrix_load(d,K,c->lights)||!nflstreet_matrix_rotate_vector(d,K,v,r))return 0;
 for(unsigned k=0;k<3;++k)out[k]=v[k]-r[k];
 const float n=nflstreet_recip_sqrt(nflstreet_fmadds(out[2],out[2],nflstreet_fmadds(out[0],out[0],out[1]*out[1])),c->recip[0],c->recip[1],c->recip[2]);
 for(unsigned k=0;k<3;++k)out[k]*=n;
 return nflstreet_matrix_pop(d,K);}
/* 80164E78 */
static int shadow(nflstreet_draw *d,const nflstreet_ball_context *c,uint32_t entry,const float dir[2]){
 const uint8_t *e=nflstreet_draw_at(d,entry,0x14),*p;if(!e||!(p=nflstreet_draw_at(d,rd32(e+4),12)))return 0;
 const float s=f32(rd32(e+8)),dx=s*dir[0],dy=s*dir[1],x=f32(rd32(p)),y=f32(rd32(p+4)),z=f32(rd32(p+8)),lo=c->consts[3],hi=c->consts[4];
 const float v[4][5]={{x+dx,y+dy,z,lo,hi},{x-dy,y+dx,z,hi,hi},{x-dx,y-dy,z,hi,lo},{x+dy,y-dx,z,lo,lo}};
 if(!GX(d,NFLSTREET_GX_BEGIN,0xa0,0,4))return 0;
 for(unsigned i=0;i<4;++i){for(unsigned k=0;k<3;++k)if(!GX(d,NFLSTREET_GX_WRITE,4,nflstreet_float_bits(v[i][k])))return 0;
  static const uint8_t order[4]={0xc,0xe,0xd,0xf};for(unsigned k=0;k<4;++k)if(!GX(d,NFLSTREET_GX_WRITE,1,e[order[k]]))return 0;
  for(unsigned k=3;k<5;++k)if(!GX(d,NFLSTREET_GX_WRITE,4,nflstreet_float_bits(v[i][k])))return 0;}
 return 1;}
static int shadows(nflstreet_draw *d,const nflstreet_ball_context *c,uint32_t type,const float dir[2]){
 for(uint32_t i=0;;++i){const uint8_t *n=nflstreet_draw_at(d,c->count,2);uint32_t e;const uint8_t *x;if(!n)return 0;
  if(i>=nflstreet_rd16(n))return 1;if(!word(d,c->entries+4*i,&e)||!(x=nflstreet_draw_at(d,e,0x14)))return 0;
  if(x[0x10]&&rd32(x)==type&&!shadow(d,c,e,dir))return 0;}}
int nflstreet_ball_shadows_draw(const nflstreet_ball_context *c,nflstreet_render_state *s){
 if(!c||!c->hud||!c->hud->scene||!c->hud->scene->draw||!c->hud->scene->draw->span||!c->hud->scene->draw->emit||!s)return 0;
 nflstreet_draw dd={c->hud->scene->draw,s},*d=&dd;const uint8_t *on=nflstreet_draw_at(d,c->enabled,1);if(!on)return 0;if(!*on)return 1;
 uint32_t stamp;if(!nflstreet_hud_profile(d,c->hud,2)||!word(d,c->hud->profile_next,&stamp))return 0;
 nflstreet_draw_reset(d);
 if(!GX(d,NFLSTREET_GX_SET_CULL_MODE,0)||!GX(d,NFLSTREET_GX_SET_BLEND_MODE,1,4,5,5)||!GX(d,NFLSTREET_GX_SET_Z_COMP_LOC,0)||
    !GX(d,NFLSTREET_GX_SET_Z_MODE,0,3,0)||!nflstreet_draw_gx(d,NFLSTREET_GX_CLEAR_VTX_DESC,0,NULL,NULL,0)||!GX(d,NFLSTREET_GX_SET_VTX_DESC,9,1)||
    !GX(d,NFLSTREET_GX_SET_VTX_ATTR_FMT,0,9,1,4,0)||!GX(d,NFLSTREET_GX_SET_VTX_DESC,0xb,1)||!GX(d,NFLSTREET_GX_SET_VTX_ATTR_FMT,0,0xb,1,5,0)||
    !GX(d,NFLSTREET_GX_SET_NUM_CHANS,1)||!GX(d,NFLSTREET_GX_SET_CHAN_CTRL,4,0,1,1,0,0,2)||!GX(d,NFLSTREET_GX_SET_VTX_DESC,0xd,1)||
    !GX(d,NFLSTREET_GX_SET_VTX_ATTR_FMT,0,0xd,1,4,0)||!GX(d,NFLSTREET_GX_SET_TEV_OP,0,0)||!GX(d,NFLSTREET_GX_SET_NUM_TEX_GENS,1)||
    !GX(d,NFLSTREET_GX_SET_TEX_COORD_GEN,0,1,4,0x3c,0,0x7d)||!GX(d,NFLSTREET_GX_SET_NUM_TEV_STAGES,1)||!GX(d,NFLSTREET_GX_SET_TEV_ORDER,0,0,0,4)||
    !nflstreet_matrix_select(d,K)||!GX(d,NFLSTREET_GX_LOAD_TEX_OBJ,c->texture,0))return 0;
 float dir[3];
 if(!nflstreet_matrix_push(d,K)||!nflstreet_matrix_concat(d,K,c->lights,c->unit)||!light_direction(d,c,dir))return 0;
 dir[2]=c->consts[0];
 { /* 801DE2AC(0x200000) then 801DE0CC */
  float sn,cs;if(!nflstreet_sine(d,K,0x200000u,0,&sn)||!nflstreet_sine(d,K,0x200000u,1,&cs))return 0;
  const float x=dir[0],y=dir[1];dir[1]=nflstreet_fmadds(cs,y,sn*x);dir[0]=nflstreet_fmadds(cs,x,-(sn*y));
  const float n=nflstreet_recip_sqrt(nflstreet_fmadds(dir[0],dir[0],dir[1]*dir[1]),c->recip[0],c->recip[1],c->recip[2]);
  dir[0]*=n;dir[1]*=n;}
 if(!load_top(d,c)||!nflstreet_matrix_pop(d,K)||!shadows(d,c,0,dir))return 0;
 if(!nflstreet_matrix_push(d,K)||!nflstreet_matrix_scale(d,K,c->consts[1],c->consts[1],c->consts[0])||!load_top(d,c)||!nflstreet_matrix_pop(d,K))return 0;
 const float flat[2]={c->consts[2],c->consts[2]};if(!shadows(d,c,1,flat))return 0;
 const uint32_t grey=0x808080ffu;uint8_t *next=nflstreet_draw_at(d,c->hud->profile_next,4);if(!next)return 0;nflstreet_wr32(next,stamp);
 return GX(d,NFLSTREET_GX_SET_Z_MODE,1,3,1)&&nflstreet_draw_gx(d,NFLSTREET_GX_SET_CHAN_MAT_COLOR,1,(uint32_t[]){4},&grey,1);}

#undef K
#define K (&b->shadows->hud->stacks)
/* 801C6D4C(0, 0): the top as position and normal matrix 0. */
static int load_model(nflstreet_draw *d,const nflstreet_ball_model_context *b){
 uint32_t top,m[12],n[9];const uint8_t *t;if(!nflstreet_matrix_top(d,K,&top)||!(t=nflstreet_draw_at(d,top,0x30)))return 0;
 for(unsigned i=0;i<12;++i)m[i]=rd32(t+4*i);for(unsigned r=0;r<3;++r)for(unsigned k=0;k<3;++k)n[3*r+k]=m[4*r+k];
 return nflstreet_draw_gx(d,NFLSTREET_GX_LOAD_POS_MTX,1,(uint32_t[]){0},m,12)&&nflstreet_draw_gx(d,NFLSTREET_GX_LOAD_NRM_MTX,1,(uint32_t[]){0},n,9);}
static int copy16(nflstreet_draw *d,uint32_t to,uint32_t from){
 uint8_t *t=nflstreet_draw_at(d,to,0x40);const uint8_t *f=nflstreet_draw_at(d,from,0x40);if(!t||!f)return 0;memmove(t,f,0x40);return 1;}
/* 801EC298: clip codes of the box through view x m2 x m (bounds when asked). */
static int frustum(nflstreet_draw *d,const nflstreet_ball_model_context *b,uint32_t m,uint32_t m2,float lo[3],float hi[3],uint32_t *codes){
 const nflstreet_scenery_context *sc=b->shadows->hud->scene;const float *unit=b->shadows->unit;
 if(!nflstreet_matrix_push(d,K)||!nflstreet_matrix_load(d,K,b->slots+0xc0)||!copy16(d,sc->cull_projection,b->slots+0x80)||
    !nflstreet_matrix_push(d,K)||(m2&&!nflstreet_matrix_concat(d,K,m2,unit))||(m&&!nflstreet_matrix_concat(d,K,m,unit)))return 0;
 uint32_t top;if(!nflstreet_matrix_top(d,K,&top)||!copy16(d,sc->cull_modelview,top)||!nflstreet_matrix_pop(d,K))return 0;
 const uint8_t *P=nflstreet_draw_at(d,sc->cull_projection,0x40),*M=nflstreet_draw_at(d,sc->cull_modelview,0x30),*B=nflstreet_draw_at(d,b->box,0x20);
 if(!P||!M||!B)return 0;float proj[16],mm[12],box[8],pts[24];
 for(unsigned i=0;i<16;++i)proj[i]=f32(rd32(P+4*i));for(unsigned i=0;i<12;++i)mm[i]=f32(rd32(M+4*i));for(unsigned i=0;i<8;++i)box[i]=f32(rd32(B+4*i));
 nflstreet_box_corners(box,mm,pts);
 if(lo){for(unsigned k=0;k<3;++k){lo[k]=b->bounds_init[0];hi[k]=b->bounds_init[1];}*codes=nflstreet_clip_bounds(pts,8,proj,lo,hi);}
 else *codes=nflstreet_clip_codes(pts,8,proj);
 return nflstreet_matrix_pop(d,K);}
/* 801EB68C/801EB5E0/801EB6C4: the model view for a projected area. */
static int level(nflstreet_draw *d,const nflstreet_ball_model_context *b,const float *lo,const float *hi,uint32_t *view){
 uint32_t count,list,hook,target;int32_t i=0;
 if(!word(d,b->lod+0xc,&count)||!word(d,b->lod+0x10,&list)||!word(d,b->refresh_hook,&hook)||!word(d,hook+4,&target))return 0;
 if(target!=b->refresh_target){const float area=fabsf((hi[0]-lo[0])*(hi[1]-lo[1]));
  for(;i<(int32_t)count;++i){uint32_t used;const uint8_t *e=nflstreet_draw_at(d,list+0x50u*(uint32_t)i,0x14);if(!e)return 0;used=rd32(e+0x10);
   if(!used)continue;const float t=f32(rd32(e));if(i==0?area>t:!(area<=t))break;}
  if(i>0){uint32_t id;if(!word(d,list+0x50u*(uint32_t)i+0xc,&id))return 0;
   if(id==0xffffu)for(;;){if(--i<=0)break;if(!word(d,list+0x50u*(uint32_t)i+0xc,&id))return 0;if(id!=0xffffu)break;}}}
 if(i<0||i>=(int32_t)count)return 0;*view=list+0x50u*(uint32_t)i+0x1c;return 1;}
int nflstreet_ball_draw(const nflstreet_ball_model_context *b,nflstreet_render_state *s){
 if(!b||!b->shadows||!b->shadows->hud||!b->shadows->hud->scene||!b->shadows->hud->scene->draw||!b->shadows->hud->scene->draw->span||
    !b->shadows->hud->scene->draw->emit||!s)return 0;
 nflstreet_draw dd={b->shadows->hud->scene->draw,s},*d=&dd;uint8_t *o=nflstreet_draw_at(d,b->ball,0x2a4);if(!o)return 0;
 const uint32_t light0=b->shadows->lights;const float *unit=b->shadows->unit;
 nflstreet_draw_reset(d);
 float rgb[3]={b->k[0],b->k[0],b->k[0]};const uint32_t lc=rd32(o+0x294);
 if(lc){const uint8_t *l=nflstreet_draw_at(d,lc+0xc,12);if(!l)return 0;for(unsigned k=0;k<3;++k)rgb[k]=f32(rd32(l+4*k));}
 if(rd32(o+0x14)&0x20)for(unsigned k=0;k<3;++k)rgb[k]*=f32(rd32(o+0x298+4*k));
 const float alpha=f32(rd32(o+0x28c));const uint32_t color=0xffffff00u|((uint32_t)nflstreet_fctiwz(alpha*b->k[1])&0xffu);
 if(!nflstreet_draw_light_set(d,4,rgb[0],rgb[1],rgb[2]))return 0;const uint32_t mask=s->light_mask;
 if(!nflstreet_draw_gx(d,NFLSTREET_GX_SET_CHAN_MAT_COLOR,1,(uint32_t[]){4},&color,1)||!GX(d,NFLSTREET_GX_SET_CHAN_CTRL,0,1,0,0,mask,2,2)||
    !GX(d,NFLSTREET_GX_SET_CHAN_CTRL,2,0,0,0,mask,2,2))return 0;
 uint32_t shadow_codes=0x3f,ball_codes=0x3f;float lo[3]={0},hi[3]={0};
 if(!(rd32(o+0x14)&8)){if(!frustum(d,b,b->ball+0x20,light0,NULL,NULL,&shadow_codes)||!frustum(d,b,b->ball+0x20,0,lo,hi,&ball_codes))return 0;}
 else{uint32_t hook,target;if(!word(d,b->refresh_hook,&hook)||!word(d,hook+4,&target))return 0;
  if(target!=b->refresh_target)return 0; /* the area would come from an unset stack buffer */
  nflstreet_wr32(o+0x14,rd32(o+0x14)&~8u);}
 uint32_t view;if(!level(d,b,lo,hi,&view)||!nflstreet_draw_select_model(d,view))return 0;s->materials=b->ball+0x284;
 if(!(ball_codes&0x3f)){if(!nflstreet_matrix_push(d,K)||!nflstreet_matrix_concat(d,K,b->ball+0x20,unit)||!load_model(d,b)||
   !nflstreet_draw_blend(d,2)||!nflstreet_draw_all(d)||!nflstreet_draw_blend(d,1)||!nflstreet_matrix_pop(d,K))return 0;}
 if(!(rd32(o+0x14)&2)&&!(shadow_codes&0x3f)){
  const float depth=b->k[3]-(float)0*b->k[2];uint32_t shade=0x68; /* the compare of 1 with 4 always selects 0x68 */
  if(!nflstreet_matrix_push(d,K)||!nflstreet_matrix_concat(d,K,light0,unit)||!nflstreet_matrix_concat(d,K,b->ball+0x20,unit)||
     !load_model(d,b)||!GX(d,NFLSTREET_GX_SET_Z_MODE,1,1,1)||!nflstreet_draw_depth_range(d,depth,depth))return 0;
  if(f32(rd32(o+0x28c))==b->k[0]){if(!nflstreet_draw_blend(d,4))return 0;}
  else{if(!nflstreet_draw_blend(d,2))return 0;shade=(uint32_t)nflstreet_fctiwz(f32(rd32(o+0x28c))*b->k[4])&0xffu;}
  if(!nflstreet_draw_gx(d,NFLSTREET_GX_SET_CHAN_MAT_COLOR,1,(uint32_t[]){4},&shade,1)||!nflstreet_draw_chunk(d,3)||!nflstreet_matrix_pop(d,K))return 0;}
 return GX(d,NFLSTREET_GX_SET_Z_MODE,1,3,1)&&nflstreet_draw_depth_range(d,b->k[5],b->k[0])&&nflstreet_draw_blend(d,1);}
