#include "draw_engine.h"
uint8_t *nflstreet_draw_at(nflstreet_draw *d,uint32_t token,uint32_t size){return token?d->x->span(d->x->memory,token,size):NULL;}
int nflstreet_draw_word(nflstreet_draw *d,uint32_t token,uint32_t *v){uint8_t *p=nflstreet_draw_at(d,token,4);if(!p)return 0;*v=nflstreet_rd32(p);return 1;}
int nflstreet_draw_gx(nflstreet_draw *d,nflstreet_gx_op op,unsigned n,const uint32_t *a,const uint32_t *data,unsigned words){
 nflstreet_gx_call c;memset(&c,0,sizeof c);c.op=op;
 for(unsigned i=0;i<n;++i)c.arg[i]=a[i];for(unsigned i=0;i<words;++i)c.data[i]=data[i];
 return d->x->emit(d->x->sink,&c);}
/* Gekko fmadds: a*c+b rounded once to single (the recompiler corrects
   double-rounding ties the same way). The product of two singles is exact. */
float nflstreet_fmadds(float a,float c,float b){
 double p=(double)a*(double)c,s=p+(double)b,v=s-p,e=(p-(s-v))+((double)b-v);uint64_t q;memcpy(&q,&s,8);
 if((q&0x1fffffffull)==0x10000000ull&&e!=0.0){q+=(e>0.0)==(s>0.0)?1u:(uint64_t)-1;memcpy(&s,&q,8);}
 return (float)s;}
/* fctiwz: truncation with the PowerPC saturation and NaN result. */
int32_t nflstreet_fctiwz(float v){
 if(v!=v||v<-2147483648.0f)return INT32_MIN;if(v>=2147483648.0f)return INT32_MAX;return (int32_t)v;}

/* 801C7D98 and 801C7914 (model view, or 0). */
static void model_20(nflstreet_draw *d,uint32_t v){d->s->model_20=v;for(unsigned i=0;i<4;++i)d->s->matrix_cache[i]=0xfffffffeu;}
int nflstreet_draw_select_model(nflstreet_draw *d,uint32_t view){
 nflstreet_render_state *s=d->s;
 if(!view){s->chunks=s->model_10=s->groups=s->overrides=s->model_24=s->model_28=s->model_30=s->model_04=0;model_20(d,0);s->format=0xffffffffu;}
 else{const uint8_t *v=nflstreet_draw_at(d,view,0x34);if(!v)return 0;
  if(s->chunks!=nflstreet_rd32(v+8)){s->format=0xffffffffu;s->chunks=nflstreet_rd32(v+8);}
  s->groups=nflstreet_rd32(v+0x14);s->overrides=nflstreet_rd32(v+0x1c);s->model_24=nflstreet_rd32(v+0x24);s->model_28=nflstreet_rd32(v+0x28);s->model_30=nflstreet_rd32(v+0x30);
  s->model_04=nflstreet_rd32(v+4);s->model_10=nflstreet_rd32(v+0x10);model_20(d,nflstreet_rd32(v+0x20));}
 s->lod=0;return 1;}
/* 801C7648. */
void nflstreet_draw_reset(nflstreet_draw *d){nflstreet_draw_select_model(d,0);d->s->materials=d->s->override=d->s->instance=d->s->shader=d->s->effects=d->s->blend=0;}
/* 801C79E4: both matrix arrays use the palette's first word. */
int nflstreet_draw_palette(nflstreet_draw *d,uint32_t p){
 d->s->palette=p;if(!p)return 1;uint32_t base;
 return nflstreet_draw_word(d,p,&base)&&NFLSTREET_GX(d,NFLSTREET_GX_SET_ARRAY,0x15,base,0x30)&&NFLSTREET_GX(d,NFLSTREET_GX_SET_ARRAY,0x16,base,0x10);}
/* 801C7DC0 blend presets. */
int nflstreet_draw_blend(nflstreet_draw *d,uint32_t mode){
 static const uint8_t presets[7][3]={{0,0,0},{0,4,5},{1,4,5},{1,0,2},{1,0,4},{1,1,1},{1,4,1}};
 if(mode==d->s->blend)return 1;d->s->blend=mode;
 if(!mode||mode>6)return 1;const uint8_t *p=presets[mode];
 return NFLSTREET_GX(d,NFLSTREET_GX_SET_BLEND_MODE,p[0],p[1],p[2],5);}
