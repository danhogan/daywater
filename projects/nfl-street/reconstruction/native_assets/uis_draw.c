#include "uis_draw.h"
#include "hud_shared.h"
#include "matrix_stack.h"
#include "scenery_shared.h"
#define rd32 nflstreet_rd32
#define rd16 nflstreet_rd16
#define GX NFLSTREET_GX
typedef struct {nflstreet_draw *d;const nflstreet_uis_context *u;const nflstreet_hud_context *h;} uis;
static uint8_t *at(const uis *x,uint32_t token,uint32_t size){return nflstreet_draw_at(x->d,token,size);}
static int word(const uis *x,uint32_t token,uint32_t *v){return nflstreet_draw_word(x->d,token,v);}
static int byte(const uis *x,uint32_t token,uint8_t *v){const uint8_t *p=at(x,token,1);if(!p)return 0;*v=*p;return 1;}
static int half(const uis *x,uint32_t token,int32_t *v){const uint8_t *p=at(x,token,2);if(!p)return 0;*v=(int16_t)rd16(p);return 1;}
static int put(const uis *x,uint32_t token,uint32_t v){uint8_t *p=at(x,token,4);if(!p)return 0;nflstreet_wr32(p,v);return 1;}
static int put16(const uis *x,uint32_t token,uint32_t v){uint8_t *p=at(x,token,2);if(!p)return 0;nflstreet_wr16(p,v);return 1;}
static float f32(uint32_t v){return nflstreet_as_float(v);}
static uint32_t bits(float f){return nflstreet_float_bits(f);}
#define K (&x->h->stacks)

/* Resource manager: archive a (0x24 bytes) and its entry e (8 bytes). */
static int archive(const uis *x,int32_t a,uint32_t *out){uint32_t list;if(!word(x,x->u->manager+0xc,&list))return 0;*out=list+0x24u*(uint32_t)a;return 1;}
static int entry(const uis *x,int32_t a,int32_t e,uint32_t *out){
 uint32_t ar,list;if(!archive(x,a,&ar)||!word(x,ar+0x20,&list))return 0;*out=list+8u*(uint32_t)e;return 1;}
/* 801A83B4 (bit 2) / 801A8464 (bit 0x10): an archive entry's state bit,
   with the resource lock 801AF944/801AF9CC as a supplied no-op. */
static int archive_state(const uis *x,uint32_t object,int32_t e,uint8_t bit,int *out){
 uint8_t up,state;uint32_t code=0,count_word,table;*out=0;if(!byte(x,x->u->resources_up,&up))return 0;
 if(!up)code=0xf0002u;else if(!object)code=0xf0005u;
 else{const uint8_t *c=at(x,object+0x14,2);if(!c)return 0;count_word=rd16(c);
  if((uint32_t)e>=count_word)code=0xf0006u;else{if(!word(x,object+0x78,&table)||!byte(x,table+(uint32_t)e,&state))return 0;*out=(state&bit)!=0;}}
 return nflstreet_hud_error(x->d,x->h,code);}
/* 801D7C1C: the palette for (a, b), from the one-entry cache only. */
static int palette(const uis *x,int32_t a,int32_t b,uint32_t *out){
 uint32_t cached,owner,ca,cb;*out=0;if(!x->u->manager)return 1;
 if(!word(x,x->u->palette_cache[0],&cached)||!word(x,x->u->palette_cache[1],&owner)||!word(x,x->u->palette_cache[2],&ca)||
    !word(x,x->u->palette_cache[3],&cb))return 0;
 if(!cached||owner!=x->u->manager||ca!=(uint32_t)a||cb!=(uint32_t)b)return 0; /* 801D54A4 acquisition: not modeled */
 *out=cached;return 1;}
static int remap(const uis *x,uint32_t pal,uint32_t *color){
 uint8_t n;if(!byte(x,pal,&n))return 0;uint32_t i=*color>>24;if(i>n)i=(uint32_t)(uint8_t)(n-1u);
 return word(x,pal+4+4u*i,color);}

/* Color stack (801D7E74 push, 801D81E8 pop): entries of scale RGBA, add
   RGBA and the eight clamped bytes. */
static uint8_t clamp_byte(const uis *x,float v){
 v*=x->u->color_limits[2];if(v<x->u->color_limits[1])return 0;if(!(v<=x->u->color_limits[2]))v=x->u->color_limits[2];
 return (uint8_t)nflstreet_fctiwz(v);}
