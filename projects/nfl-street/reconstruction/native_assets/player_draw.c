#include "player_draw.h"
#include <string.h>
typedef struct {const nflstreet_player_draw_context *x;nflstreet_render_state *s;} draw;
static uint32_t rd32(const uint8_t *p){return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static uint16_t rd16(const uint8_t *p){return(uint16_t)((unsigned)p[0]<<8|p[1]);}
static void wr32(uint8_t *p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
static void wr16(uint8_t *p,uint32_t v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static float as_float(uint32_t v){float f;memcpy(&f,&v,4);return f;}
static uint32_t bits(float f){uint32_t v;memcpy(&v,&f,4);return v;}
static uint8_t *at(draw *d,uint32_t token,uint32_t size){return token?d->x->span(d->x->memory,token,size):NULL;}
static int word(draw *d,uint32_t token,uint32_t *v){uint8_t *p=at(d,token,4);if(!p)return 0;*v=rd32(p);return 1;}
static int gx(draw *d,nflstreet_gx_op op,unsigned n,const uint32_t *a,const uint32_t *data,unsigned words){
 nflstreet_gx_call c;memset(&c,0,sizeof c);c.op=op;
 for(unsigned i=0;i<n;++i)c.arg[i]=a[i];for(unsigned i=0;i<words;++i)c.data[i]=data[i];
 return d->x->emit(d->x->sink,&c);}
#define GX(d,op,...) gx(d,op,sizeof((uint32_t[]){__VA_ARGS__})/4u,(uint32_t[]){__VA_ARGS__},NULL,0)
/* Gekko fmadds: a*c+b rounded once to single (the recompiler corrects
   double-rounding ties the same way). The product of two singles is exact. */
static float fmadds(float a,float c,float b){
 double p=(double)a*(double)c,s=p+(double)b,v=s-p,e=(p-(s-v))+((double)b-v);uint64_t q;memcpy(&q,&s,8);
 if((q&0x1fffffffull)==0x10000000ull&&e!=0.0){q+=(e>0.0)==(s>0.0)?1u:(uint64_t)-1;memcpy(&s,&q,8);}
 return (float)s;}
/* fctiwz: truncation with the PowerPC saturation and NaN result. */
static int32_t fctiwz(float v){
 if(v!=v||v<-2147483648.0f)return INT32_MIN;if(v>=2147483648.0f)return INT32_MAX;return (int32_t)v;}

/* 801C7D98 and 801C7914 (model view, or 0). */
static void model_20(draw *d,uint32_t v){d->s->model_20=v;for(unsigned i=0;i<4;++i)d->s->matrix_cache[i]=0xfffffffeu;}
static int select_model(draw *d,uint32_t view){
 nflstreet_render_state *s=d->s;
 if(!view){s->chunks=s->model_10=s->groups=s->overrides=s->model_24=s->model_28=s->model_30=s->model_04=0;model_20(d,0);s->format=0xffffffffu;}
 else{const uint8_t *v=at(d,view,0x34);if(!v)return 0;
  if(s->chunks!=rd32(v+8)){s->format=0xffffffffu;s->chunks=rd32(v+8);}
  s->groups=rd32(v+0x14);s->overrides=rd32(v+0x1c);s->model_24=rd32(v+0x24);s->model_28=rd32(v+0x28);s->model_30=rd32(v+0x30);
  s->model_04=rd32(v+4);s->model_10=rd32(v+0x10);model_20(d,rd32(v+0x20));}
 s->lod=0;return 1;}
/* 801C7648. */
static void reset(draw *d){select_model(d,0);d->s->materials=d->s->override=d->s->instance=d->s->shader=d->s->effects=d->s->blend=0;}
/* 801C79E4: both matrix arrays use the palette's first word. */
static int palette(draw *d,uint32_t p){
 d->s->palette=p;if(!p)return 1;uint32_t base;
 return word(d,p,&base)&&GX(d,NFLSTREET_GX_SET_ARRAY,0x15,base,0x30)&&GX(d,NFLSTREET_GX_SET_ARRAY,0x16,base,0x10);}
/* 801C7DC0 blend presets. */
static int blend(draw *d,uint32_t mode){
 static const uint8_t presets[7][3]={{0,0,0},{0,4,5},{1,4,5},{1,0,2},{1,0,4},{1,1,1},{1,4,1}};
 if(mode==d->s->blend)return 1;d->s->blend=mode;
 if(!mode||mode>6)return 1;const uint8_t *p=presets[mode];
 return GX(d,NFLSTREET_GX_SET_BLEND_MODE,p[0],p[1],p[2],5);}
/* 801C7EAC. */
static int depth_range(draw *d,float near_z,float far_z){
 nflstreet_render_state *s=d->s;s->viewport[4]=near_z;s->viewport[5]=far_z;uint32_t v[6];
 for(unsigned i=0;i<6;++i)v[i]=bits(s->viewport[i]);
 return gx(d,NFLSTREET_GX_SET_VIEWPORT,0,NULL,v,6);}

/* 801CACE4: GXSetTevOp-style presets; stage 0 reads the raster color/alpha,
   later stages the previous result. Scale 1 doubles the result. */
static int tev_op(draw *d,uint32_t stage,uint32_t mode,uint32_t scale){
 uint32_t rc=stage?0:10,ra=stage?0:5;int ok=1;
 switch(mode){
 case 0:ok=GX(d,NFLSTREET_GX_SET_TEV_COLOR_IN,stage,0xf,8,rc,0xf)&&GX(d,NFLSTREET_GX_SET_TEV_ALPHA_IN,stage,7,4,ra,7);break;
 case 1:ok=GX(d,NFLSTREET_GX_SET_TEV_COLOR_IN,stage,rc,8,9,0xf)&&GX(d,NFLSTREET_GX_SET_TEV_ALPHA_IN,stage,7,7,7,ra);break;
 case 2:ok=GX(d,NFLSTREET_GX_SET_TEV_COLOR_IN,stage,rc,0xc,8,0xf)&&GX(d,NFLSTREET_GX_SET_TEV_ALPHA_IN,stage,7,4,ra,7);break;
 case 3:ok=GX(d,NFLSTREET_GX_SET_TEV_COLOR_IN,stage,0xf,0xf,0xf,8)&&GX(d,NFLSTREET_GX_SET_TEV_ALPHA_IN,stage,7,7,7,4);break;
 case 4:ok=GX(d,NFLSTREET_GX_SET_TEV_COLOR_IN,stage,0xf,0xf,0xf,rc)&&GX(d,NFLSTREET_GX_SET_TEV_ALPHA_IN,stage,7,7,7,ra);break;
 default:break;}
 return ok&&GX(d,NFLSTREET_GX_SET_TEV_COLOR_OP,stage,0,0,scale,1,0)&&GX(d,NFLSTREET_GX_SET_TEV_ALPHA_OP,stage,0,0,scale,1,0);}
/* Shader setup callbacks 801C9D0C (NULL), 801C9DAC (OnePass: texture times
   raster, doubled) and 801C9DFC (TwoPass: base, decal overlay by its alpha,
   then times raster, doubled; alpha = constant times raster). */
static int setup(draw *d,unsigned kind){
 switch(kind){
 case 0:return GX(d,NFLSTREET_GX_SET_NUM_TEX_GENS,1)&&GX(d,NFLSTREET_GX_SET_NUM_TEV_STAGES,1)&&
  GX(d,NFLSTREET_GX_SET_TEV_ORDER,0,0xff,0xff,4)&&tev_op(d,0,4,1);
 case 1:return GX(d,NFLSTREET_GX_SET_NUM_TEX_GENS,1)&&GX(d,NFLSTREET_GX_SET_NUM_TEV_STAGES,1)&&
  GX(d,NFLSTREET_GX_SET_TEV_ORDER,0,0,0,4)&&tev_op(d,0,0,1);
 default:return GX(d,NFLSTREET_GX_SET_NUM_TEX_GENS,2)&&GX(d,NFLSTREET_GX_SET_NUM_TEV_STAGES,3)&&
  GX(d,NFLSTREET_GX_SET_TEV_ORDER,0,0,0,0xff)&&tev_op(d,0,3,0)&&GX(d,NFLSTREET_GX_SET_TEV_ORDER,1,1,1,0xff)&&tev_op(d,1,1,0)&&
  GX(d,NFLSTREET_GX_SET_TEV_ORDER,2,0xff,0xff,4)&&GX(d,NFLSTREET_GX_SET_TEV_COLOR_IN,2,0xf,0,10,0xf)&&
  GX(d,NFLSTREET_GX_SET_TEV_COLOR_OP,2,0,0,1,1,0)&&GX(d,NFLSTREET_GX_SET_TEV_ALPHA_IN,2,7,6,5,7)&&
  GX(d,NFLSTREET_GX_SET_TEV_ALPHA_OP,2,0,0,1,1,0);}}
/* 801C7A3C: shader object {definition, setup, teardown, -, stages +10,
   parameter count +14, parameters +18}; 0 selects the NULL shader. */
static int apply_shader(draw *d,uint32_t shader){
 nflstreet_render_state *s=d->s;if(!shader)shader=d->x->null_shader;
 if(shader!=s->shader){uint32_t teardown=0,def,unused,callback;s->override=0;
  if(s->shader&&(!word(d,s->shader+8,&teardown)||teardown))return 0;
  if(!word(d,shader,&def)||!word(d,def+0x14,&unused)||!word(d,shader+4,&callback))return 0;(void)unused;
  if(callback){unsigned k=0;while(k<3&&d->x->setups[k]!=callback)++k;if(k==3||!setup(d,k))return 0;}}
 s->shader=shader;return 1;}
/* 801C7AD4: texture map load; a TLUT object whose first word is 0 is loaded
   and named by the map first. A null texture loads the default texture. */
static int load_texture(draw *d,uint32_t map,uint32_t texture,uint32_t tlut){
 if(!texture)return !d->x->default_texture||load_texture(d,map,d->x->default_texture,0);
 if(tlut){uint32_t format;if(!word(d,tlut,&format))return 0;
  if(!format&&(!GX(d,NFLSTREET_GX_LOAD_TLUT,tlut+4,map)||!GX(d,NFLSTREET_GX_INIT_TEX_OBJ_TLUT,texture,map)))return 0;}
 return GX(d,NFLSTREET_GX_LOAD_TEX_OBJ,texture,map);}
/* 801C7B58: material instance {-, name, shader +08, stages +0C, stage
   entries +10 (texture, palette, index), parameter values +14}. */
static int apply_instance(draw *d,uint32_t instance){
 nflstreet_render_state *s=d->s;if(instance==s->instance)return 1;
 if(!instance){if(!apply_shader(d,0))return 0;s->instance=0;return 1;}
 uint32_t shader,stages,params,list,entries;
 if(!word(d,instance+8,&shader)||!shader||!apply_shader(d,shader)||s->override)return 0;
 if(!word(d,shader+0x10,&stages)||!word(d,shader+0x14,&params))return 0;
 if((int32_t)stages>0&&!word(d,instance+0x10,&entries))return 0;
 for(int32_t i=0;i<(int32_t)stages;++i){const uint8_t *e=at(d,entries+12u*(uint32_t)i,8);
  if(!e||!load_texture(d,(uint32_t)i,rd32(e),rd32(e+4)))return 0;}
 if((int32_t)params>0&&!word(d,shader+0x18,&list))return 0;
 for(int32_t i=0;i<(int32_t)params;++i){const uint8_t *r=at(d,list+0x14u*(uint32_t)i,0x14);if(!r)return 0;
  uint32_t value=instance+0x14u+4u*r[0x11],id=rd16(r+0x12),v[4];int ok=1;
  switch(r[0x10]){
  case 2:ok=word(d,value,v)&&gx(d,NFLSTREET_GX_SET_TEV_COLOR,1,(uint32_t[]){id+1},v,1);break;
  case 3:return 0;
  case 4:ok=word(d,value,v)&&gx(d,NFLSTREET_GX_SET_CHAN_MAT_COLOR,1,&id,v,1);break;
  case 5:ok=word(d,value,v)&&gx(d,NFLSTREET_GX_SET_CHAN_AMB_COLOR,1,&id,v,1);break;
  case 6:{const uint8_t *p=at(d,value,16);if(!p)return 0;
   uint32_t m[8]={rd32(p),0,0,rd32(p+8),0,rd32(p+4),0,rd32(p+12)};
   ok=gx(d,NFLSTREET_GX_LOAD_TEX_MTX,2,(uint32_t[]){0x1e,1},m,8)&&GX(d,NFLSTREET_GX_SET_TEX_COORD_GEN,id,1,4,0x1e,0,0x7d);break;}
  default:break;}
  if(!ok)return 0;}
 s->instance=instance;return 1;}
/* 801C7D54/801C7498: signed index into the current material array. */
static int material(draw *d,int32_t index){
 uint32_t array=d->s->materials,count,list,instance=0;if(!array)return 1;
 if(!word(d,array,&count))return 0;
 if(index>=0&&index<(int32_t)count&&(!word(d,array+4,&list)||!word(d,list+4u*(uint32_t)index,&instance)))return 0;
 return apply_instance(d,instance);}

/* 801C826C/801CCBD8/801C7F18 for one 0x18-byte chunk {display list, size/32
   +04, material +06, first array +0E, arrays +10, flags +11, format +12,
   LOD count +14} with 12-byte array records {base, -, attribute +06, stride
   +07, count +08, type +09, fraction +0A, descriptor type +0B}. Flags bit 0:
   matrix-index attribute; bit 3: S16 positions are read as F32 (stride 12). */
static int draw_chunk(draw *d,uint32_t chunk){
 nflstreet_render_state *s=d->s;const uint8_t *c=at(d,chunk,0x18);if(!c||!material(d,(int16_t)rd16(c+6)))return 0;
 if(s->effects)return 0;
 const uint8_t *l=c;if(s->lod&&c[0x14]>=s->lod&&!(l=at(d,chunk+0x18u*s->lod,0x18)))return 0;
 uint32_t arrays,n=l[0x10],first=rd16(l+0xe),base[8];const uint8_t *rec[8];uint8_t type[8],stride[8],frac[8];
 if(n>8||!word(d,s->chunks+8,&arrays))return 0;
 for(uint32_t i=0;i<n;++i){
  if(!(rec[i]=at(d,arrays+0xcu*(first+i),0xc)))return 0;type[i]=stride[i]=frac[i]=0xff;base[i]=rd32(rec[i]);
  if(s->overrides){uint32_t o;if(!word(d,s->overrides+4u*(first+i),&o))return 0;if(o)base[i]=o;}}
 if(c[0x11]&8)for(uint32_t i=0;i<n;++i)if(rec[i][6]==9&&rec[i][9]==3){type[i]=4;stride[i]=0xc;}
 if(c[0x12]==s->format){
  for(uint32_t i=0;i<n;++i)if(!GX(d,NFLSTREET_GX_SET_ARRAY,rec[i][6],base[i],stride[i]==0xff?rec[i][7]:stride[i]))return 0;}
 else{if(!gx(d,NFLSTREET_GX_CLEAR_VTX_DESC,0,NULL,NULL,0))return 0;
  for(uint32_t i=0;i<n;++i){const uint8_t *r=rec[i];
   if(!GX(d,NFLSTREET_GX_SET_VTX_DESC,r[6],r[0xb]&0xfu)||
      !GX(d,NFLSTREET_GX_SET_VTX_ATTR_FMT,0,r[6],r[8],type[i]==0xff?r[9]:type[i],frac[i]==0xff?r[10]:frac[i])||
      !GX(d,NFLSTREET_GX_SET_ARRAY,r[6],base[i],stride[i]==0xff?r[7]:stride[i]))return 0;}
  if((c[0x11]&1)&&!GX(d,NFLSTREET_GX_SET_VTX_DESC,0,1))return 0;
  if(c[0x12])s->format=c[0x12];}
 return GX(d,NFLSTREET_GX_CALL_DISPLAY_LIST,rd32(c),(uint32_t)rd16(c+4)<<5)&&(!(c[0x11]&1)||GX(d,NFLSTREET_GX_SET_CURRENT_MTX,0));}
/* 801C8460: draw the listed chunks of the current model. */
static int draw_list(draw *d,uint32_t list,uint32_t count){
 uint32_t chunks,total;if(!d->s->chunks||!count)return 1;
 if(!word(d,d->s->chunks,&chunks)||!word(d,d->s->chunks+4,&total))return 0;
 for(uint32_t i=0;i<count;++i){const uint8_t *p=at(d,list+2u*i,2);if(!p||rd16(p)>=total||!draw_chunk(d,chunks+0x18u*rd16(p)))return 0;}
 return 1;}
/* 801CD43C: group table {count, groups (0x14: first variant +10, variants
   +12), variants (0x14: first index +10, count +12), -, index list +0C}. */
static int variant(draw *d,uint32_t groups,int32_t group,int32_t v,uint32_t *out){
 uint32_t count,list,variants;*out=0;if(!word(d,groups,&count))return 0;if(group>=(int32_t)count)return 1;
 const uint8_t *g;if(!word(d,groups+4,&list)||!(g=at(d,list+0x14u*(uint32_t)group,0x14)))return 0;if(v>=(int32_t)g[0x12])return 1;
 if(!word(d,groups+8,&variants))return 0;*out=variants+0x14u*rd16(g+0x10)+0x14u*(uint32_t)v;return 1;}
/* 801C8544: draw one group variant of the current model. */
static int draw_group(draw *d,int32_t group,int32_t v){
 uint32_t groups=d->s->groups,e,list;const uint8_t *p;if(!groups)return 1;
 if(!variant(d,groups,group,v,&e))return 0;if(!e)return 1;
 return (p=at(d,e,0x14))&&word(d,groups+0xc,&list)&&draw_list(d,list+2u*rd16(p+0x10),rd16(p+0x12));}
/* 801EC0D4/801EB12C/801CD484: rebuild an instance's chunk list from its
   group selectors (+1C, 0xFF hides a group) when +14 or +18 is set. */
static int refresh(draw *d,uint32_t instance){
 uint8_t *in=at(d,instance,0xe0);if(!in)return 0;uint32_t model=rd32(in+0xc);
 if(!model||!(rd32(in+0x14)|rd32(in+0x18)))return 1;
 uint32_t groups,count=0,n=0,list;if(!word(d,model+0x14,&groups))return 0;
 if(groups&&!word(d,groups,&count))return 0;if(count>0x3f)count=0x40;
 for(uint32_t i=0;i<count;++i){uint8_t sel=in[0x1c+i];uint32_t e;if(sel==0xff)continue;
  if(!variant(d,groups,(int32_t)i,sel,&e))return 0;if(!e)continue;const uint8_t *p=at(d,e,0x14);if(!p||!word(d,groups+0xc,&list))return 0;
  uint32_t k=rd16(p+0x12);if(n+k>64)return 0;
  for(uint32_t j=0;j<k;++j){const uint8_t *q=at(d,list+2u*(rd16(p+0x10)+j),2);if(!q)return 0;wr16(in+0x60+2u*(n+j),rd16(q));}
  n+=k;}
 wr32(in+0x5c,n);wr32(in+4,rd32(in+4)|1);wr32(in+8,rd32(in+8)|1);wr32(in+0x18,0);wr32(in+0x14,0);return 1;}
/* 801EC140(0, instance, 0): instance {-, flags +04/+08, model +0C,
   selectors, chunk count +5C, chunk list +60, matrix palette +E0, materials +E8}. */
static int draw_instance(draw *d,uint32_t instance){
 uint32_t model,materials,pal,count;
 if(!refresh(d,instance)||!word(d,instance+0xc,&model)||!select_model(d,model)||!word(d,instance+0xe8,&materials))return 0;
 d->s->materials=materials;
 return word(d,instance+0xe0,&pal)&&palette(d,pal)&&word(d,instance+0x5c,&count)&&draw_list(d,instance+0x60,count);}

/* 800D5800 linear remap with clamping (NaN takes the high value). */
static float remap(float x,float hi,float at_hi,float lo,float at_lo){
 if(!(x<=hi))return at_hi;if(!(x>=lo))return at_lo;if(hi==lo)return 0.0f;return fmadds((x-lo)/(hi-lo),at_hi-at_lo,at_lo);}
/* 8016373C: eye texture offsets from the skeleton's eye angles (+A8..+B0
   halfwords, 24-bit angle units in degrees) when actor flag bit 7 is set. */
static int eye_offsets(draw *d,uint32_t actor,float a[2],float b[2]){
 uint32_t flags,skeleton;if(!word(d,actor+0x14,&flags))return 0;
 if(!(flags&0x80)){a[0]=a[1]=b[0]=b[1]=0.0f;return 1;}
 const uint8_t *e;if(!word(d,actor+0x5c,&skeleton)||!(e=at(d,skeleton+0xa8,10)))return 0;
 /* Halfwords +A8, +AA, (+AC unused), +AE, +B0. */
 float deg[5];for(unsigned i=0;i<5;++i)deg[i]=(float)(int32_t)((uint32_t)(int32_t)(int16_t)rd16(e+2*i)<<8)*2.1457672119140625e-05f;
 b[0]=remap(deg[3],45.619f,-0.03f,-45.619f,0.03f);a[0]=remap(deg[0],45.619f,-0.03f,-45.619f,0.03f);
 b[1]=-remap(deg[4],47.028f,-0.1f,-47.028f,0.1f);a[1]=-remap(deg[1],47.028f,-0.1f,-47.028f,0.1f);
 return 1;}
static int eye_matrix(draw *d,float tx,float ty){
 uint32_t m[8]={bits(1.0f),0,0,bits(tx),0,bits(1.0f),0,bits(ty)};
 return gx(d,NFLSTREET_GX_LOAD_TEX_MTX,2,(uint32_t[]){0x1e,1},m,8)&&GX(d,NFLSTREET_GX_SET_TEX_COORD_GEN,0,1,4,0x1e,0,0x7d);}
/* 80162220: head. The draw struct at actor +4A8 is {actor, flags +04, LOD
   +08, -, LOD model +10}. */
static int draw_head(draw *d,uint32_t actor){
 uint32_t st,lod,flags,model;int compiled=0;
 if(!word(d,actor+0x4a8,&st)||!word(d,st+8,&lod))return 0;
 if(lod)return draw_instance(d,actor+0x59c);
 if(!word(d,st+4,&flags))return 0;
 if((flags&8)&&(!d->x->face||!d->x->face(d->x->face_context,actor,&compiled)))return 0;
 if(compiled)model=d->x->shared_model;else{if(!word(d,st+0x10,&model))return 0;model+=0x10;}
 float a[2],b[2];
 if(!select_model(d,model))return 0;d->s->materials=actor+0xfec;
 return palette(d,actor+0x1014)&&blend(d,1)&&GX(d,NFLSTREET_GX_SET_CULL_MODE,0)&&eye_offsets(d,actor,a,b)&&
  eye_matrix(d,b[0],b[1])&&draw_group(d,3,0)&&eye_matrix(d,a[0],a[1])&&draw_group(d,4,0)&&
  GX(d,NFLSTREET_GX_SET_TEX_COORD_GEN,0,1,4,0x3c,0,0x7d)&&draw_group(d,2,0);}
/* 801ED6A8 for light set 1 (801EDC60): each light object's color scaled
   per channel (clamped at 255, truncated) unless every scale is 1. */
static int lights(draw *d,float r,float g,float b){
 uint32_t set=d->x->light_sets+0x108u,count,mask=0,ambient;
 if(!word(d,set,&count)||count>4)return 0;
 for(uint32_t i=0;i<count;++i){const uint8_t *o=at(d,set+8u+0x40u*i,0x40);if(!o)return 0;uint32_t light[16];
  for(unsigned k=0;k<16;++k)light[k]=rd32(o+4*k);
  if(!(r==1.0f&&g==1.0f&&b==1.0f)){uint8_t c[4];const float scale[3]={r,g,b};wr32(c,light[3]);
   for(unsigned k=0;k<3;++k){float v=(float)c[k]*scale[k];c[k]=v>255.0f?0xff:(uint8_t)fctiwz(v);}
   light[3]=rd32(c);}
  if(!gx(d,NFLSTREET_GX_LOAD_LIGHT,1,(uint32_t[]){1u<<i},light,16))return 0;mask|=1u<<i;}
 if(!word(d,set+4,&ambient)||!gx(d,NFLSTREET_GX_SET_CHAN_AMB_COLOR,1,(uint32_t[]){4},&ambient,1))return 0;
 d->s->light_set=set;d->s->light_mask=mask;return 1;}
/* 80161E10 light color: actor +3EC (target RGB at +0C..+14) is approached
   over 15 frames from +1A74..+1A7C toward +1A64..+1A6C; +1A88 fades by
   (2 - t/30) for 30 frames, otherwise +1A4C == 1 applies +1A50..+1A58. */
static int light_color(draw *d,uint8_t *a,float rgb[3]){
 const float k=0.06666667014360428f;uint32_t target=rd32(a+0x3ec);rgb[0]=rgb[1]=rgb[2]=1.0f;
#define F(o) as_float(rd32(a+(o)))
#define SETF(o,v) wr32(a+(o),bits(v))
 if(target){const uint8_t *t=at(d,target+0xc,12);if(!t)return 0;
  if(!(as_float(rd32(t))==F(0x1a64))||!(as_float(rd32(t+4))==F(0x1a68))||!(as_float(rd32(t+8))==F(0x1a6c))){
   float n=(float)rd32(a+0x1a84),r0=F(0x1a74),g0=F(0x1a78),b0=F(0x1a7c);
   float dr=(F(0x1a64)-r0)*k,dg=(F(0x1a68)-g0)*k,db=(F(0x1a6c)-b0)*k;
   SETF(0x1a7c,fmadds(db,n,b0));SETF(0x1a74,fmadds(dr,n,r0));SETF(0x1a78,fmadds(dg,n,g0));
   wr32(a+0x1a64,rd32(t));wr32(a+0x1a68,rd32(t+4));wr32(a+0x1a84,0);wr32(a+0x1a6c,rd32(t+8));}
  uint32_t step=rd32(a+0x1a84);
  if(step<=14){float n=(float)step;wr32(a+0x1a84,step+1);
   rgb[2]=fmadds((F(0x1a6c)-F(0x1a7c))*k,n,F(0x1a7c));rgb[0]=fmadds((F(0x1a64)-F(0x1a74))*k,n,F(0x1a74));
   rgb[1]=fmadds((F(0x1a68)-F(0x1a78))*k,n,F(0x1a78));}
  else{rgb[0]=F(0x1a64);rgb[1]=F(0x1a68);rgb[2]=F(0x1a6c);}}
 if(rd32(a+0x1a88)){uint32_t step=rd32(a+0x1a8c);
  if(step>0x1d){wr32(a+0x1a8c,0);wr32(a+0x1a88,0);return 1;}
  float w=2.0f-(float)step*0.03333333507180214f;wr32(a+0x1a8c,step+1);
  float sb=F(0x1a58)*w,sr=F(0x1a50)*w,sg=F(0x1a54)*w;rgb[0]*=sr;rgb[1]*=sg;rgb[2]*=sb;}
 else if(a[0x1a4c]==1){rgb[0]*=F(0x1a50);rgb[1]*=F(0x1a54);rgb[2]*=F(0x1a58);}
#undef F
#undef SETF
 return 1;}
int nflstreet_player_draw(const nflstreet_player_draw_context *x,uint32_t actor,nflstreet_render_state *s){
 if(!x||!x->span||!x->emit||!s||!actor)return 0;
 draw d={x,s};uint8_t *a=at(&d,actor,0x1a90);if(!a)return 0;
 reset(&d);
 float dim=(rd32(a+0x14)&8)?x->dim[1]:x->dim[0],far_z=1.0f-as_float(rd32(a+0x1084))*dim,rgb[3];
 if(!light_color(&d,a,rgb)||!lights(&d,rgb[0],rgb[1],rgb[2]))return 0;
 static const uint16_t before[3]={0x4ac,0xd1c,0xe0c},after[2]={0xb3c,0xc2c},late[2]={0x77c,0xefc};
 if(!gx(&d,NFLSTREET_GX_SET_CHAN_MAT_COLOR,1,(uint32_t[]){4},(uint32_t[]){0xffffffffu},1)||!GX(&d,NFLSTREET_GX_SET_NUM_CHANS,1)||
    !GX(&d,NFLSTREET_GX_SET_CHAN_CTRL,4,1,0,0,s->light_mask,2,2)||!GX(&d,NFLSTREET_GX_SET_ALPHA_COMPARE,7,0,0,7,0)||
    !GX(&d,NFLSTREET_GX_SET_CULL_MODE,1)||!blend(&d,1))return 0;
 for(unsigned i=0;i<3;++i)if(!draw_instance(&d,actor+before[i]))return 0;
 if(!draw_head(&d,actor))return 0;
 for(unsigned i=0;i<2;++i)if(!draw_instance(&d,actor+after[i]))return 0;
 if(!depth_range(&d,0.0f,far_z))return 0;
 for(unsigned i=0;i<2;++i)if(!draw_instance(&d,actor+late[i]))return 0;
 if(!GX(&d,NFLSTREET_GX_SET_ALPHA_COMPARE,4,0,0,7,0)||!blend(&d,2)||!draw_instance(&d,actor+0x95c))return 0;
 /* 800DEB30: record +65 = 0 (headwear) draws instance +68C with blend preset 1. */
 const uint8_t *record=at(&d,x->player_records+0x68u*a[0x1363]+0x65u,1);
 if(!record||(!*record&&!blend(&d,1)))return 0;
 return draw_instance(&d,actor+0x68c)&&blend(&d,2)&&draw_instance(&d,actor+0xa4c)&&draw_instance(&d,actor+0x86c)&&
  blend(&d,1)&&depth_range(&d,0.0f,1.0f)&&GX(&d,NFLSTREET_GX_SET_CULL_MODE,0);}