/* 801C7EAC. */
int nflstreet_draw_depth_range(nflstreet_draw *d,float near_z,float far_z){
 nflstreet_render_state *s=d->s;s->viewport[4]=near_z;s->viewport[5]=far_z;uint32_t v[6];
 for(unsigned i=0;i<6;++i)v[i]=nflstreet_float_bits(s->viewport[i]);
 return nflstreet_draw_gx(d,NFLSTREET_GX_SET_VIEWPORT,0,NULL,v,6);}

/* 801CACE4: GXSetTevOp-style presets; stage 0 reads the raster color/alpha,
   later stages the previous result. Scale 1 doubles the result. */
static int tev_op(nflstreet_draw *d,uint32_t stage,uint32_t mode,uint32_t scale){
 uint32_t rc=stage?0:10,ra=stage?0:5;int ok=1;
 switch(mode){
 case 0:ok=NFLSTREET_GX(d,NFLSTREET_GX_SET_TEV_COLOR_IN,stage,0xf,8,rc,0xf)&&NFLSTREET_GX(d,NFLSTREET_GX_SET_TEV_ALPHA_IN,stage,7,4,ra,7);break;
 case 1:ok=NFLSTREET_GX(d,NFLSTREET_GX_SET_TEV_COLOR_IN,stage,rc,8,9,0xf)&&NFLSTREET_GX(d,NFLSTREET_GX_SET_TEV_ALPHA_IN,stage,7,7,7,ra);break;
 case 2:ok=NFLSTREET_GX(d,NFLSTREET_GX_SET_TEV_COLOR_IN,stage,rc,0xc,8,0xf)&&NFLSTREET_GX(d,NFLSTREET_GX_SET_TEV_ALPHA_IN,stage,7,4,ra,7);break;
 case 3:ok=NFLSTREET_GX(d,NFLSTREET_GX_SET_TEV_COLOR_IN,stage,0xf,0xf,0xf,8)&&NFLSTREET_GX(d,NFLSTREET_GX_SET_TEV_ALPHA_IN,stage,7,7,7,4);break;
 case 4:ok=NFLSTREET_GX(d,NFLSTREET_GX_SET_TEV_COLOR_IN,stage,0xf,0xf,0xf,rc)&&NFLSTREET_GX(d,NFLSTREET_GX_SET_TEV_ALPHA_IN,stage,7,7,7,ra);break;
 default:break;}
 return ok&&NFLSTREET_GX(d,NFLSTREET_GX_SET_TEV_COLOR_OP,stage,0,0,scale,1,0)&&NFLSTREET_GX(d,NFLSTREET_GX_SET_TEV_ALPHA_OP,stage,0,0,scale,1,0);}
/* Shader setup callbacks 801C9D0C (NULL) and 801C9D5C (Flat): raster color
   only; 801C9DAC (OnePass: texture times raster, doubled); 801C9DFC
   (TwoPass: base, decal overlay by its alpha, then times raster, doubled;
   alpha = constant times raster). */