static int color_push(const uis *x,uint32_t scale,uint32_t add){
 uint32_t depth,cur;if(!word(x,x->u->color_depth,&depth)||!word(x,x->u->color_current,&cur))return 0;
 const uint8_t *o=at(x,cur,0x28),*s=at(x,scale,16),*a=at(x,add,16);uint8_t *n=at(x,cur+0x28,0x28);if(!o||!s||!a||!n)return 0;
 float v[8];for(unsigned k=0;k<4;++k){v[k]=f32(rd32(o+4*k))*f32(rd32(s+4*k));v[4+k]=f32(rd32(o+0x10+4*k))*f32(rd32(a+4*k));}
 if(!(v[3]<x->u->color_limits[0]))v[3]=x->u->color_limits[0];
 if(!(v[3]>x->u->color_limits[1]))v[3]=x->u->color_limits[1];
 for(unsigned k=0;k<8;++k)nflstreet_wr32(n+4*k,bits(v[k]));
 for(unsigned k=0;k<8;++k)n[0x20+k]=clamp_byte(x,v[k]);
 return put(x,x->u->color_depth,depth+1)&&put(x,x->u->color_current,cur+0x28)&&put(x,x->u->draw_mode,0);}
static int color_pop(const uis *x){
 uint32_t depth,cur;return word(x,x->u->color_depth,&depth)&&word(x,x->u->color_current,&cur)&&put(x,x->u->color_depth,depth-1)&&
  put(x,x->u->color_current,cur-0x28)&&put(x,x->u->draw_mode,0);}
/* 801D8268: the node transform and color push; 801D8364: the pop. */
static int transform_push(const uis *x,uint32_t t){
 const uint8_t *v=at(x,t,0x54);if(!v)return 0;float f[12];for(unsigned i=0;i<12;++i)f[i]=f32(rd32(v+4*i));
 if(!nflstreet_matrix_select(x->d,K)||!nflstreet_matrix_push(x->d,K)||!nflstreet_matrix_translate(x->d,K,f[0],f[1],f[2])||
    !nflstreet_matrix_translate(x->d,K,f[3],f[4],f[5]))return 0;
 if(!(f[6]==x->u->zero&&f[7]==x->u->zero&&f[8]==x->u->zero))
  for(unsigned axis=0;axis<3;++axis)if(!nflstreet_matrix_rotate(x->d,K,axis,(uint32_t)nflstreet_fctiwz(f[6+axis]*x->u->degrees)))return 0;
 return nflstreet_matrix_scale(x->d,K,f[9],f[10],f[11])&&nflstreet_matrix_translate(x->d,K,-f[3],-f[4],-f[5])&&
  color_push(x,t+0x34,t+0x44)&&put(x,x->u->draw_mode,0);}
static int transform_pop(const uis *x){
 return nflstreet_matrix_select(x->d,K)&&nflstreet_matrix_pop(x->d,K)&&color_pop(x)&&put(x,x->u->draw_mode,0);}

