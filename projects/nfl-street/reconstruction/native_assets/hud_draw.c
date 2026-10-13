#include "hud_draw.h"
#include "scenery_shared.h"
#include <math.h>
#define rd32 nflstreet_rd32
#define rd16 nflstreet_rd16
#define GX NFLSTREET_GX
static uint8_t *at(nflstreet_draw *d,uint32_t token,uint32_t size){return nflstreet_draw_at(d,token,size);}
static int word(nflstreet_draw *d,uint32_t token,uint32_t *v){return nflstreet_draw_word(d,token,v);}
static int byte(nflstreet_draw *d,uint32_t token,uint8_t *v){const uint8_t *p=at(d,token,1);if(!p)return 0;*v=*p;return 1;}
/* 801AF57C without an error callback: the code is stored. */
static int set_error(nflstreet_draw *d,const nflstreet_hud_context *h,uint32_t code){
 uint8_t *e=at(d,h->error_code,4);if(h->error_hook||!e)return 0;nflstreet_wr32(e,code);return 1;}

#define K (&h->stacks)
static int stack_translate(nflstreet_draw *d,const nflstreet_hud_context *h,uint32_t vector){
 const uint8_t *v=at(d,vector,12);if(!v)return 0;
 return nflstreet_matrix_translate(d,K,nflstreet_as_float(rd32(v)),nflstreet_as_float(rd32(v+4)),nflstreet_as_float(rd32(v+8)));}

/* 80185A98: glyph record (12 bytes) for a code, by index then binary search. */
static int glyph(nflstreet_draw *d,uint32_t font,uint32_t code,uint32_t *out){
 uint32_t header,table;const uint8_t *p;if(!word(d,font+4,&header)||!word(d,font+8,&table)||!(p=at(d,header+0xa,2)))return 0;
 uint32_t n=rd16(p);*out=0;
 if(code-32u<n){const uint8_t *g=at(d,table+12u*(code-32u),2);if(!g)return 0;if(rd16(g)==code){*out=table+12u*(code-32u);return 1;}}
 while(n){uint32_t half=n>>1,e=table+12u*half;const uint8_t *g=at(d,e,2);if(!g)return 0;uint32_t v=rd16(g);
  if(code<v)n=half;else if(code>v){table=e+12;n-=half+1;}else{*out=e;return 1;}}
 return 1;}
/* 80185B18: kerning between two codes (signed byte), linear search. */
static int kerning(nflstreet_draw *d,uint32_t font,uint32_t a,uint32_t b,int32_t *out){
 uint32_t table,n;*out=0;if(!word(d,font+0xc,&table))return 0;if(!table)return 1;if(!word(d,table,&n))return 0;
 for(uint32_t e=table+4;(int32_t)n>0;--n,e+=8){const uint8_t *k=at(d,e,5);if(!k)return 0;
  if(rd16(k)==a&&rd16(k+2)==b){*out=(int8_t)k[4];return 1;}}
 return 1;}
/* 801863A4: glyph boxes {x, y, width, height, u16 u, u16 v} into the box
   array, lines broken at newline or (positive limit) when the pen passes
   it; centered (x/2) or right-aligned per line. */
static int layout(nflstreet_draw *d,const nflstreet_hud_context *h,uint32_t font,uint32_t text,uint32_t flags,float limit,uint32_t *count){
 uint8_t ready;*count=0;if(!byte(d,h->text_ready,&ready))return 0;if(!ready)return set_error(d,h,0x110002u);
 if(isnan(limit))return 0; /* the original never leaves its line loop */
 uint32_t header,length=0;uint8_t line;if(!word(d,font+4,&header)||!byte(d,header+0x13,&line))return 0;
 for(uint8_t c;length<=0x162u;++length){if(!byte(d,text+length,&c))return 0;if(!c)break;}
 if(length>0x162u)return 1; /* 8017CE3C length above 0x162: nothing, not even an error code */
 const int kern=!(flags&4);uint32_t used=0,i=0;int32_t y=0;
 while(i<length){uint32_t start=used;int32_t x=0;uint32_t previous=0xffffffffu;
  for(;i<length;++i){uint8_t c;if(!byte(d,text+i,&c))return 0;if(c==10){++i;break;}
   if(!(limit<=0.0f)&&!((float)x<=limit))break;
   uint32_t g;if(!glyph(d,font,c,&g))return 0;
   if(g){const uint8_t *r=at(d,g,10);uint8_t *b=at(d,h->text_boxes+0x14u*used,0x14);if(!r||!b)return 0;
    if(previous!=0xffffffffu&&kern){int32_t k;if(!kerning(d,font,previous,c,&k))return 0;x+=k;}
    nflstreet_wr32(b,(uint32_t)(x+(int8_t)r[9]));nflstreet_wr32(b+4,(uint32_t)(y+(int8_t)r[10]));nflstreet_wr32(b+8,r[2]);nflstreet_wr32(b+12,r[3]);
    memcpy(b+16,r+4,4);x+=(int8_t)r[8];++used;}
   previous=c;}
  int32_t shift=flags&1?x/2:flags&2?x:0;
  if(flags&3)for(uint32_t j=start;j<used;++j){uint8_t *b=at(d,h->text_boxes+0x14u*j,4);if(!b)return 0;nflstreet_wr32(b,rd32(b)-(uint32_t)shift);}
  y+=line;}
 *count=used;return set_error(d,h,0);}