static int setup(nflstreet_draw *d,unsigned kind){
 switch(kind){
 case 0:case 3:return NFLSTREET_GX(d,NFLSTREET_GX_SET_NUM_TEX_GENS,1)&&NFLSTREET_GX(d,NFLSTREET_GX_SET_NUM_TEV_STAGES,1)&&
  NFLSTREET_GX(d,NFLSTREET_GX_SET_TEV_ORDER,0,0xff,0xff,4)&&tev_op(d,0,4,1);
 case 1:return NFLSTREET_GX(d,NFLSTREET_GX_SET_NUM_TEX_GENS,1)&&NFLSTREET_GX(d,NFLSTREET_GX_SET_NUM_TEV_STAGES,1)&&
  NFLSTREET_GX(d,NFLSTREET_GX_SET_TEV_ORDER,0,0,0,4)&&tev_op(d,0,0,1);
 default:return NFLSTREET_GX(d,NFLSTREET_GX_SET_NUM_TEX_GENS,2)&&NFLSTREET_GX(d,NFLSTREET_GX_SET_NUM_TEV_STAGES,3)&&
  NFLSTREET_GX(d,NFLSTREET_GX_SET_TEV_ORDER,0,0,0,0xff)&&tev_op(d,0,3,0)&&NFLSTREET_GX(d,NFLSTREET_GX_SET_TEV_ORDER,1,1,1,0xff)&&tev_op(d,1,1,0)&&
  NFLSTREET_GX(d,NFLSTREET_GX_SET_TEV_ORDER,2,0xff,0xff,4)&&NFLSTREET_GX(d,NFLSTREET_GX_SET_TEV_COLOR_IN,2,0xf,0,10,0xf)&&
  NFLSTREET_GX(d,NFLSTREET_GX_SET_TEV_COLOR_OP,2,0,0,1,1,0)&&NFLSTREET_GX(d,NFLSTREET_GX_SET_TEV_ALPHA_IN,2,7,6,5,7)&&
  NFLSTREET_GX(d,NFLSTREET_GX_SET_TEV_ALPHA_OP,2,0,0,1,1,0);}}
/* 801C7A3C: shader object {definition, setup, teardown, -, stages +10,
   parameter count +14, parameters +18}; 0 selects the NULL shader. */
int nflstreet_draw_shader(nflstreet_draw *d,uint32_t shader){
 nflstreet_render_state *s=d->s;if(!shader)shader=d->x->null_shader;
 if(shader!=s->shader){uint32_t teardown=0,def,unused,callback;s->override=0;
  if(s->shader&&(!nflstreet_draw_word(d,s->shader+8,&teardown)||teardown))return 0;
  if(!nflstreet_draw_word(d,shader,&def)||!nflstreet_draw_word(d,def+0x14,&unused)||!nflstreet_draw_word(d,shader+4,&callback))return 0;(void)unused;
  if(callback){unsigned k=0;while(k<4&&d->x->setups[k]!=callback)++k;if(k==4||!setup(d,k))return 0;}}
 s->shader=shader;return 1;}
/* 801C7AD4: texture map load; a TLUT object whose first word is 0 is loaded
   and named by the map first. A null texture loads the default texture. */
int nflstreet_draw_texture(nflstreet_draw *d,uint32_t map,uint32_t texture,uint32_t tlut){
 if(!texture)return !d->x->default_texture||nflstreet_draw_texture(d,map,d->x->default_texture,0);
 if(tlut){uint32_t format;if(!nflstreet_draw_word(d,tlut,&format))return 0;
  if(!format&&(!NFLSTREET_GX(d,NFLSTREET_GX_LOAD_TLUT,tlut+4,map)||!NFLSTREET_GX(d,NFLSTREET_GX_INIT_TEX_OBJ_TLUT,texture,map)))return 0;}
 return NFLSTREET_GX(d,NFLSTREET_GX_LOAD_TEX_OBJ,texture,map);}
/* 801C7B58: material instance {-, name, shader +08, stages +0C, stage
   entries +10 (texture, palette, index), parameter values +14}. */