/* 801D7AD8, 801D7A58, 801D8980, 801D89D0: the image GX state. */
static int image_state(const uis *x,uint32_t flags){
 uint32_t cur,mat,amb,mc,ac;
 if(!nflstreet_draw_gx(x->d,NFLSTREET_GX_CLEAR_VTX_DESC,0,NULL,NULL,0)||!GX(x->d,NFLSTREET_GX_SET_VTX_DESC,9,1)||
    !GX(x->d,NFLSTREET_GX_SET_VTX_ATTR_FMT,0,9,1,3,4)||!GX(x->d,NFLSTREET_GX_SET_VTX_ATTR_FMT,0,0xb,1,5,0)||
    !GX(x->d,NFLSTREET_GX_SET_VTX_ATTR_FMT,0,0xd,1,3,8)||!GX(x->d,NFLSTREET_GX_SET_BLEND_MODE,1,4,5,5)||
    !GX(x->d,NFLSTREET_GX_SET_Z_COMP_LOC,0)||!GX(x->d,NFLSTREET_GX_SET_Z_MODE,0,7,0)||!GX(x->d,NFLSTREET_GX_SET_ALPHA_COMPARE,4,0,0,4,0)||
    !put(x,x->u->color_cache[1],0x12345678u)||!put(x,x->u->color_cache[0],0x12345678u)||!put(x,x->u->channel_cache,0xffffffffu))return 0;
 if(!word(x,x->u->color_current,&cur)||!word(x,cur+0x20,&mat)||!word(x,cur+0x24,&amb)||!word(x,x->u->color_cache[0],&mc)||
    !word(x,x->u->color_cache[1],&ac))return 0;
 if(mat!=mc||amb!=ac){
  uint32_t light[16];const uint8_t *l;
  if(!nflstreet_draw_gx(x->d,NFLSTREET_GX_SET_CHAN_MAT_COLOR,1,(uint32_t[]){4},&mat,1)||!put(x,x->u->light+0xc,amb)||!(l=at(x,x->u->light,64)))return 0;
  for(unsigned i=0;i<16;++i)light[i]=rd32(l+4*i);
  if(!nflstreet_draw_gx(x->d,NFLSTREET_GX_LOAD_LIGHT,1,(uint32_t[]){0x80},light,16)||!put(x,x->u->color_cache[0],mat)||!put(x,x->u->color_cache[1],amb))return 0;}
 const uint32_t mode=flags&3;
 if(!GX(x->d,NFLSTREET_GX_SET_NUM_CHANS,1)||!GX(x->d,NFLSTREET_GX_SET_VTX_DESC,0xb,1)||!GX(x->d,NFLSTREET_GX_SET_CHAN_CTRL,4,1,1,0,0x80,0,2))return 0;
 if(flags&2){if(!GX(x->d,NFLSTREET_GX_SET_NUM_TEV_STAGES,1)||!GX(x->d,NFLSTREET_GX_SET_TEV_ORDER,0,0,0,4)||!GX(x->d,NFLSTREET_GX_SET_TEV_OP,0,0)||
   !GX(x->d,NFLSTREET_GX_SET_NUM_TEX_GENS,1)||!GX(x->d,NFLSTREET_GX_SET_TEX_COORD_GEN,0,1,4,0x1e,0,0x7d)||!GX(x->d,NFLSTREET_GX_SET_VTX_DESC,0xd,1))return 0;}
 else if(!GX(x->d,NFLSTREET_GX_SET_NUM_TEV_STAGES,1)||!GX(x->d,NFLSTREET_GX_SET_TEV_ORDER,0,0xff,0xff,4)||!GX(x->d,NFLSTREET_GX_SET_TEV_OP,0,4)||
   !GX(x->d,NFLSTREET_GX_SET_NUM_IND_STAGES,0)||!GX(x->d,NFLSTREET_GX_SET_TEV_DIRECT,0)||!GX(x->d,NFLSTREET_GX_SET_NUM_TEX_GENS,0)||
   !GX(x->d,NFLSTREET_GX_SET_VTX_DESC,0xd,0))return 0;
 return put(x,x->u->channel_cache,mode);}
/* 801D7BA0 + 801D8A9C + 801D8848: the image's texture (wrap where a
   coordinate passes 0x100), loaded as map 0. */
static int image_texture(const uis *x,uint32_t object,uint32_t record){
 const uint8_t *o=at(x,object+0x24,16),*r=at(x,record,0x18);if(!o||!r)return 0;
 uint32_t s=0,t=0;for(unsigned k=0;k<4;++k){if(rd16(o+4*k)>0x100)s=1;if(rd16(o+4*k+2)>0x100)t=1;}
 const uint32_t image=rd32(r+0xc),format=rd32(r+8),lut=rd32(r+0x14),lut_format=rd32(r+0x10);const uint16_t w=rd16(r),hh=rd16(r+2);
 uint8_t *tex=at(x,x->u->texture,0x20);if(!tex)return 0;
 if(!lut)nflstreet_gx_init_tex_obj(tex,image,w,hh,format,s,t);
 else if(lut_format==6)return 0; /* indirect texture: not modeled */
 else nflstreet_gx_init_tex_obj_ci(tex,image,w,hh,format,s,t,0xf);
 if(!nflstreet_gx_init_tex_obj_lod(tex,&x->u->texobj,1,1,x->u->lod,x->u->lod,x->u->lod,0,0,0))return 0;
 if(lut){uint8_t *tl=at(x,x->u->tlut,12);if(!tl)return 0;nflstreet_gx_init_tlut_obj(tl,lut,lut_format,format==9?0x100:0x10);
  if(!GX(x->d,NFLSTREET_GX_LOAD_TLUT,x->u->tlut,0xf))return 0;}
 return GX(x->d,NFLSTREET_GX_LOAD_TEX_OBJ,x->u->texture,0);}