/* 80186924 with 8018675C: one four-vertex quad per box, position (z 0),
   the color as bytes (x255, truncated) and texture coordinates over the
   atlas size. */
static int quads(nflstreet_draw *d,uint32_t font,uint32_t boxes,uint32_t count,uint32_t color){
 const uint8_t *c=at(d,color,16);uint32_t atlas;const uint8_t *a;if(!c||!word(d,font+0x10,&atlas)||!(a=at(d,atlas+4,4)))return 0;
 uint32_t rgba[4];for(unsigned k=0;k<4;++k)rgba[k]=(uint32_t)nflstreet_fctiwz(nflstreet_as_float(rd32(c+4*k))*255.0f)&0xffu;
 const float width=(float)(int32_t)rd16(a),height=(float)(int32_t)rd16(a+2);
 for(uint32_t i=0;i<count;++i){const uint8_t *b=at(d,boxes+0x14u*i,0x14);if(!b)return 0;
  int32_t x=(int32_t)rd32(b),y=(int32_t)rd32(b+4),w=(int32_t)rd32(b+8),hh=(int32_t)rd32(b+12);uint32_t u=rd16(b+16),v=rd16(b+18);
  const float x0=(float)x,x1=(float)(x+w),y0=(float)y,y1=(float)(y+hh),u0=(float)u/width,u1=(float)(int32_t)(u+(uint32_t)w)/width,
   v0=(float)v/height,v1=(float)(int32_t)(v+(uint32_t)hh)/height;
  const float corner[4][4]={{x0,y0,u0,v0},{x1,y0,u1,v0},{x1,y1,u1,v1},{x0,y1,u0,v1}};
  if(!GX(d,NFLSTREET_GX_BEGIN,0x80,0,4))return 0;
  for(unsigned k=0;k<4;++k){const float *q=corner[k];
   if(!GX(d,NFLSTREET_GX_WRITE,4,nflstreet_float_bits(q[0]))||!GX(d,NFLSTREET_GX_WRITE,4,nflstreet_float_bits(q[1]))||
      !GX(d,NFLSTREET_GX_WRITE,4,nflstreet_float_bits(0.0f)))return 0;
   for(unsigned j=0;j<4;++j)if(!GX(d,NFLSTREET_GX_WRITE,1,rgba[j]))return 0;
   if(!GX(d,NFLSTREET_GX_WRITE,4,nflstreet_float_bits(q[2]))||!GX(d,NFLSTREET_GX_WRITE,4,nflstreet_float_bits(q[3])))return 0;}}
 return 1;}
static int text(nflstreet_draw *d,const nflstreet_hud_context *h,uint32_t font,uint32_t string,uint32_t flags,float limit,uint32_t position){
 uint32_t count,header,font_flags,texture,state;
 if(!layout(d,h,font,string,flags,limit,&count))return 0;if(!count)return 1;
 if(!nflstreet_matrix_select(d,K)||!nflstreet_matrix_push(d,K)||!stack_translate(d,h,position)||!nflstreet_matrix_load_top(d,K))return 0;
 if(!word(d,font+4,&header)||!word(d,header+0xc,&font_flags)||!word(d,font+0x18,&texture)||!word(d,h->text_state,&state))return 0;
 const uint32_t shadow=h->text_state+0x14,main=h->text_state+4;
 if(font_flags&4){uint8_t *s=at(d,h->text_state,4);if(!s)return 0;nflstreet_wr32(s,state|0x40u);
  if(!GX(d,NFLSTREET_GX_LOAD_TLUT,h->tluts[0],0)||!GX(d,NFLSTREET_GX_LOAD_TEX_OBJ,texture,0)||!quads(d,font,h->text_boxes,count,shadow)||
     !GX(d,NFLSTREET_GX_LOAD_TLUT,h->tluts[1],0)||!GX(d,NFLSTREET_GX_LOAD_TEX_OBJ,texture,0))return 0;}
 else{if(!GX(d,NFLSTREET_GX_LOAD_TEX_OBJ,texture,0))return 0;
  if(state&0x10){if(!nflstreet_matrix_push(d,K)||!stack_translate(d,h,h->text_state+0x24)||!nflstreet_matrix_load_top(d,K)||
     !quads(d,font,h->text_boxes,count,shadow)||!nflstreet_matrix_pop(d,K)||!nflstreet_matrix_load_top(d,K))return 0;}}
 return quads(d,font,h->text_boxes,count,main)&&nflstreet_matrix_pop(d,K)&&set_error(d,h,0);}
