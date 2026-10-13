#include "slot_materials.h"
#include "face_texture.h"
#include <string.h>
#define NAMES_BYTES (NFLSTREET_GEAR_TEXTURE_SLOTS*NFLSTREET_SLOT_MATERIAL_NAME_BYTES)
#define SETTING_BYTES (NFLSTREET_SLOT_MATERIAL_NAME_BYTES+24u)
static uint32_t rd32(const uint8_t *p){return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static uint16_t rd16(const uint8_t *p){return(uint16_t)((unsigned)p[0]<<8|p[1]);}
static void wr32(uint8_t *p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
static void wr16(uint8_t *p,uint32_t v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
int nflstreet_slot_material_catalog_open(const uint8_t *b,size_t n,nflstreet_slot_material_catalog *out){
 if(!b||!out||n<12u+NAMES_BYTES||memcmp(b,"SMC1",4)||rd32(b+4)!=1)return 0;
 uint32_t count=rd32(b+8+NAMES_BYTES);if(count>64||n<16u+NAMES_BYTES+count*SETTING_BYTES)return 0;
 size_t at=12u+NAMES_BYTES+count*SETTING_BYTES;uint32_t descriptors=rd32(b+at);at+=4;
 if(descriptors>NFLSTREET_ACTOR_MATERIALS_MAX||n-at<descriptors*48u+4u)return 0;
 out->descriptors=b+at;out->descriptor_count=descriptors;at+=descriptors*48u;
 for(uint32_t i=0;i<descriptors*3u;++i)if(!memchr(out->descriptors+16u*i,0,16))return 0;
 if(rd32(b+at)!=2)return 0;at+=4;
 for(unsigned k=0;k<2;++k){if(n-at<20)return 0;uint32_t stages=rd32(b+at+16);if(!stages||stages>8||n-at-20<stages*20u||!memchr(b+at,0,16))return 0;
  out->shaders[k]=b+at;out->shader_stages[k]=stages;at+=20u+stages*20u;}
 if(at!=n)return 0;
 for(unsigned s=0;s<NFLSTREET_GEAR_TEXTURE_SLOTS;++s)if(!memchr(b+8+s*NFLSTREET_SLOT_MATERIAL_NAME_BYTES,0,NFLSTREET_SLOT_MATERIAL_NAME_BYTES))return 0;
 for(unsigned i=0;i<count;++i)if(!memchr(b+12+NAMES_BYTES+i*SETTING_BYTES,0,NFLSTREET_SLOT_MATERIAL_NAME_BYTES))return 0;
 out->names=b+8;out->settings=b+12+NAMES_BYTES;out->setting_count=count;return 1;
}
const char *nflstreet_slot_material_name(const nflstreet_slot_material_catalog *c,unsigned slot){
 return c&&slot<NFLSTREET_GEAR_TEXTURE_SLOTS?(const char*)c->names+slot*NFLSTREET_SLOT_MATERIAL_NAME_BYTES:NULL;}
/* 80205530 GXGetTexBufferSize. */
static uint32_t tex_buffer_size(uint32_t w,uint32_t h,uint32_t fmt,int mipmap,uint32_t levels){
 unsigned xs=0,ys=0;
 switch(fmt){case 0:case 8:case 0xe:case 0x20:case 0x30:xs=3;ys=3;break;
  case 1:case 2:case 9:case 0x11:case 0x22:case 0x27:case 0x28:case 0x29:case 0x2a:case 0x39:case 0x3a:xs=3;ys=2;break;
  case 3:case 4:case 5:case 6:case 10:case 0x13:case 0x16:case 0x23:case 0x2b:case 0x2c:case 0x3c:xs=2;ys=2;break;default:break;}
 uint32_t tile=fmt==6||fmt==0x16?0x40u:0x20u,total=0;w&=0xffffu;h&=0xffffu;
 if(mipmap!=1)return tile*((w+(1u<<xs)-1u)>>xs)*((h+(1u<<ys)-1u)>>ys);
 for(levels&=0xffu;levels;--levels){total+=tile*((w+(1u<<xs)-1u)>>xs)*((h+(1u<<ys)-1u)>>ys);
  if(w==1&&h==1)break;w=w<2?1u:w>>1;h=h<2?1u:h>>1;}
 return total;
}
static unsigned clz32(uint32_t v){unsigned n=32;while(v){--n;v>>=1;}return n;}
/* 80205754 GXInitTexObj. */
static void init_tex_obj(uint8_t *o,uint32_t addr,uint32_t w,uint32_t h,uint32_t fmt,uint32_t ws,uint32_t wt,int mipmap){
 memset(o,0,0x20);uint32_t w0=0,w1=0,w2=0;
 w0=(w0&~3u)|ws;w0=(w0&~0xcu)|wt<<2;w0|=0x10u;
 if(!mipmap)w0=(w0&~0xe0u)|0x80u;
 else{o[0x1f]|=1;w0=(w0&~0xe0u)|(fmt-8u<3u?0xa0u:0xc0u);
  unsigned z=(h&0xffffu)<(w&0xffffu)?clz32(w&0xffffu):clz32(h&0xffffu);
  w1=(((uint32_t)(int)(16.0f*(float)(31-(int)z))&0xffu)<<8)|(w1&0xffff00ffu);}
 wr32(o+0x14,fmt);
 w2=(w2&0xfffffc00u)|((w&0xffffu)-1u);w2=(w2&0xfff003ffu)|(((h&0xffffu)-1u)*0x400u);w2=((fmt&0xfu)<<20)|(w2&0xff0fffffu);
 uint32_t w3=(addr>>5)&0x1ffffffu;unsigned xs,ys;
 switch(fmt&0xfu){case 0:case 8:o[0x1e]=1;xs=3;ys=3;break;case 1:case 2:case 9:o[0x1e]=2;xs=3;ys=2;break;
  case 3:case 4:case 5:case 10:o[0x1e]=2;xs=2;ys=2;break;case 6:o[0x1e]=3;xs=2;ys=2;break;case 0xe:o[0x1e]=0;xs=3;ys=3;break;
  default:o[0x1e]=2;xs=2;ys=2;break;}
 wr32(o,w0);wr32(o+4,w1);wr32(o+8,w2);wr32(o+12,w3);
 wr16(o+0x1c,((((w&0xffffu)+(1u<<xs)-1u)>>xs)*(((h&0xffffu)+(1u<<ys)-1u)>>ys))&0x7fffu);o[0x1f]|=2;
}
/* 80205A10 GXInitTexObjLOD with the r13-6090 min-filter map. */
static void init_tex_lod(uint8_t *o,float min_lod,float max_lod,float bias,unsigned min_filter,unsigned mag_filter,
 unsigned clamp,unsigned edge,unsigned aniso){
 static const uint8_t filters[6]={0,4,1,5,2,6};uint32_t w0=rd32(o),w1=rd32(o+4);
 if(bias<-4.0f)bias=-4.0f;else if(bias>=4.0f)bias=3.99f;
 w0=(((uint32_t)(int)(32.0f*bias)&0xffu)<<9)|(w0&0xfffe01ffu);w0=(w0&~0x10u)|(uint32_t)(mag_filter==1)<<4;
 w0=(w0&~0xe0u)|(uint32_t)filters[min_filter]<<5;w0=(w0&~0x100u)|(uint32_t)(edge==0)<<8;w0&=~0x60000u;
 w0=(w0&0xffe7ffffu)|aniso<<19;w0=(w0&0xffdfffffu)|(clamp&0xffu)<<21;
 if(min_lod<0.0f)min_lod=0.0f;else if(min_lod>10.0f)min_lod=10.0f;
 if(max_lod<0.0f)max_lod=0.0f;else if(max_lod>10.0f)max_lod=10.0f;
 w1=((uint32_t)(int)(16.0f*min_lod)&0xffu)|(w1&0xffffff00u);w1=(((uint32_t)(int)(16.0f*max_lod)&0xffu)<<8)|(w1&0xffff00ffu);
 wr32(o,w0);wr32(o+4,w1);
}
/* 801C926C: max LOD = mips-1 (+20), CI formats map min filters 4/5 to 2/3. */
static void apply_lod(uint8_t *o,float bias){unsigned min=5;
 float max=(float)((int)rd16(o+0x20)-1);uint32_t fmt=rd32(o+0x14);if(fmt-8u<2u)min=3;
 init_tex_lod(o,0.0f,max,bias,min,1,0,0,0);}
static int at(const uint8_t *m,size_t n,uint32_t token,uint32_t p,size_t len,const uint8_t **q){
 if(p<token||p-token>n||len>n-(p-token))return 0;*q=m+(p-token);return 1;}
static int allocate(nflstreet_material_allocate a,void *o,uint32_t count,uint32_t each,nflstreet_material_block *b){
 memset(b,0,sizeof *b);if(!count)return 1;b->size=count*each;return a(o,b->size,&b->token,&b->bytes)&&b->token&&b->bytes;}
int nflstreet_texture_set_build(uint8_t *m,size_t n,uint32_t token,nflstreet_material_allocate alloc,void *ao,
 nflstreet_texture_flush flush,void *fo,nflstreet_texture_set *out){
 nflstreet_texture_set t;memset(&t,0,sizeof t);if(!alloc||!flush||!out||(token&&(!m||n<0x24)))return 0;
 t.object.size=0x28;if(!alloc(ao,0x28,&t.object.token,&t.object.bytes)||!t.object.token||!t.object.bytes)return 0;
 uint32_t images=0,palettes=0;
 if(token){if(!nflstreet_mmap_relocate(m,n,token)||rd16(m+4)!=2)return 0;images=rd16(m+0xc);palettes=rd16(m+0x10);}
 uint8_t *o=t.object.bytes;
 wr32(o+0x20,images);if(!allocate(alloc,ao,images,0x10,&t.entries))return 0;wr32(o+0x24,t.entries.token);
 wr32(o+0x10,images);if(!allocate(alloc,ao,images,0x2c,&t.textures))return 0;wr32(o+0x14,t.textures.token);
 wr32(o+0x18,palettes);if(!allocate(alloc,ao,palettes,0x28,&t.palettes))return 0;wr32(o+0x1c,t.palettes.token);
 wr32(o,images);if(!allocate(alloc,ao,images,8,&t.names))return 0;wr32(o+8,t.names.token);o[0xc]=0;o[0xd]=0;
 if(t.names.bytes)memset(t.names.bytes,0,t.names.size);wr32(o+4,images);
 const uint8_t *p;
 for(uint32_t i=0;i<palettes;++i){
  if(!at(m,n,token,rd32(m+0x1c)+i*0xcu,0xc,&p))return 0;
  uint32_t fmt=rd16(p+2),size=rd32(p+4),data=rd32(p+8),entries=size>0x7fu?0x100u:0x10u;uint8_t *q=t.palettes.bytes+i*0x28u;
  if(!flush(fo,data,entries<<2))return 0;
  if(fmt==6){wr32(q,1);init_tex_obj(q+4,data,16,16,6,0,0,0);}
  else{wr32(q,0);uint32_t w1=rd32(q+8);wr32(q+4,fmt<<10);w1=(w1&0xffe00000u)|((data>>5)&0x1ffffffu);wr32(q+8,(w1&0xffffffu)|0x64000000u);wr16(q+12,entries);}
  wr32(q+0x24,size);
 }
 for(uint32_t i=0;i<images;++i){const uint8_t *r;uint8_t *e=t.entries.bytes+i*0x10u;
  if(!at(m,n,token,rd32(m+0x14)+i*0xcu,0xc,&r))return 0;uint32_t name=rd16(m+4)?rd32(m+0x20)+i*0x10u:0;
  wr32(e,name);wr16(e+8,i);wr32(e+4,t.object.token);wr16(e+10,rd16(r+8));wr16(e+14,0xffff);wr16(e+12,rd16(r));
  wr32(t.names.bytes+i*8u,name);wr32(t.names.bytes+i*8u+4,t.entries.token+i*0x10u);}
 for(uint32_t i=0;i<images;++i){const uint8_t *r,*h;uint8_t *x=t.textures.bytes+i*0x2cu;
  if(!at(m,n,token,rd32(m+0x14)+i*0xcu,0xc,&r)||!at(m,n,token,rd32(m+0x18)+rd16(r+4)*0x10u,0x10,&h))return 0;
  uint32_t w=rd16(h),ht=rd16(h+2),fmt=rd16(h+4),size=rd32(h+8),data=rd32(h+12);int32_t mips=(int16_t)rd16(r+2);
  if(!w||!ht)return 0;int mipmap=mips>1;
  wr16(x+0x24,w);wr32(x+0x28,size);wr16(x+0x20,(uint32_t)mips);wr16(x+0x26,ht); /* 801C9464 */
  if(!flush(fo,data,tex_buffer_size(w,ht,fmt,mipmap,(uint32_t)mips)))return 0;
  init_tex_obj(x,data,w,ht,fmt,1,1,mipmap);if(fmt-8u<2u){x[0x1f]&=0xfdu;wr32(x+0x18,0);}
  apply_lod(x,0.0f);
  apply_lod(x,0.0f); /* 801C93D4 */
  uint32_t w0=rd32(x);w0=(w0&~3u)|(uint32_t)((w&(w-1u))==0);w0=(w0&~0xcu)|(uint32_t)((ht&(ht-1u))==0)<<2;wr32(x,w0);
  wr16(x+0x26,ht);wr32(x+0x28,size);wr16(x+0x24,w);}
 *out=t;return 1;
}
int nflstreet_slot_materials_bind(const nflstreet_slot_material_catalog *c,nflstreet_slot_textures *s,
 nflstreet_material_allocate alloc,void *ao,nflstreet_texture_flush flush,void *fo,nflstreet_slot_material out[NFLSTREET_GEAR_TEXTURE_SLOTS]){
 if(!c||!s||!s->block||!out)return 0;
 for(unsigned i=0;i<NFLSTREET_GEAR_TEXTURE_SLOTS;++i){nflstreet_slot_material *b=out+i;const nflstreet_slot_texture *t=s->slots+i;
  memset(b,0,sizeof *b);b->material=nflstreet_slot_material_name(c,i);b->destination=t->token;
  if(!nflstreet_texture_set_build(s->block+t->offset,t->template_size,t->token,alloc,ao,flush,fo,&b->texture_set))return 0;
  b->set=b->texture_set.object.token;b->entries=rd32(b->texture_set.object.bytes+0x24);}
 return 1;
}
int nflstreet_slot_materials_configure(const nflstreet_slot_material_catalog *c,nflstreet_slot_material m[NFLSTREET_GEAR_TEXTURE_SLOTS]){
 if(!c||!m)return 0;
 for(unsigned i=0;i<c->setting_count;++i){const uint8_t *e=c->settings+i*SETTING_BYTES,*w=e+NFLSTREET_SLOT_MATERIAL_NAME_BYTES;unsigned s=0;
  while(s<NFLSTREET_GEAR_TEXTURE_SLOTS&&(!m[s].material||strcmp(m[s].material,(const char*)e)))++s;
  if(s==NFLSTREET_GEAR_TEXTURE_SLOTS)return 0;
  const nflstreet_material_block *t=&m[s].texture_set.textures;uint32_t count=rd32(m[s].texture_set.object.bytes+0x10);
  if(count*0x2cu>t->size)return 0;
  if(w[0])for(uint32_t k=0;k<count;++k){uint8_t *x=t->bytes+k*0x2cu;wr32(x,rd32(x)&~0xfu);} /* 801C9208 */
  uint32_t bits=rd32(w+12);float bias;memcpy(&bias,&bits,4);uint32_t min=rd32(w+4),mag=rd32(w+8),aniso=rd32(w+16);
  if(min>5)return 0;
  for(uint32_t k=0;k<count;++k){uint8_t *x=t->bytes+k*0x2cu;unsigned f=min;
   float max=(float)((int)rd16(x+0x20)-1);uint32_t fmt=rd32(x+0x14),a=aniso;
   if(fmt-8u<2u){if(f==4)f=2;else if(f==5)f=3;a=0;}
   init_tex_lod(x,0.0f,max,bias,f,mag,w[20],w[21],a);}}
 return 1;
}
/* 801C6DC0/801C9C64: index of the type-1 stage with this name, or -1. */
static int stage_index(const nflstreet_slot_material_catalog *c,unsigned shader,const char *name,uint32_t *index){
 for(unsigned i=0;i<c->shader_stages[shader];++i){const uint8_t *r=c->shaders[shader]+20u+i*20u;
  if(r[16]==1&&!strncmp((const char*)r,name,16)){*index=rd16(r+18);return 1;}}
 return 0;
}
/* 801C9088/801C90AC on a slot's texture-set entries (binding entry +14). */
static void slot_stage(const nflstreet_slot_material *b,uint32_t *texture,uint32_t *palette){
 const nflstreet_texture_set *t=&b->texture_set;*texture=*palette=0;if(!b->entries||!t->entries.bytes)return;
 const uint8_t *e=t->entries.bytes;*texture=t->textures.token+(uint32_t)(int16_t)rd16(e+8)*0x2cu;
 if((int16_t)rd16(e+12)>0)*palette=t->palettes.token+(uint32_t)(int16_t)rd16(e+10)*0x28u;
}
static int set_stage(const nflstreet_slot_material_catalog *c,nflstreet_actor_material *m,const char *name,uint32_t texture,uint32_t palette){
 uint32_t i;if(!stage_index(c,m->shader,name,&i)||i>=m->stage_count||i>=2)return 0;m->stages[i].texture=texture;m->stages[i].palette=palette;return 1;}
static void use_shader(const nflstreet_slot_material_catalog *c,nflstreet_actor_material *m,unsigned shader){ /* 801C7150/801C6E28 */
 unsigned stages=c->shader_stages[shader];m->shader=shader;if(stages==m->stage_count)return;
 nflstreet_material_stage kept[2]={{0,0,0},{0,0,0}};for(unsigned i=0;i<stages&&i<m->stage_count&&i<2;++i)kept[i]=m->stages[i];
 memcpy(m->stages,kept,sizeof kept);m->stage_count=stages;
}
int nflstreet_actor_materials_hook(const nflstreet_slot_material_catalog *c,const nflstreet_slot_material b[NFLSTREET_GEAR_TEXTURE_SLOTS],
 nflstreet_actor_material out[NFLSTREET_ACTOR_MATERIALS_MAX],unsigned *count){
 static const uint8_t tattoo_materials[8]={10,11,0x13,0x14,0xe,0xf,0x17,0x18},tattoo_slots[8]={19,19,20,20,21,21,22,22};
 if(!c||!b||!out||!count||c->shader_stages[0]>2||c->shader_stages[1]>2)return 0;
 for(unsigned i=0;i<c->descriptor_count;++i){nflstreet_actor_material *m=out+i;const char *texture=(const char*)c->descriptors+48u*i+32u;unsigned s=0;
  memset(m,0,sizeof *m);m->material=(const char*)c->descriptors+48u*i;m->texture=texture;
  while(s<NFLSTREET_GEAR_TEXTURE_SLOTS&&(!b[s].material||strcmp(b[s].material,texture)))++s;
  if(s==NFLSTREET_GEAR_TEXTURE_SLOTS)return 0;
  m->shader=0;m->stage_count=c->shader_stages[0];for(unsigned k=0;k<m->stage_count;++k)m->stages[k].index=UINT32_MAX;
  uint32_t tex,pal;slot_stage(b+s,&tex,&pal);if(!set_stage(c,m,"base",tex,pal))return 0;use_shader(c,m,0);}
 for(unsigned k=0;k<8;++k){if(tattoo_materials[k]>=c->descriptor_count)return 0;nflstreet_actor_material *m=out+tattoo_materials[k];uint32_t tex,pal;
  slot_stage(b+tattoo_slots[k],&tex,&pal);if(!tex){use_shader(c,m,0);continue;}
  use_shader(c,m,1);if(!set_stage(c,m,"over",tex,pal))return 0;}
 *count=c->descriptor_count;return 1;
}