/* 801D73E0 / 801D6C00: four corners, colors from +14 (palette-mapped),
   texture coordinates from +24, positions from +34, written to the pipe. */
static int image_vertices(const uis *x,uint32_t object,uint32_t flags){
 const uint8_t *o=at(x,object,0x4c);int32_t a,b;uint32_t pal=0;if(!o)return 0;
 if(!half(x,object+8,&b)||!half(x,object+0xa,&a))return 0;
 if(b!=-1&&a!=-1&&!palette(x,a,b,&pal))return 0;
 for(unsigned k=0;k<4;++k){uint32_t color=rd32(o+0x14+4*k);
  if(pal&&(!(flags&2)||!(flags&0x10))&&!remap(x,pal,&color))return 0;
  for(unsigned c=0;c<3;++c)if(!GX(x->d,NFLSTREET_GX_WRITE,2,rd16(o+0x34+6*k+2*c)))return 0;
  for(unsigned c=0;c<4;++c)if(!GX(x->d,NFLSTREET_GX_WRITE,1,color>>(24-8*c)&0xffu))return 0;
  if((flags&2)&&(!GX(x->d,NFLSTREET_GX_WRITE,2,rd16(o+0x24+4*k))||!GX(x->d,NFLSTREET_GX_WRITE,2,rd16(o+0x26+4*k))))return 0;}
 return 1;}
/* 801D6A54. */
static int image_quad(const uis *x,uint32_t object,uint32_t record){
 uint32_t flags,top,m[12];const uint8_t *f=at(x,object,2);if(!f)return 0;flags=rd16(f);
 if((flags&2)&&!record)return 1;
 if(flags&0x8008u)return 0; /* display list / nine-slice strip: not modeled */
 nflstreet_draw_reset(x->d);
 if(!image_state(x,flags)||!GX(x->d,NFLSTREET_GX_SET_CULL_MODE,flags&4?2:0)||!nflstreet_matrix_top(x->d,K,&top))return 0;
 const uint8_t *t=at(x,top,0x30);if(!t)return 0;for(unsigned i=0;i<12;++i)m[i]=rd32(t+4*i);
 if(!nflstreet_draw_gx(x->d,NFLSTREET_GX_LOAD_POS_MTX,1,(uint32_t[]){0},m,12)||!GX(x->d,NFLSTREET_GX_SET_CURRENT_MTX,0))return 0;
 if(flags&2){uint32_t sx,sy,tm[8];const uint8_t *q;
  if(!word(x,record+0x30,&sx)||!word(x,record+0x34,&sy)||!put(x,x->u->texture_matrix,sx)||!put(x,x->u->texture_matrix+0x14,sy)||
     !(q=at(x,x->u->texture_matrix,32)))return 0;
  for(unsigned i=0;i<8;++i)tm[i]=rd32(q+4*i);
  if(!nflstreet_draw_gx(x->d,NFLSTREET_GX_LOAD_TEX_MTX,2,(uint32_t[]){0x1e,1},tm,8)||!image_texture(x,object,record))return 0;}
 return GX(x->d,NFLSTREET_GX_BEGIN,0x80,0,4)&&image_vertices(x,object,flags)&&GX(x->d,NFLSTREET_GX_SET_CULL_MODE,0);}