static int valid(const nflstreet_hud_context *h,const nflstreet_render_state *s){
 return h&&h->scene&&h->scene->draw&&h->scene->draw->span&&h->scene->draw->emit&&s;}
int nflstreet_text_draw(const nflstreet_hud_context *h,uint32_t font,uint32_t string,uint32_t flags,float limit,uint32_t position,
 nflstreet_render_state *s){
 if(!valid(h,s))return 0;nflstreet_draw d={h->scene->draw,s};return text(&d,h,font,string,flags,limit,position);}

/* 80186264: default text state (white text; shadow black at half alpha,
   offset (3, 3, 1)). */
static int text_defaults(nflstreet_draw *d,const nflstreet_hud_context *h){
 uint8_t *t=at(d,h->text_state,0x30);if(!t)return 0;
 static const float v[11]={1,1,1,1,0,0,0,0.5f,3,3,1};nflstreet_wr32(t,0);
 for(unsigned i=0;i<11;++i)nflstreet_wr32(t+4+4*i,nflstreet_float_bits(v[i]));return 1;}
/* 80186CD8: direct position, color and texture coordinate; vertex color
   with texture alpha; source-alpha blending without depth; alpha > 0. */
static int text_setup(nflstreet_draw *d){
 return nflstreet_draw_gx(d,NFLSTREET_GX_CLEAR_VTX_DESC,0,NULL,NULL,0)&&GX(d,NFLSTREET_GX_SET_VTX_DESC,9,1)&&GX(d,NFLSTREET_GX_SET_VTX_DESC,0xb,1)&&
  GX(d,NFLSTREET_GX_SET_VTX_DESC,0xd,1)&&GX(d,NFLSTREET_GX_SET_NUM_CHANS,1)&&GX(d,NFLSTREET_GX_SET_CHAN_CTRL,4,0,0,1,1,2,1)&&
  GX(d,NFLSTREET_GX_SET_NUM_TEV_STAGES,1)&&GX(d,NFLSTREET_GX_SET_TEV_ORDER,0,0,0,4)&&GX(d,NFLSTREET_GX_SET_TEV_OP,0,0)&&
  GX(d,NFLSTREET_GX_SET_TEV_COLOR_IN,0,0xf,0xf,0xf,0xa)&&GX(d,NFLSTREET_GX_SET_TEV_COLOR_OP,0,0,0,0,1,0)&&GX(d,NFLSTREET_GX_SET_NUM_TEX_GENS,1)&&
  GX(d,NFLSTREET_GX_SET_TEX_COORD_GEN,0,0,4,0x3c,0,0x7d)&&GX(d,NFLSTREET_GX_SET_CULL_MODE,0)&&GX(d,NFLSTREET_GX_SET_VTX_ATTR_FMT,0,9,1,4,0)&&
  GX(d,NFLSTREET_GX_SET_VTX_ATTR_FMT,0,0xb,1,5,0)&&GX(d,NFLSTREET_GX_SET_VTX_ATTR_FMT,0,0xd,1,4,0)&&GX(d,NFLSTREET_GX_SET_BLEND_MODE,1,4,5,5)&&
  GX(d,NFLSTREET_GX_SET_Z_COMP_LOC,0)&&GX(d,NFLSTREET_GX_SET_Z_MODE,0,7,0)&&GX(d,NFLSTREET_GX_SET_ALPHA_COMPARE,4,0,0,4,0);}
