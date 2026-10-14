#include "gx_aurora.h"
#include <dolphin/gx.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Vertex arrays seen so far: the largest size their display lists needed. */
#define ARRAY_SIZES 4096
typedef struct {uint32_t base,size;uint8_t attr;} array_size;
struct nflstreet_gx_aurora {
 uint8_t *ram;uint32_t ram_bytes;unsigned calls;
 uint8_t desc[26],cnt[8][26],type[8][26],frac[8][26];
 struct {uint32_t base,stride,set_size;int dirty;} arrays[26];
 array_size sizes[ARRAY_SIZES];unsigned size_count;
 struct {uint32_t texture,name;} names[512];unsigned name_count;
 GXTexObj textures[8];GXTlutObj tluts[32];
 /* The open immediate primitive, buffered until complete so its indexed
    arrays can be sized from its indices. */
 int in_begin;uint32_t begin_left,begin_args[3];
 struct {uint8_t size;uint32_t value;} *writes;uint32_t write_count,write_cap;
 uint8_t *bytes;uint32_t byte_count,byte_cap;
 /* View delta and this frame's transformed palette copies. */
 int delta_on;float delta[12];
 struct {uint32_t base,size;uint8_t *copy;} palettes[1024];unsigned palette_count;
};
static uint32_t be32(const uint8_t *p){return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static uint16_t be16(const uint8_t *p){return(uint16_t)(p[0]<<8|p[1]);}
static float word_float(uint32_t v){float f;memcpy(&f,&v,4);return f;}
static uint8_t *host(nflstreet_gx_aurora *b,uint32_t address,uint32_t size){
 uint32_t off=address&0x01ffffffu;if((uint64_t)off+size>b->ram_bytes)return NULL;return b->ram+off;}
static GXColor color(uint32_t v){GXColor c={(u8)(v>>24),(u8)(v>>16),(u8)(v>>8),(u8)v};return c;}

nflstreet_gx_aurora *nflstreet_gx_aurora_open(uint8_t *ram,uint32_t ram_bytes){
 nflstreet_gx_aurora *b=calloc(1,sizeof *b);if(!b)return NULL;b->ram=ram;b->ram_bytes=ram_bytes;return b;}
void nflstreet_gx_aurora_new_frame(nflstreet_gx_aurora *b){
 for(unsigned i=0;i<b->palette_count;++i)free(b->palettes[i].copy);b->palette_count=0;
 for(unsigned a=21;a<=22;++a)b->arrays[a].dirty=1;}
void nflstreet_gx_aurora_close(nflstreet_gx_aurora *b){if(b){nflstreet_gx_aurora_new_frame(b);free(b->writes);free(b->bytes);}free(b);}
void nflstreet_gx_aurora_set_view_delta(nflstreet_gx_aurora *b,const float *delta){
 b->delta_on=delta!=NULL;if(delta)memcpy(b->delta,delta,sizeof b->delta);
 for(unsigned a=21;a<=22;++a)b->arrays[a].dirty=1;}
/* r = D x m (3x4 affine); normals use only the rotation part. */
static void delta_mul(const float d[12],const float m[12],float r[12]){
 for(unsigned i=0;i<3;++i)for(unsigned j=0;j<4;++j)r[4*i+j]=d[4*i]*m[j]+d[4*i+1]*m[4+j]+d[4*i+2]*m[8+j]+(j==3?d[4*i+3]:0.0f);}
static uint32_t float_word(float f){uint32_t v;memcpy(&v,&f,4);return v;}
/* A transformed copy of a palette (3x4 big-endian matrices) for this frame. */
static void *palette_copy(nflstreet_gx_aurora *b,uint32_t base,uint32_t size){
 size=(size+0x2fu)/0x30u*0x30u;
 for(unsigned i=0;i<b->palette_count;++i)if(b->palettes[i].base==base&&b->palettes[i].size>=size)return b->palettes[i].copy;
 const uint8_t *src=host(b,base,size);if(!src||b->palette_count>=1024)return NULL;uint8_t *copy=malloc(size);if(!copy)return NULL;
 for(uint32_t o=0;o<size;o+=0x30){float m[12],r[12];for(unsigned k=0;k<12;++k)m[k]=word_float(be32(src+o+4*k));delta_mul(b->delta,m,r);
  for(unsigned k=0;k<12;++k){uint32_t w=float_word(r[k]);copy[o+4*k]=(uint8_t)(w>>24);copy[o+4*k+1]=(uint8_t)(w>>16);copy[o+4*k+2]=(uint8_t)(w>>8);copy[o+4*k+3]=(uint8_t)w;}}
 b->palettes[b->palette_count].base=base;b->palettes[b->palette_count].size=size;b->palettes[b->palette_count++].copy=copy;return copy;}
unsigned nflstreet_gx_aurora_calls(const nflstreet_gx_aurora *b){return b?b->calls:0;}
static int set_arrays(nflstreet_gx_aurora *b,const uint32_t max_index[26],const int used[26]);
/* Submits the buffered primitive: arrays sized from its indices, then
   GXBegin, the writes as recorded, GXEnd. */
static int submit_primitive(nflstreet_gx_aurora *b){
 uint32_t max_index[26];memset(max_index,0,sizeof max_index);int used[26]={0};
 const unsigned f=b->begin_args[1]&7;const uint32_t n=b->begin_args[2]&0xffff;uint32_t i=0;
 for(uint32_t v=0;v<n;++v)for(unsigned a=0;a<=20;++a){unsigned d=b->desc[a];
  if(d==1)i+=direct_bytes(b,f,a);
  else if(d>=2){if(i+(d==2?1u:2u)>b->byte_count)return 0;uint32_t x=d==2?b->bytes[i]:be16(b->bytes+i);i+=d==2?1u:2u;
   if(x>max_index[a])max_index[a]=x;used[a]=1;}}
 if(!set_arrays(b,max_index,used))return 0;
 GXBegin((GXPrimitive)b->begin_args[0],(GXVtxFmt)f,(u16)n);
 for(uint32_t k=0;k<b->write_count;++k){const uint32_t v=b->writes[k].value;
  if(b->writes[k].size==1)GXCmd1u8((u8)v);else if(b->writes[k].size==2)GXCmd1u16((u16)v);else GXCmd1u32(v);}
 GXEnd();b->in_begin=0;return 1;}
void nflstreet_gx_aurora_flush(nflstreet_gx_aurora *b){if(b->in_begin)submit_primitive(b);}

/* Bytes one vertex attribute occupies in format f (direct data). */
static uint32_t direct_bytes(const nflstreet_gx_aurora *b,unsigned f,unsigned a){
 static const uint8_t comp[5]={1,1,2,2,4},color_bytes[6]={2,3,4,2,3,4};
 if(a<9)return 1;
 if(a==9)return (b->cnt[f][a]?3u:2u)*comp[b->type[f][a]<5?b->type[f][a]:4];
 if(a==10||a==25)return (b->cnt[f][10]?9u:3u)*comp[b->type[f][10]<5?b->type[f][10]:4];
 if(a==11||a==12)return color_bytes[b->type[f][a]<6?b->type[f][a]:5];
 if(a>=13&&a<=20)return (b->cnt[f][a]?2u:1u)*comp[b->type[f][a]<5?b->type[f][a]:4];
 return 0;}
static uint32_t vertex_bytes(const nflstreet_gx_aurora *b,unsigned f){
 uint32_t n=0;for(unsigned a=0;a<=20;++a){unsigned d=b->desc[a];n+=d==0?0:d==1?direct_bytes(b,f,a):d==2?1u:2u;}return n;}
/* Arrays: size each indexed attribute from the display list's indices. */
static array_size *size_entry(nflstreet_gx_aurora *b,uint8_t attr,uint32_t base){
 for(unsigned i=0;i<b->size_count;++i)if(b->sizes[i].attr==attr&&b->sizes[i].base==base)return b->sizes+i;
 if(b->size_count>=ARRAY_SIZES)return NULL;array_size *e=b->sizes+b->size_count++;e->attr=attr;e->base=base;e->size=0;return e;}
/* CP register writes inside display lists: vertex descriptor (0x50 low:
   matrix indices, position, normal, colors; 0x60 high: texture coordinates)
   and attribute formats (0x70/0x80/0x90 + format: VAT A/B/C). */
static void set_fmt(nflstreet_gx_aurora *b,unsigned f,unsigned a,uint32_t cnt,uint32_t type,uint32_t frac){
 b->cnt[f][a]=(uint8_t)cnt;b->type[f][a]=(uint8_t)type;b->frac[f][a]=(uint8_t)frac;}
static void cp_write(nflstreet_gx_aurora *b,uint8_t reg,uint32_t v){
 const unsigned f=reg&7;
 if(reg==0x50){b->desc[0]=(uint8_t)(v&1);for(unsigned k=0;k<8;++k)b->desc[1+k]=(uint8_t)(v>>(1+k)&1);
  b->desc[9]=(uint8_t)(v>>9&3);b->desc[10]=(uint8_t)(v>>11&3);b->desc[11]=(uint8_t)(v>>13&3);b->desc[12]=(uint8_t)(v>>15&3);}
 else if(reg==0x60){for(unsigned k=0;k<8;++k)b->desc[13+k]=(uint8_t)(v>>(2*k)&3);}
 else if((reg&0xf8)==0x70){set_fmt(b,f,9,v&1,v>>1&7,v>>4&31);set_fmt(b,f,10,v>>9&1,v>>10&7,0);set_fmt(b,f,11,v>>13&1,v>>14&7,0);
  set_fmt(b,f,12,v>>17&1,v>>18&7,0);set_fmt(b,f,13,v>>21&1,v>>22&7,v>>25&31);}
 else if((reg&0xf8)==0x80){set_fmt(b,f,14,v&1,v>>1&7,v>>4&31);set_fmt(b,f,15,v>>9&1,v>>10&7,v>>13&31);set_fmt(b,f,16,v>>18&1,v>>19&7,v>>22&31);
  set_fmt(b,f,17,v>>27&1,v>>28&7,b->frac[f][17]);}
 else if((reg&0xf8)==0x90){b->frac[f][17]=(uint8_t)(v&31);set_fmt(b,f,18,v>>5&1,v>>6&7,v>>9&31);set_fmt(b,f,19,v>>14&1,v>>15&7,v>>18&31);
  set_fmt(b,f,20,v>>23&1,v>>24&7,v>>27&31);}}
static int scan_list(nflstreet_gx_aurora *b,const uint8_t *p,uint32_t n,uint32_t max_index[26],int used[26]){
 uint32_t i=0;
 while(i<n){uint8_t op=p[i];
  if(op==0x00){++i;continue;}
  if(op==0x08){if(i+6>n)return 0;cp_write(b,p[i+1],be32(p+i+2));i+=6;continue;}
  if(op==0x10){if(i+5>n)return 0;i+=5+4u*((uint32_t)be16(p+i+1)+1);continue;}
  if((op&0xe7)==0x20){ /* indexed XF load from matrix array 21 + (op>>3&3): index, then length-1 in the top nibble */
   if(i+5>n)return 0;const unsigned array=21u+(op>>3&3u);const uint32_t index=be16(p+i+1),words=(uint32_t)(be16(p+i+3)>>12)+1;
   const uint32_t need=index*b->arrays[array].stride+4u*words;if(need>max_index[array])max_index[array]=need;i+=5;continue;}
  if(op==0x48){++i;continue;}
  if(op==0x61){i+=5;continue;}
  if(op>=0x80){unsigned f=op&7;if(i+3>n)return 0;uint32_t count=be16(p+i+1);i+=3;
   for(uint32_t v=0;v<count;++v)for(unsigned a=0;a<=20;++a){unsigned d=b->desc[a];
    if(d==1)i+=direct_bytes(b,f,a);
    else if(d>=2){if(i+(d==2?1u:2u)>n)return 0;uint32_t x=d==2?p[i]:be16(p+i);i+=d==2?1u:2u;if(x>max_index[a])max_index[a]=x;used[a]=1;}}
   continue;}
  return 1;} /* unknown command: stop scanning */
 return 1;}
/* Vertex arrays 9-20: sized to the highest index used (the largest seen per
   array so far), re-set when the base changed or more is needed. */
static int set_arrays(nflstreet_gx_aurora *b,const uint32_t max_index[26],const int used[26]){
 for(unsigned a=9;a<=20;++a){if(!used[a])continue;
  uint32_t base=b->arrays[a].base,stride=b->arrays[a].stride,elem=direct_bytes(b,0,a);
  /* The element size uses format 0's layout; use the stride when larger. */
  if(stride>elem)elem=stride;
  uint32_t need=max_index[a]*stride+elem;array_size *e=size_entry(b,(uint8_t)a,base);if(!e)return 0;if(need>e->size)e->size=need;
  if(b->arrays[a].dirty||e->size>b->arrays[a].set_size){void *data=host(b,base,e->size);if(!data)return 0;
   GXSetArray((GXAttr)a,data,e->size,(u8)stride,false);b->arrays[a].set_size=e->size;b->arrays[a].dirty=0;}}
 return 1;}
static int call_list(nflstreet_gx_aurora *b,uint32_t list,uint32_t bytes){
 const uint8_t *p=host(b,list,bytes);if(!p)return 0;
 uint32_t max_index[26];memset(max_index,0,sizeof max_index);int used[26]={0};
 /* The scan follows the list's own descriptor and format writes, which
    persist after it as they do in Aurora; arrays are set before the call
    for every attribute the list indexes. */
 if(!scan_list(b,p,bytes,max_index,used))return 0;
 if(!set_arrays(b,max_index,used))return 0;
 /* Matrix arrays: max_index holds the bytes needed. */
 for(unsigned a=21;a<=24;++a){if(!max_index[a])continue;const uint32_t base=b->arrays[a].base,stride=b->arrays[a].stride;
  array_size *e=size_entry(b,(uint8_t)a,base);if(!e)return 0;if(max_index[a]>e->size)e->size=max_index[a];
  if(b->arrays[a].dirty||e->size>b->arrays[a].set_size){void *data=b->delta_on?palette_copy(b,base,e->size):host(b,base,e->size);if(!data)return 0;
   GXSetArray((GXAttr)a,data,b->delta_on?(e->size+0x2fu)/0x30u*0x30u:e->size,(u8)stride,false);b->arrays[a].set_size=e->size;b->arrays[a].dirty=0;}}
 GXCallDisplayList(p,bytes);return 1;}
/* SDK GXTexObj words: mode0 (wrap 0-3, mag 4, min 5-7, no edge LOD 8, bias
   9-16, anisotropy 19-20, bias clamp 21),
   mode1 (min/max LOD x16), image0 (width-1, height-1, format), image3
   (address >> 5), format +14, TLUT name +18, flags +1F (bit 0 mipmap). */
static int load_texture(nflstreet_gx_aurora *b,uint32_t token,uint32_t map){
 const uint8_t *t=host(b,token,0x20);if(!t||map>7)return 0;
 static const uint8_t filter_of[8]={0,2,4,0,1,3,5,0}; /* register encoding to GXTexFilter */
 uint32_t w0=be32(t),w1=be32(t+4),w2=be32(t+8),image=(be32(t+12)&0x1fffffu)<<5,format=be32(t+0x14),name=be32(t+0x18);
 for(unsigned i=0;i<b->name_count;++i)if(b->names[i].texture==token)name=b->names[i].name;
 uint16_t w=(uint16_t)((w2&0x3ff)+1),h=(uint16_t)((w2>>10&0x3ff)+1);const int mip=t[0x1f]&1;
 void *data=host(b,0x80000000u|image,1);if(!data)return 0;GXTexObj *o=&b->textures[map];
 if(format>=8&&format<=10)GXInitTexObjCI(o,data,w,h,(GXCITexFmt)format,(GXTexWrapMode)(w0&3),(GXTexWrapMode)(w0>>2&3),mip?GX_TRUE:GX_FALSE,name);
 else GXInitTexObj(o,data,w,h,(GXTexFmt)format,(GXTexWrapMode)(w0&3),(GXTexWrapMode)(w0>>2&3),mip?GX_TRUE:GX_FALSE);
 GXInitTexObjLOD(o,(GXTexFilter)filter_of[w0>>5&7],(GXTexFilter)(w0>>4&1),(float)(w1&0xff)/16.0f,(float)(w1>>8&0xff)/16.0f,
  (float)(int8_t)(w0>>9&0xff)/32.0f,(w0>>21&1)?GX_TRUE:GX_FALSE,(w0>>8&1)?GX_FALSE:GX_TRUE,(GXAnisotropy)(w0>>19&3));
 GXLoadTexObj(o,(GXTexMapID)map);return 1;}
/* SDK GXTlutObj: format (bits 10-11), address (>> 5), entries +8. */
static int load_tlut(nflstreet_gx_aurora *b,uint32_t token,uint32_t name){
 const uint8_t *t=host(b,token,12);if(!t||name>=32)return 0;
 uint16_t entries=be16(t+8);void *data=host(b,0x80000000u|(be32(t+4)&0x1fffffu)<<5,2u*entries);if(!data)return 0;
 GXInitTlutObj(&b->tluts[name],data,(GXTlutFmt)(be32(t)>>10&3),entries);GXLoadTlut(&b->tluts[name],name);return 1;}
/* SDK GXLightObj: color +0C, attenuation +10..+24, position +28, direction +34. */
static void load_light(uint32_t mask,const uint32_t *w){
 GXLightObj l;memset(&l,0,sizeof l);GXInitLightColor(&l,color(w[3]));
 GXInitLightAttn(&l,word_float(w[4]),word_float(w[5]),word_float(w[6]),word_float(w[7]),word_float(w[8]),word_float(w[9]));
 GXInitLightPos(&l,word_float(w[10]),word_float(w[11]),word_float(w[12]));GXInitLightDir(&l,word_float(w[13]),word_float(w[14]),word_float(w[15]));
 GXLoadLightObjImm(&l,(GXLightID)mask);}
FILE *nflstreet_gx_aurora_trace; /* optional: every call before it is forwarded */
int nflstreet_gx_aurora_emit(void *sink,const nflstreet_gx_call *c){
 nflstreet_gx_aurora *b=sink;const uint32_t *a=c->arg;++b->calls;
 if(nflstreet_gx_aurora_trace&&c->op!=NFLSTREET_GX_WRITE){fprintf(nflstreet_gx_aurora_trace,"%u op %d %x %x %x %x %x %x %x\n",b->calls,(int)c->op,a[0],a[1],a[2],a[3],a[4],a[5],a[6]);
  fflush(nflstreet_gx_aurora_trace);}
 if(c->op==NFLSTREET_GX_WRITE){if(!b->in_begin||(a[0]!=1&&a[0]!=2&&a[0]!=4))return 0;
  if(b->write_count==b->write_cap){uint32_t cap=b->write_cap?2*b->write_cap:1024;void *w=realloc(b->writes,cap*sizeof *b->writes);if(!w)return 0;
   b->writes=w;b->write_cap=cap;}
  if(b->byte_count+4>b->byte_cap){uint32_t cap=b->byte_cap?2*b->byte_cap:4096;uint8_t *w=realloc(b->bytes,cap);if(!w)return 0;b->bytes=w;b->byte_cap=cap;}
  b->writes[b->write_count].size=(uint8_t)a[0];b->writes[b->write_count++].value=a[1];
  for(uint32_t k=0;k<a[0];++k)b->bytes[b->byte_count++]=(uint8_t)(a[1]>>(8*(a[0]-1-k)));
  if(b->begin_left<=a[0]){b->begin_left=0;return submit_primitive(b);}
  b->begin_left-=a[0];return 1;}
 nflstreet_gx_aurora_flush(b);
 switch(c->op){
 case NFLSTREET_GX_SET_VTX_DESC:if(a[0]>25)return 0;b->desc[a[0]]=(uint8_t)a[1];GXSetVtxDesc((GXAttr)a[0],(GXAttrType)a[1]);break;
 case NFLSTREET_GX_CLEAR_VTX_DESC:memset(b->desc,0,sizeof b->desc);GXClearVtxDesc();break;
 case NFLSTREET_GX_SET_VTX_ATTR_FMT:if(a[0]>7||a[1]>25)return 0;b->cnt[a[0]][a[1]]=(uint8_t)a[2];b->type[a[0]][a[1]]=(uint8_t)a[3];b->frac[a[0]][a[1]]=(uint8_t)a[4];
  GXSetVtxAttrFmt((GXVtxFmt)a[0],(GXAttr)a[1],(GXCompCnt)a[2],(GXCompType)a[3],(u8)a[4]);break;
 case NFLSTREET_GX_SET_ARRAY:if(a[0]>25)return 0;b->arrays[a[0]].base=a[1];b->arrays[a[0]].stride=a[2];b->arrays[a[0]].dirty=1;break;
 case NFLSTREET_GX_SET_TEX_COORD_GEN:GXSetTexCoordGen2((GXTexCoordID)a[0],(GXTexGenType)a[1],(GXTexGenSrc)a[2],a[3],(GXBool)a[4],a[5]);break;
 case NFLSTREET_GX_SET_NUM_TEX_GENS:GXSetNumTexGens((u8)a[0]);break;
 case NFLSTREET_GX_SET_CULL_MODE:GXSetCullMode((GXCullMode)a[0]);break;
 case NFLSTREET_GX_LOAD_LIGHT:{uint32_t w[16];memcpy(w,c->data,sizeof w);
  if(b->delta_on){const float *d=b->delta,p[3]={word_float(w[10]),word_float(w[11]),word_float(w[12])},n[3]={word_float(w[13]),word_float(w[14]),word_float(w[15])};
   for(unsigned i=0;i<3;++i){w[10+i]=float_word(d[4*i]*p[0]+d[4*i+1]*p[1]+d[4*i+2]*p[2]+d[4*i+3]);w[13+i]=float_word(d[4*i]*n[0]+d[4*i+1]*n[1]+d[4*i+2]*n[2]);}}
  load_light(a[0],w);break;}
 case NFLSTREET_GX_SET_CHAN_AMB_COLOR:GXSetChanAmbColor((GXChannelID)a[0],color(c->data[0]));break;
 case NFLSTREET_GX_SET_CHAN_MAT_COLOR:GXSetChanMatColor((GXChannelID)a[0],color(c->data[0]));break;
 case NFLSTREET_GX_SET_NUM_CHANS:GXSetNumChans((u8)a[0]);break;
 case NFLSTREET_GX_SET_CHAN_CTRL:GXSetChanCtrl((GXChannelID)a[0],(GXBool)a[1],(GXColorSrc)a[2],(GXColorSrc)a[3],a[4],(GXDiffuseFn)a[5],(GXAttnFn)a[6]);break;
 case NFLSTREET_GX_INIT_TEX_OBJ_TLUT:{unsigned i=0;while(i<b->name_count&&b->names[i].texture!=a[0])++i;
  if(i==b->name_count){if(i>=512)return 0;++b->name_count;}b->names[i].texture=a[0];b->names[i].name=a[1];break;}
 case NFLSTREET_GX_LOAD_TEX_OBJ:return load_texture(b,a[0],a[1]);
 case NFLSTREET_GX_LOAD_TLUT:return load_tlut(b,a[0],a[1]);
 case NFLSTREET_GX_SET_TEV_COLOR_IN:GXSetTevColorIn((GXTevStageID)a[0],(GXTevColorArg)a[1],(GXTevColorArg)a[2],(GXTevColorArg)a[3],(GXTevColorArg)a[4]);break;
 case NFLSTREET_GX_SET_TEV_ALPHA_IN:GXSetTevAlphaIn((GXTevStageID)a[0],(GXTevAlphaArg)a[1],(GXTevAlphaArg)a[2],(GXTevAlphaArg)a[3],(GXTevAlphaArg)a[4]);break;
 case NFLSTREET_GX_SET_TEV_COLOR_OP:GXSetTevColorOp((GXTevStageID)a[0],(GXTevOp)a[1],(GXTevBias)a[2],(GXTevScale)a[3],(GXBool)a[4],(GXTevRegID)a[5]);break;
 case NFLSTREET_GX_SET_TEV_ALPHA_OP:GXSetTevAlphaOp((GXTevStageID)a[0],(GXTevOp)a[1],(GXTevBias)a[2],(GXTevScale)a[3],(GXBool)a[4],(GXTevRegID)a[5]);break;
 case NFLSTREET_GX_SET_TEV_COLOR:{GXColor k=color(c->data[0]);GXSetTevColor((GXTevRegID)a[0],k);break;}
 case NFLSTREET_GX_SET_ALPHA_COMPARE:GXSetAlphaCompare((GXCompare)a[0],(u8)a[1],(GXAlphaOp)a[2],(GXCompare)a[3],(u8)a[4]);break;
 case NFLSTREET_GX_SET_TEV_ORDER:GXSetTevOrder((GXTevStageID)a[0],(GXTexCoordID)a[1],(GXTexMapID)a[2],(GXChannelID)a[3]);break;
 case NFLSTREET_GX_SET_NUM_TEV_STAGES:GXSetNumTevStages((u8)a[0]);break;
 case NFLSTREET_GX_SET_BLEND_MODE:GXSetBlendMode((GXBlendMode)a[0],(GXBlendFactor)a[1],(GXBlendFactor)a[2],(GXLogicOp)a[3]);break;
 case NFLSTREET_GX_CALL_DISPLAY_LIST:return call_list(b,a[0],a[1]);
 case NFLSTREET_GX_SET_CURRENT_MTX:GXSetCurrentMtx(a[0]);break;
 case NFLSTREET_GX_LOAD_TEX_MTX:{float m[3][4];memset(m,0,sizeof m);for(unsigned i=0;i<(a[1]==1?8u:12u);++i)m[i/4][i%4]=word_float(c->data[i]);
  GXLoadTexMtxImm(m,a[0],(GXTexMtxType)a[1]);break;}
 case NFLSTREET_GX_SET_VIEWPORT:GXSetViewport(word_float(c->data[0]),word_float(c->data[1]),word_float(c->data[2]),word_float(c->data[3]),word_float(c->data[4]),word_float(c->data[5]));break;
 case NFLSTREET_GX_LOAD_POS_MTX:{float m[12],r[12];for(unsigned i=0;i<12;++i)m[i]=word_float(c->data[i]);
  if(b->delta_on){delta_mul(b->delta,m,r);memcpy(m,r,sizeof m);}GXLoadPosMtxImm(m,a[0]);break;}
 case NFLSTREET_GX_LOAD_NRM_MTX:{float m[12],r[12];memset(m,0,sizeof m);for(unsigned i=0;i<9;++i)m[4*(i/3)+i%3]=word_float(c->data[i]);
  if(b->delta_on){delta_mul(b->delta,m,r);for(unsigned i=0;i<3;++i)r[4*i+3]=0.0f;memcpy(m,r,sizeof m);}GXLoadNrmMtxImm(m,a[0]);break;}
 case NFLSTREET_GX_SET_Z_MODE:GXSetZMode((GXBool)a[0],(GXCompare)a[1],(GXBool)a[2]);break;
 case NFLSTREET_GX_SET_Z_COMP_LOC:GXSetZCompLoc((GXBool)a[0]);break;
 case NFLSTREET_GX_SET_FOG:GXSetFog((GXFogType)a[0],word_float(c->data[1]),word_float(c->data[2]),word_float(c->data[3]),word_float(c->data[4]),color(c->data[0]));break;
 case NFLSTREET_GX_BEGIN:{const uint32_t v=vertex_bytes(b,a[1]&7),n=a[2]&0xffff;if(!v||!n)return 0;
  b->in_begin=1;b->begin_left=v*n;b->begin_args[0]=a[0];b->begin_args[1]=a[1];b->begin_args[2]=a[2];b->write_count=b->byte_count=0;break;}
 case NFLSTREET_GX_SET_TEV_OP:GXSetTevOp((GXTevStageID)a[0],(GXTevMode)a[1]);break;
 case NFLSTREET_GX_SET_NUM_IND_STAGES:GXSetNumIndStages((u8)a[0]);break;
 case NFLSTREET_GX_SET_TEV_DIRECT:GXSetTevDirect((GXTevStageID)a[0]);break;
 default:return 0;}
 return 1;}
int nflstreet_gx_aurora_begin_frame(nflstreet_gx_aurora *b,uint32_t gx){
 const uint8_t *g=host(b,gx,0x460);if(!g)return 0;
 float p[6],v[6],m[4][4];for(unsigned i=0;i<6;++i){p[i]=word_float(be32(g+0x424+4*i));v[i]=word_float(be32(g+0x43c+4*i));}
 const uint32_t type=be32(g+0x420);memset(m,0,sizeof m);
 if(type==0){m[0][0]=p[0];m[0][2]=p[1];m[1][1]=p[2];m[1][2]=p[3];m[2][2]=p[4];m[2][3]=p[5];m[3][2]=-1.0f;}
 else{m[0][0]=p[0];m[0][3]=p[1];m[1][1]=p[2];m[1][3]=p[3];m[2][2]=p[4];m[2][3]=p[5];m[3][3]=1.0f;}
 GXSetProjection(m,type?GX_ORTHOGRAPHIC:GX_PERSPECTIVE);GXSetViewport(v[0],v[1],v[2],v[3],v[4],v[5]);
 GXSetZMode(GX_TRUE,GX_LEQUAL,GX_TRUE);
 for(unsigned i=0;i<8;++i)GXSetTexCoordGen((GXTexCoordID)i,GX_TG_MTX2x4,GX_TG_TEX0,GX_IDENTITY); /* 801C769C */
 /* Konst selections (KSEL registers: stage pairs, color 4-8/14-18, alpha 9-13/19-23). */
 for(unsigned i=0;i<8;++i){uint32_t k=be32(g+0x1b0+4*i);
  GXSetTevKColorSel((GXTevStageID)(2*i),(GXTevKColorSel)(k>>4&0x1f));GXSetTevKAlphaSel((GXTevStageID)(2*i),(GXTevKAlphaSel)(k>>9&0x1f));
  GXSetTevKColorSel((GXTevStageID)(2*i+1),(GXTevKColorSel)(k>>14&0x1f));GXSetTevKAlphaSel((GXTevStageID)(2*i+1),(GXTevKAlphaSel)(k>>19&0x1f));}
 return 1;}