/* 801D6690: resource checks, the image record (801D5C34) and the quad. */
static int image_draw(const uis *x,uint32_t object){
 uint32_t flags,record=0;int32_t a,e,pa,pe;const uint8_t *f=at(x,object,2);if(!f)return 0;flags=rd16(f);
 if(!half(x,object+6,&a)||!half(x,object+4,&e)||!half(x,object+0xc,&pe)||!half(x,object+0xe,&pa))return 0;
 if((flags&2)&&a!=-1&&e!=-1){
  uint32_t ar,af,en,rec,obj;int state;uint8_t refresh;
  if(!archive(x,a,&ar)||!word(x,ar+0x10,&af)||!entry(x,a,e,&en))return 0;
  if((af&0x7fffffffu)&2){if(!word(x,en+4,&rec)||!byte(x,x->u->refresh,&refresh))return 0;if(rec&&refresh)return 0;} /* 801DD5E8 */
  if(pe!=-1&&pa!=-1){state=0;if((af&0x10)&&(!word(x,ar+0x14,&obj)||!archive_state(x,obj,e,0x10,&state)))return 0;
   if(!state){if(a!=pa||e!=pe)return 0; /* 801D84D8 exchange: not modeled */
    if(!put16(x,object+0xe,0xffffu)||!put16(x,object+0xc,0xffffu))return 0;}}
  /* 801D5C34 */
  if(!word(x,en+4,&rec))return 0;
  if((af&0x10)&&!rec&&!(af&0xa)){if(!word(x,ar+0x14,&obj)||!archive_state(x,obj,e,0x2,&state))return 0;if(state)return 0;} /* load */
  const uint8_t *count=at(x,en,2);if(!count)return 0;
  if(rd16(count)){if(x->u->notify||!(af&4))return 0;} /* 801D5DC0 with flag 4: nothing */
  if(!word(x,en+4,&record))return 0;}
 int32_t pb,pc;if(!half(x,object+8,&pb)||!half(x,object+0xa,&pc))return 0;
 if(record&&(flags&0x10)&&pb!=-1&&pc!=-1)return 0; /* 801D7D1C acquisition: not modeled */
 return image_quad(x,object,record);}

/* 801D97D4: a text object's lines through 80186B1C. */
static int text_color(const uis *x,uint32_t color,uint32_t cur,uint32_t slot){
 const uint8_t *c=at(x,cur,0x20);uint8_t *s=at(x,x->h->text_state+4+0x10*slot,16);if(!c||!s)return 0;
 for(unsigned k=0;k<4;++k){float v=((float)(color>>(24-8*k)&0xffu)+f32(rd32(c+0x10+4*k)));
  nflstreet_wr32(s+4*k,bits(x->u->color_unit*v*f32(rd32(c+4*k))));}
 return 1;}
static int text_draw(const uis *x,uint32_t object){
 const uint8_t *o=at(x,object,0x28);if(!o)return 0;
 uint32_t en,rec,font,dirty,cur,state,color[2],mode=0,header;uint8_t on,line_height;
 if(!entry(x,(int8_t)o[3],(int8_t)o[2],&en)||!word(x,en+4,&rec))return 0;if(!rec)return 1;
 if(!word(x,rec,&font)||!word(x,rec+4,&dirty))return 0;if(dirty)return 0; /* 801861C4 */
 if(!word(x,x->u->color_current,&cur)||!word(x,x->h->text_state,&state))return 0;
 const uint32_t flags=rd16(o);
 if(!put(x,x->h->text_state,(state&~0xf0u)|(flags&0xf0u)))return 0;
 color[0]=rd32(o+0x18);color[1]=rd32(o+0x1c);
 if((int8_t)o[5]!=-1&&(int8_t)o[4]!=-1){uint32_t pal;if(!palette(x,(int8_t)o[5],(int8_t)o[4],&pal))return 0;
  if(pal)for(unsigned k=0;k<2;++k)if(!remap(x,pal,color+k))return 0;}
 if(!text_color(x,color[0],cur,0))return 0;
 if(flags&0x50){uint8_t *sh=at(x,x->h->text_state+0x24,12);if(!text_color(x,color[1],cur,1)||!sh)return 0;
  for(unsigned k=0;k<3;++k)nflstreet_wr32(sh+4*k,bits((float)(int8_t)o[0xc+k]));}
 if(!byte(x,x->u->text_mode_on,&on))return 0;if(on&&!word(x,x->u->text_mode,&mode))return 0;
 if(mode!=2&&(!nflstreet_text_setup(x->d)||!put(x,x->u->text_mode,2)))return 0;
 if(!put(x,x->u->draw_mode,2))return 0;
 if(flags&0x8000)return 0; /* 801D8E54 */
 float p[3]={(float)(int16_t)rd16(o+6),(float)(int16_t)rd16(o+8),(float)(int16_t)rd16(o+0xa)};
 if(flags&0x100){if(flags&1)p[0]+=(float)((int16_t)rd16(o+0x14)>>1);else if(flags&2)p[0]+=(float)(int16_t)rd16(o+0x14);}
 const int32_t limit=nflstreet_fctiwz((float)(int16_t)rd16(o+0x16)+p[1]);
 if(!word(x,font+4,&header)||!byte(x,header+0x13,&line_height))return 0;
 const float step=(float)(int32_t)line_height;
 uint32_t line=o[0x10];if(line>=o[0xf])return 1;
 if((flags&0x200)&&p[1]+step>(float)limit)return 1;
 uint32_t buffer;if(!word(x,rd32(o+0x20)+8,&buffer))return 0;
 for(;;){uint32_t span=o[0xf]>1?rd32(o+0x24)+4u*line:object+0x24+4u*line;const uint8_t *sp=at(x,span,4);if(!sp)return 0;
  uint8_t *end=at(x,buffer+rd16(sp+2),1);if(!end)return 0;const uint8_t kept=*end;*end=0;
  if(!nflstreet_text_draw_at(x->d,x->h,font,buffer+rd16(sp),flags&0xf,x->u->text_limit,p))return 0;
  *end=kept;p[1]+=step;++line;
  if(line>=o[0xf])return 1;
  if((flags&0x200)&&!(p[1]+step<=(float)limit))return 1;}}