static int apply_instance(nflstreet_draw *d,uint32_t instance){
 nflstreet_render_state *s=d->s;if(instance==s->instance)return 1;
 if(!instance){if(!nflstreet_draw_shader(d,0))return 0;s->instance=0;return 1;}
 uint32_t shader,stages,params,list,entries;
 if(!nflstreet_draw_word(d,instance+8,&shader)||!shader||!nflstreet_draw_shader(d,shader)||s->override)return 0;
 if(!nflstreet_draw_word(d,shader+0x10,&stages)||!nflstreet_draw_word(d,shader+0x14,&params))return 0;
 if((int32_t)stages>0&&!nflstreet_draw_word(d,instance+0x10,&entries))return 0;
 for(int32_t i=0;i<(int32_t)stages;++i){const uint8_t *e=nflstreet_draw_at(d,entries+12u*(uint32_t)i,8);
  if(!e||!nflstreet_draw_texture(d,(uint32_t)i,nflstreet_rd32(e),nflstreet_rd32(e+4)))return 0;}
 if((int32_t)params>0&&!nflstreet_draw_word(d,shader+0x18,&list))return 0;
 for(int32_t i=0;i<(int32_t)params;++i){const uint8_t *r=nflstreet_draw_at(d,list+0x14u*(uint32_t)i,0x14);if(!r)return 0;
  uint32_t value=instance+0x14u+4u*r[0x11],id=nflstreet_rd16(r+0x12),v[4];int ok=1;
  switch(r[0x10]){
  case 2:ok=nflstreet_draw_word(d,value,v)&&nflstreet_draw_gx(d,NFLSTREET_GX_SET_TEV_COLOR,1,(uint32_t[]){id+1},v,1);break;
  case 3:return 0;
  case 4:ok=nflstreet_draw_word(d,value,v)&&nflstreet_draw_gx(d,NFLSTREET_GX_SET_CHAN_MAT_COLOR,1,&id,v,1);break;
  case 5:ok=nflstreet_draw_word(d,value,v)&&nflstreet_draw_gx(d,NFLSTREET_GX_SET_CHAN_AMB_COLOR,1,&id,v,1);break;
  case 6:{const uint8_t *p=nflstreet_draw_at(d,value,16);if(!p)return 0;
   uint32_t m[8]={nflstreet_rd32(p),0,0,nflstreet_rd32(p+8),0,nflstreet_rd32(p+4),0,nflstreet_rd32(p+12)};
   ok=nflstreet_draw_gx(d,NFLSTREET_GX_LOAD_TEX_MTX,2,(uint32_t[]){0x1e,1},m,8)&&NFLSTREET_GX(d,NFLSTREET_GX_SET_TEX_COORD_GEN,id,1,4,0x1e,0,0x7d);break;}
  default:break;}
  if(!ok)return 0;}
 s->instance=instance;return 1;}
/* 801C7D54/801C7498: signed index into the current material array. */
static int material(nflstreet_draw *d,int32_t index){
 uint32_t array=d->s->materials,count,list,instance=0;if(!array)return 1;
 if(!nflstreet_draw_word(d,array,&count))return 0;
 if(index>=0&&index<(int32_t)count&&(!nflstreet_draw_word(d,array+4,&list)||!nflstreet_draw_word(d,list+4u*(uint32_t)index,&instance)))return 0;
 return apply_instance(d,instance);}

/* 801C826C/801CCBD8/801C7F18 for one 0x18-byte chunk {display list, size/32
   +04, material +06, first array +0E, arrays +10, flags +11, format +12,
   LOD count +14} with 12-byte array records {base, -, attribute +06, stride
   +07, count +08, type +09, fraction +0A, descriptor type +0B}. Flags bit 0:
   matrix-index attribute; bit 3: S16 positions are read as F32 (stride 12). */