/* 80187CB4/80187CC4: switch the profile bucket. */
static int profile(nflstreet_draw *d,const nflstreet_hud_context *h,uint32_t bucket){
 uint32_t next=h->profile_base+0x7cu*bucket,current,stamp;if(!word(d,h->profile_current,&current)||!word(d,h->profile_next,&stamp))return 0;
 if(current==next)return 1;
 if(current){uint8_t *p=at(d,current+0x40,4),*n=at(d,h->profile_next,4);if(!p||!n)return 0;nflstreet_wr32(p,stamp);nflstreet_wr32(n,stamp+0x10);}
 if(next){uint8_t *p=at(d,next+0x40,4);if(!p)return 0;if(rd32(p))nflstreet_wr32(p,0);}
 uint8_t *c=at(d,h->profile_current,4);if(!c)return 0;nflstreet_wr32(c,next);return 1;}
/* 8014E29C(0x12) through 8010B60C: the option word (index 0x12 is not
   one of the mode-6 overrides). */
static int option(nflstreet_draw *d,const nflstreet_hud_context *h,uint32_t *value){
 uint8_t override;uint32_t mode;if(!byte(d,h->option_override,&override))return 0;
 if(override){if(!word(d,h->option_mode,&mode))return 0;if(mode==8)return 0;}
 return word(d,h->option_table+0x12u*16u,value);}
int nflstreet_name_tag_draw(const nflstreet_hud_context *h,uint32_t tag,nflstreet_render_state *s){
 if(!valid(h,s)||!tag)return 0;
 nflstreet_draw d={h->scene->draw,s};const nflstreet_scenery_context *c=h->scene;
 uint8_t on,shown,black;uint32_t value,font,mode;int held,counting;
 if(!byte(&d,h->tag_enable,&on))return 0;if(!on)return 1;
 if(!option(&d,h,&value))return 0;if(!value)return 1;
 const uint8_t *t=at(&d,tag,0x24);if(!t)return 0;shown=t[0x1c];font=rd32(t+0x14);if(!shown||!font)return 1;
 if(!nflstreet_play_held(&d,c,&held))return 0;if(held)return 1;
 if(!nflstreet_play_mode(&d,c,&mode))return 0;if(mode==4)return 1;
 if(!nflstreet_play_mode(&d,c,&mode))return 0;if(mode==7)return 1;
 if(!nflstreet_play_counting(&d,c,&counting))return 0;if(counting)return 1;
 uint32_t fog;if(!profile(&d,h,0xd)||!nflstreet_scenery_fog_switch(&d,c,0,&fog)||!text_defaults(&d,h)||!byte(&d,h->tag_black,&black))return 0;
 uint8_t *state=at(&d,h->text_state,0x14);if(!state)return 0;
 if(black)for(unsigned k=0;k<4;++k)nflstreet_wr32(state+4+4*k,nflstreet_float_bits(k==3?1.0f:0.0f));
 nflstreet_wr32(state,rd32(state)&~0x20u);
 if(!text_setup(&d))return 0;
 /* The projection slot 1 is copied, given the tag depth and restored:
    no net change. */
 if(!nflstreet_matrix_select(&d,K)||!nflstreet_matrix_push(&d,K)||!nflstreet_matrix_translate(&d,K,h->tag_offset,0.0f,0.0f)||
    !text(&d,h,font,rd32(t+0x18),1,0.0f,tag+4)||!nflstreet_matrix_pop(&d,K))return 0;
 uint32_t unused;return nflstreet_scenery_fog_switch(&d,c,fog,&unused);}
int nflstreet_camera_view(const nflstreet_hud_context *h,uint32_t camera,nflstreet_render_state *s){
 if(!valid(h,s)||!camera)return 0;nflstreet_draw d={h->scene->draw,s};
 uint32_t data,entry,handler,before;const uint8_t *v;
 if(!word(&d,camera+0x40,&data)||!(v=at(&d,data,0x20)))return 0;
 const uint32_t type=h->camera_types+12u*v[1];
 if(!word(&d,type+8,&before)||!word(&d,type,&entry)||!word(&d,entry+8,&handler)||before||handler!=h->screen_camera)return 0;
 return nflstreet_matrix_select(&d,K)&&nflstreet_matrix_reset(&d,K)&&nflstreet_matrix_rotate(&d,K,0,rd32(v+0x14))&&
  nflstreet_matrix_rotate(&d,K,1,rd32(v+0x18))&&nflstreet_matrix_rotate(&d,K,2,rd32(v+0x1c))&&
  nflstreet_matrix_translate(&d,K,-nflstreet_as_float(rd32(v+4)),-nflstreet_as_float(rd32(v+8)),-nflstreet_as_float(rd32(v+12)));}