static int handle(const uis *x,uint32_t screen,uint32_t cls,uint32_t object){
 uint32_t n,table,handler;if(!word(x,screen+0x48,&n))return 0;if(cls>=n)return 1;
 if(!word(x,screen+0x4c,&table)||!word(x,table+4*cls,&handler))return 0;if(!handler)return 1;
 if(handler==x->u->image_class)return object&&image_draw(x,object);
 if(handler==x->u->text_class)return object&&text_draw(x,object);
 return handler==x->u->quiet_classes[0]||handler==x->u->quiet_classes[1];}
static int set_tint(const uis *x,const uint32_t v[8]){uint8_t *t=at(x,x->u->tint,0x20);if(!t)return 0;
 for(unsigned i=0;i<8;++i)nflstreet_wr32(t+4*i,v[i]);return 1;}
/* 801CEFDC. */
static int node(const uis *x,uint32_t screen,uint32_t root,uint32_t index,unsigned depth){
 uint32_t data,c,nodes,count,list,callback;if(depth>64)return 0;
 if(!word(x,root+0x10,&data)||!word(x,root+0xc,&c))return 0;if(!data||c)return 1;
 if(!word(x,data+4,&nodes))return 0;const uint32_t slot=nodes+0x14u*index;
 for(uint32_t k=0;;++k){uint32_t d2,ptr,off;const uint8_t *e;
  if(!word(x,root+0x10,&d2)||!word(x,d2+0x18,&count))return 0;if(k>=count)break;
  if(!word(x,d2+0x1c,&list)||!(e=at(x,list+8*k,8)))return 0;ptr=rd32(e+4);off=0;
  if(ptr&&!word(x,ptr,&off))return 0;if(!handle(x,screen,rd16(e),ptr?d2+off:0))return 0;}
 uint32_t n,visible,saved[8],tint[8];if(!word(x,slot,&n)||!word(x,n,&visible))return 0;if(!visible)return 1;
 const uint8_t *t=at(x,x->u->tint,0x20),*v=at(x,n+0x3c,0x20);if(!t||!v)return 0;
 for(unsigned i=0;i<8;++i)saved[i]=rd32(t+4*i);
 for(unsigned k=0;k<4;++k){tint[k]=bits(f32(saved[k])*f32(rd32(v+4*k)));tint[4+k]=bits(f32(saved[4+k])+f32(rd32(v+0x10+4*k)));}
 if(!set_tint(x,tint)||!word(x,screen+0x1c,&callback)||callback!=x->u->transform||!transform_push(x,n+8))return 0;
 for(uint32_t g=0;;++g){uint32_t groups,gl,gp,en,enabled,entries;
  if(!word(x,slot+4,&groups))return 0;if(g>=groups)break;
  if(!word(x,slot+8,&gl)||!word(x,gl+4*g,&gp)||!word(x,gp,&en)||!word(x,en,&enabled))return 0;if(!enabled)continue;
  for(uint32_t k=0;;++k){uint32_t el,ptr,off=0,d2;const uint8_t *e;
   if(!word(x,gp+4,&entries))return 0;if(k>=entries)break;
   if(!word(x,gp+8,&el)||!(e=at(x,el+8*k,8)))return 0;ptr=rd32(e+4);
   if(rd16(e)==0xffffu){if(!node(x,screen,root,ptr,depth+1))return 0;continue;}
   if(!(int16_t)rd16(e+2))continue;
   if(ptr&&(!word(x,root+0x10,&d2)||!word(x,ptr,&off)))return 0;
   if(!handle(x,screen,rd16(e),ptr?d2+off:0))return 0;}}
 return set_tint(x,saved)&&transform_pop(x);}