static int draw_chunk(nflstreet_draw *d,uint32_t chunk){
 nflstreet_render_state *s=d->s;const uint8_t *c=nflstreet_draw_at(d,chunk,0x18);if(!c||!material(d,(int16_t)nflstreet_rd16(c+6)))return 0;
 if(s->effects)return 0;
 const uint8_t *l=c;if(s->lod&&c[0x14]>=s->lod&&!(l=nflstreet_draw_at(d,chunk+0x18u*s->lod,0x18)))return 0;
 uint32_t arrays,n=l[0x10],first=nflstreet_rd16(l+0xe),base[8];const uint8_t *rec[8];uint8_t type[8],stride[8],frac[8];
 if(n>8||!nflstreet_draw_word(d,s->chunks+8,&arrays))return 0;
 for(uint32_t i=0;i<n;++i){
  if(!(rec[i]=nflstreet_draw_at(d,arrays+0xcu*(first+i),0xc)))return 0;type[i]=stride[i]=frac[i]=0xff;base[i]=nflstreet_rd32(rec[i]);
  if(s->overrides){uint32_t o;if(!nflstreet_draw_word(d,s->overrides+4u*(first+i),&o))return 0;if(o)base[i]=o;}}
 if(c[0x11]&8)for(uint32_t i=0;i<n;++i)if(rec[i][6]==9&&rec[i][9]==3){type[i]=4;stride[i]=0xc;}
 if(c[0x12]==s->format){
  for(uint32_t i=0;i<n;++i)if(!NFLSTREET_GX(d,NFLSTREET_GX_SET_ARRAY,rec[i][6],base[i],stride[i]==0xff?rec[i][7]:stride[i]))return 0;}
 else{if(!nflstreet_draw_gx(d,NFLSTREET_GX_CLEAR_VTX_DESC,0,NULL,NULL,0))return 0;
  for(uint32_t i=0;i<n;++i){const uint8_t *r=rec[i];
   if(!NFLSTREET_GX(d,NFLSTREET_GX_SET_VTX_DESC,r[6],r[0xb]&0xfu)||
      !NFLSTREET_GX(d,NFLSTREET_GX_SET_VTX_ATTR_FMT,0,r[6],r[8],type[i]==0xff?r[9]:type[i],frac[i]==0xff?r[10]:frac[i])||
      !NFLSTREET_GX(d,NFLSTREET_GX_SET_ARRAY,r[6],base[i],stride[i]==0xff?r[7]:stride[i]))return 0;}
  if((c[0x11]&1)&&!NFLSTREET_GX(d,NFLSTREET_GX_SET_VTX_DESC,0,1))return 0;
  if(c[0x12])s->format=c[0x12];}
 return NFLSTREET_GX(d,NFLSTREET_GX_CALL_DISPLAY_LIST,nflstreet_rd32(c),(uint32_t)nflstreet_rd16(c+4)<<5)&&(!(c[0x11]&1)||NFLSTREET_GX(d,NFLSTREET_GX_SET_CURRENT_MTX,0));}
/* 801C8460: draw the listed chunks of the current model. */
int nflstreet_draw_list(nflstreet_draw *d,uint32_t list,uint32_t count){
 uint32_t chunks,total;if(!d->s->chunks||!count)return 1;
 if(!nflstreet_draw_word(d,d->s->chunks,&chunks)||!nflstreet_draw_word(d,d->s->chunks+4,&total))return 0;
 for(uint32_t i=0;i<count;++i){const uint8_t *p=nflstreet_draw_at(d,list+2u*i,2);if(!p||nflstreet_rd16(p)>=total||!draw_chunk(d,chunks+0x18u*nflstreet_rd16(p)))return 0;}
 return 1;}
/* 801CD43C: group table {count, groups (0x14: first variant +10, variants
   +12), variants (0x14: first index +10, count +12), -, index list +0C}. */
int nflstreet_draw_variant(nflstreet_draw *d,uint32_t groups,int32_t group,int32_t v,uint32_t *out){
 uint32_t count,list,variants;*out=0;if(!nflstreet_draw_word(d,groups,&count))return 0;if(group>=(int32_t)count)return 1;
 const uint8_t *g;if(!nflstreet_draw_word(d,groups+4,&list)||!(g=nflstreet_draw_at(d,list+0x14u*(uint32_t)group,0x14)))return 0;if(v>=(int32_t)g[0x12])return 1;
 if(!nflstreet_draw_word(d,groups+8,&variants))return 0;*out=variants+0x14u*nflstreet_rd16(g+0x10)+0x14u*(uint32_t)v;return 1;}
/* 801C8544: draw one group variant of the current model. */
int nflstreet_draw_group(nflstreet_draw *d,int32_t group,int32_t v){
 uint32_t groups=d->s->groups,e,list;const uint8_t *p;if(!groups)return 1;
 if(!nflstreet_draw_variant(d,groups,group,v,&e))return 0;if(!e)return 1;
 return (p=nflstreet_draw_at(d,e,0x14))&&nflstreet_draw_word(d,groups+0xc,&list)&&nflstreet_draw_list(d,list+2u*nflstreet_rd16(p+0x10),nflstreet_rd16(p+0x12));}
/* 801C84D4: draw every chunk of the current model. */
int nflstreet_draw_all(nflstreet_draw *d){
 uint32_t chunks,total;if(!d->s->chunks)return 1;
 if(!nflstreet_draw_word(d,d->s->chunks,&chunks)||!nflstreet_draw_word(d,d->s->chunks+4,&total))return 0;
 for(uint32_t i=0;i<total;++i)if(!draw_chunk(d,chunks+0x18u*i))return 0;
 return 1;}