int nflstreet_uis_screen_draw(const nflstreet_uis_context *u,uint32_t screen,nflstreet_render_state *s){
 if(!u||!u->hud||!u->hud->scene||!u->hud->scene->draw||!u->hud->scene->draw->span||!u->hud->scene->draw->emit||!s||!screen)return 0;
 nflstreet_draw d={u->hud->scene->draw,s};const uis x={&d,u,u->hud};
 uint32_t start[8];for(unsigned i=0;i<8;++i)start[i]=bits(u->tint_start[i<4?0:1]);
 uint32_t roots,list,callback;if(!set_tint(&x,start)||!word(&x,screen+0x38,&roots)||!word(&x,screen+0x3c,&list)||!word(&x,screen+0x28,&callback))return 0;
 if(callback)return 0; /* root callbacks: not modeled */
 for(uint32_t r=0;r<roots;++r)if(!put(&x,u->mark,0)||!node(&x,screen,list+0x14u*r,0,0))return 0;
 return 1;}
int nflstreet_hud_screens_draw(const nflstreet_uis_context *u,uint32_t camera,nflstreet_render_state *s){
 if(!u||!u->hud||!u->hud->scene||!u->hud->scene->draw||!u->hud->scene->draw->span||!u->hud->scene->draw->emit||!s)return 0;
 nflstreet_draw d={u->hud->scene->draw,s};const uis x={&d,u,u->hud};
 uint8_t on;uint32_t hook,target,selected,flags,fog,unused;
 if(!put(&x,u->text_mode,0)||!put(&x,u->draw_mode,0)||!nflstreet_hud_profile(&d,u->hud,0xd))return 0;
 /* 8012A880: pending loads */
 if(!byte(&x,u->pending_on,&on))return 0;
 if(on)for(unsigned i=0;i<2;++i){uint32_t id;if(!word(&x,u->pending+0x10u*i+4,&id))return 0;if(id!=0xffffffffu)return 0;}
 if(!GX(&d,NFLSTREET_GX_SET_Z_MODE,0,3,1)||!nflstreet_text_defaults(&d,u->hud))return 0;
 /* 80004668 → 801D5F60: refreshes for archives with flag 2 only */
 if(!word(&x,u->refresh_hook,&hook)||!word(&x,hook+4,&target))return 0;
 if(target==u->refresh_target&&u->manager){uint32_t list;const uint8_t *n=at(&x,u->manager+4,2);if(!n||!word(&x,u->manager+0xc,&list))return 0;
  for(uint32_t a=0;a<rd16(n);++a){uint32_t af;int32_t items;if(!word(&x,list+0x24*a+0x10,&af)||!half(&x,list+0x24*a+0x1e,&items))return 0;
   if(items>0&&!(af&4)&&!(af&8)&&(af&2))return 0;}} /* 801DD184 */
 if(!nflstreet_scenery_fog_switch(&d,u->hud->scene,0,&fog)||!word(&x,u->camera,&selected)||!word(&x,u->screen_flags,&flags))return 0;
 if(camera==selected){static const uint32_t enable[4]={8,1,2,4};
  for(unsigned i=0;i<4;++i){uint32_t screen;if(!word(&x,u->screens[i],&screen))return 0;
   if(screen&&(flags&enable[i])&&!nflstreet_uis_screen_draw(u,screen,s))return 0;
   if(i==1&&screen&&(flags&1)&&!put(&x,u->after_second,0))return 0;
   if(!word(&x,u->screen_flags,&flags))return 0;}}
 return nflstreet_scenery_fog_switch(&d,u->hud->scene,fog,&unused)&&GX(&d,NFLSTREET_GX_SET_Z_MODE,1,3,1);}
