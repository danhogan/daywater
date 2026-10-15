#include "gx_render.h"
#include "field_overlay.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#define DECODED_MAX 512u
#define CLIP_MAX 24u
#define NAMES_MAX 2048u
static uint32_t rd32(const uint8_t *p){return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static uint16_t rd16(const uint8_t *p){return(uint16_t)((unsigned)p[0]<<8|p[1]);}
static float as_float(uint32_t v){float f;memcpy(&f,&v,4);return f;}
#define LEVELS_MAX 11u
typedef struct {unsigned width,height;uint8_t *rgba;} level;
/* A decoded texture with its mip levels and the texture object's LOD fields
   (mode0 bits 9-16 bias in 1/32, mode1 min/max LOD in 1/16). */
typedef struct {uint32_t image,tlut,word2,mode1;unsigned levels;level level[LEVELS_MAX];float min_lod,max_lod;} decoded;
struct nflstreet_gx_machine {
 nflstreet_draw_span span;void *memory;nflstreet_gx_frame frame;nflstreet_gx_target *target;
 uint8_t desc[21],cnt[21],type[21],frac[21];
 uint32_t base[25],stride[25];
 float xf[0x100],nrm[0x60];
 uint32_t current;
 unsigned texgens,stages;
 uint32_t tg_src[8],tg_mtx[8],order[16][3],cin[16][4],ain[16][4],cop[16][5],aop[16][5];
 int regs[4][4];
 struct {uint32_t enable,amb_src,mat_src,light_mask,diffuse;} ctrl[2]; /* channel 0: color, alpha */
 uint8_t amb[4],mat[4],lights[8][64];uint32_t zmode,zearly;
 uint32_t cull,alpha[5],blend[4];
 float viewport[6];
 uint32_t maps[8],tluts[32];
 struct {uint32_t texture,name;} names[NAMES_MAX];unsigned name_count;
 decoded cache[DECODED_MAX];unsigned cached;
 uint8_t *imm;size_t imm_size,imm_need,imm_capacity; /* immediate primitive: command, count, vertex bytes */
 nflstreet_gx_stats *stats;
};
static const uint8_t *view(nflstreet_gx_machine *m,uint32_t token,uint32_t size){return token?m->span(m->memory,token,size):NULL;}

/* ---- textures ---- */
static void rgb5a3(unsigned v,uint8_t *o){
 if(v&0x8000){unsigned r=v>>10&31,g=v>>5&31,b=v&31;o[0]=(uint8_t)(r<<3|r>>2);o[1]=(uint8_t)(g<<3|g>>2);o[2]=(uint8_t)(b<<3|b>>2);o[3]=255;}
 else{unsigned a=v>>12&7,r=v>>8&15,g=v>>4&15,b=v&15;o[0]=(uint8_t)(r*17);o[1]=(uint8_t)(g*17);o[2]=(uint8_t)(b*17);o[3]=(uint8_t)(a<<5|a<<2|a>>1);}}
static void rgb565(unsigned v,uint8_t *o){unsigned r=v>>11&31,g=v>>5&63,b=v&31;
 o[0]=(uint8_t)(r<<3|r>>2);o[1]=(uint8_t)(g<<2|g>>4);o[2]=(uint8_t)(b<<3|b>>2);o[3]=255;}
static void palette_color(const uint8_t *tlut,unsigned entries,unsigned format,unsigned index,uint8_t *o){
 if(index>=entries){o[0]=o[1]=o[2]=o[3]=0;return;}unsigned v=rd16(tlut+2*index);
 if(format==0){o[0]=o[1]=o[2]=(uint8_t)(v&255);o[3]=(uint8_t)(v>>8);}else if(format==1)rgb565(v,o);else rgb5a3(v,o);}
int nflstreet_gx_decode(const uint8_t *image,size_t size,unsigned w,unsigned h,unsigned format,
 const uint8_t *tlut,unsigned entries,unsigned tlut_format,uint8_t *rgba){
 static const uint8_t tw[15]={8,8,8,4,4,4,4,0,8,8,4,0,0,0,8},th[15]={8,4,4,4,4,4,4,0,8,4,4,0,0,0,8},bpp[15]={4,8,8,16,16,16,32,0,4,8,16,0,0,0,4};
 if(!image||!rgba||!w||!h||format>14||!tw[format]||((format==8||format==9||format==10)&&(!tlut||tlut_format>2)))return 0;
 unsigned bw=(w+tw[format]-1)/tw[format],bh=(h+th[format]-1)/th[format];size_t block=(size_t)tw[format]*th[format]*bpp[format]/8;
 if(size<(size_t)bw*bh*block)return 0;
 for(unsigned by=0;by<bh;++by)for(unsigned bx=0;bx<bw;++bx){const uint8_t *b=image+((size_t)by*bw+bx)*block;
  for(unsigned y=0;y<th[format];++y)for(unsigned x=0;x<tw[format];++x){unsigned px=bx*tw[format]+x,py=by*th[format]+y;uint8_t c[4];
   unsigned i=y*tw[format]+x;
   switch(format){
   case 0:{unsigned v=b[i/2]>>(i&1?0:4)&15;c[0]=c[1]=c[2]=c[3]=(uint8_t)(v*17);break;}
   case 1:c[0]=c[1]=c[2]=c[3]=b[i];break;
   case 2:{unsigned a=b[i]>>4,l=b[i]&15;c[0]=c[1]=c[2]=(uint8_t)(l*17);c[3]=(uint8_t)(a*17);break;}
   case 3:c[3]=b[2*i];c[0]=c[1]=c[2]=b[2*i+1];break;
   case 4:rgb565(rd16(b+2*i),c);break;
   case 5:rgb5a3(rd16(b+2*i),c);break;
   case 6:c[3]=b[2*i];c[0]=b[2*i+1];c[1]=b[32+2*i];c[2]=b[33+2*i];break;
   case 8:palette_color(tlut,entries,tlut_format,b[i/2]>>(i&1?0:4)&15,c);break;
   case 9:palette_color(tlut,entries,tlut_format,b[i],c);break;
   case 10:palette_color(tlut,entries,tlut_format,rd16(b+2*i)&0x3fff,c);break;
   default:{/* CMPR: 2x2 DXT1 sub-blocks of 4x4. */
    const uint8_t *s=b+8*((y/4)*2+(x/4));unsigned c0=rd16(s),c1=rd16(s+2),sel=s[4+(y&3)]>>(6-2*(x&3))&3;uint8_t p[4][4];
    rgb565(c0,p[0]);rgb565(c1,p[1]);
    for(unsigned k=0;k<3;++k){if(c0>c1){p[2][k]=(uint8_t)((2*p[0][k]+p[1][k])/3);p[3][k]=(uint8_t)((p[0][k]+2*p[1][k])/3);}
     else{p[2][k]=(uint8_t)((p[0][k]+p[1][k])/2);p[3][k]=0;}}
    p[2][3]=255;p[3][3]=c0>c1?255:0;memcpy(c,p[sel],4);}}
   if(px<w&&py<h)memcpy(rgba+4*((size_t)py*w+px),c,4);}}
 return 1;}
/* GXTexObj words: +0 mode0 (wrap s/t, filters), +8 width-1/height-1/format,
   +0C image address >> 5, +18 TLUT name. GXTlutObj: format << 10, address
   >> 5, entry count in the high halfword of the third word. */
static const decoded *texture(nflstreet_gx_machine *m,uint32_t texobj,uint32_t *mode){
 const uint8_t *t=view(m,texobj,0x20);if(!t)return NULL;*mode=rd32(t);
 uint32_t w1=rd32(t+4),w2=rd32(t+8),image=0x80000000u|(rd32(t+12)&0x1fffffu)<<5,format=w2>>20&15,tlut=0,name=rd32(t+0x18);
 for(unsigned i=0;i<m->name_count;++i)if(m->names[i].texture==texobj)name=m->names[i].name;
 if(format>=8&&format<=10){if(name>=32||!m->tluts[name])return NULL;tlut=m->tluts[name];}
 for(unsigned i=0;i<m->cached;++i)if(m->cache[i].image==image&&m->cache[i].tlut==tlut&&m->cache[i].word2==w2&&m->cache[i].mode1==w1)return m->cache+i;
 if(m->cached>=DECODED_MAX)return NULL;
 unsigned w=(w2&0x3ffu)+1,h=(w2>>10&0x3ffu)+1,entries=0,tf=0;const uint8_t *pal=NULL;
 if(tlut){const uint8_t *o=view(m,tlut,12);if(!o)return NULL;tf=rd32(o)>>10&3;entries=rd32(o+8)>>16;
  if(!entries||!(pal=view(m,0x80000000u|(rd32(o+4)&0x1fffffu)<<5,2*entries)))return NULL;}
 static const uint8_t bits[15]={4,8,8,16,16,16,32,0,4,8,16,0,0,0,4},tw[15]={8,8,8,4,4,4,4,0,8,8,4,0,0,0,8},th[15]={8,4,4,4,4,4,4,0,8,4,4,0,0,0,8};
 if(!bits[format])return NULL;
 /* Mip levels follow the base image, each tile-padded (GXGetTexBufferSize);
    the object's mipmap flag (+1F bit 0) and max LOD bound their number. */
 decoded *d=m->cache+m->cached;memset(d,0,sizeof *d);d->image=image;d->tlut=tlut;d->word2=w2;d->mode1=w1;
 d->min_lod=(float)(w1&0xff)/16.0f;d->max_lod=(float)(w1>>8&0xff)/16.0f;
 unsigned levels=(t[0x1f]&1)?(unsigned)d->max_lod+1u:1u;uint32_t at=image;
 for(unsigned l=0;l<levels&&l<LEVELS_MAX;++l){
  size_t bytes=(size_t)((w+tw[format]-1)/tw[format]*tw[format])*((h+th[format]-1)/th[format]*th[format])*bits[format]/8;
  const uint8_t *src=view(m,at,(uint32_t)bytes);uint8_t *rgba=malloc((size_t)w*h*4);
  if(!src||!rgba||!nflstreet_gx_decode(src,bytes,w,h,format,pal,entries,tf,rgba)){free(rgba);if(!l)return NULL;break;}
  d->level[l].width=w;d->level[l].height=h;d->level[l].rgba=rgba;d->levels=l+1;at+=(uint32_t)bytes;
  if(w==1&&h==1)break;w=w>1?w/2:1;h=h>1?h/2:1;}
 ++m->cached;if(m->stats)++m->stats->textures;return d;}
static int wrap(int x,unsigned size,unsigned mode){
 if(mode==1){x%=(int)size;return x<0?x+(int)size:x;}
 if(mode==2){int p=2*(int)size;x%=p;if(x<0)x+=p;return x>=(int)size?p-1-x:x;}
 return x<0?0:x>=(int)size?(int)size-1:x;}
static void bilinear(const level *t,uint32_t mode,int linear,float u,float v,int out[4]){
 float x=u*(float)t->width,y=v*(float)t->height;
 if(!linear){int ix=wrap((int)floorf(x),t->width,mode&3),iy=wrap((int)floorf(y),t->height,mode>>2&3);
  const uint8_t *p=t->rgba+4*((size_t)iy*t->width+(unsigned)ix);for(unsigned c=0;c<4;++c)out[c]=p[c];return;}
 x-=0.5f;y-=0.5f;int ix=(int)floorf(x),iy=(int)floorf(y);float fx=x-(float)ix,fy=y-(float)iy;
 int x0=wrap(ix,t->width,mode&3),x1=wrap(ix+1,t->width,mode&3),y0=wrap(iy,t->height,mode>>2&3),y1=wrap(iy+1,t->height,mode>>2&3);
 const uint8_t *a=t->rgba+4*((size_t)y0*t->width+(unsigned)x0),*b=t->rgba+4*((size_t)y0*t->width+(unsigned)x1),
  *c=t->rgba+4*((size_t)y1*t->width+(unsigned)x0),*d=t->rgba+4*((size_t)y1*t->width+(unsigned)x1);
 for(unsigned k=0;k<4;++k)out[k]=(int)floorf((a[k]*(1-fx)+b[k]*fx)*(1-fy)+(c[k]*(1-fx)+d[k]*fx)*fy+0.5f);}
/* LOD from the screen-space coordinate derivatives (largest axis rate in
   texels per pixel) plus the mode0 bias, clamped to the object's LOD range.
   Magnification uses the mag filter on level 0; minification uses the min
   filter (mode0 bit 7 linear, bits 5-6 mip: 1 nearest, 2 linear). */
static void sample(const decoded *t,uint32_t mode,const float uv[2],const float d[4],int out[4]){
 const level *base=t->level;float w=(float)base->width,h=(float)base->height;
 float rho=fmaxf(fmaxf(fabsf(d[0])*w,fabsf(d[1])*h),fmaxf(fabsf(d[2])*w,fabsf(d[3])*h));
 float lod=(rho>0?log2f(rho):-16.0f)+(float)(int8_t)(mode>>9&0xff)/32.0f;
 if(lod<=0||t->levels<2||!(mode>>5&3)){bilinear(base,mode,lod<=0?(int)(mode>>4&1):(int)(mode>>7&1),uv[0],uv[1],out);return;}
 lod=fminf(fmaxf(lod,t->min_lod),fminf(t->max_lod,(float)(t->levels-1)));
 int linear=(int)(mode>>7&1);
 if((mode>>5&3)==2){unsigned l0=(unsigned)lod,l1=l0+1<t->levels?l0+1:l0;float f=lod-(float)l0;int a[4],b[4];
  bilinear(t->level+l0,mode,linear,uv[0],uv[1],a);bilinear(t->level+l1,mode,linear,uv[0],uv[1],b);
  for(unsigned k=0;k<4;++k)out[k]=(int)floorf((float)a[k]*(1-f)+(float)b[k]*f+0.5f);return;}
 unsigned l=(unsigned)(lod+0.5f);if(l>=t->levels)l=t->levels-1;bilinear(t->level+l,mode,linear,uv[0],uv[1],out);}

/* ---- vertices ---- */
typedef struct {float clip[4],color[4],uv[8][2];} vertex;
static int attribute_size(unsigned attr,unsigned cnt,unsigned type){
 static const uint8_t sizes[5]={1,1,2,2,4},colors[6]={2,3,4,2,3,4};
 if(attr==10)return 3*(type<5?sizes[type]:0);
 if(attr==11||attr==12)return type<6?colors[type]:0;
 if(type>4)return 0;
 unsigned n=attr==9?(cnt?3:2):attr>=13?(cnt?2:1):0;return (int)(n*sizes[type]);}
static float component(const uint8_t *p,unsigned type,unsigned frac,unsigned i){
 float v;switch(type){case 0:v=p[i];break;case 1:v=(int8_t)p[i];break;case 2:v=rd16(p+2*i);break;case 3:v=(int16_t)rd16(p+2*i);break;
 default:return as_float(rd32(p+4*i));}
 return ldexpf(v,-(int)frac);}
static void color_of(const uint8_t *p,unsigned type,float o[4]){
 uint8_t c[4]={255,255,255,255};
 switch(type){case 0:rgb565(rd16(p),c);break;case 1:case 2:c[0]=p[0];c[1]=p[1];c[2]=p[2];break;
 case 3:{unsigned v=rd16(p);c[0]=(uint8_t)((v>>12)*17);c[1]=(uint8_t)((v>>8&15)*17);c[2]=(uint8_t)((v>>4&15)*17);c[3]=(uint8_t)((v&15)*17);break;}
 case 4:{uint32_t v=(uint32_t)p[0]<<16|(uint32_t)p[1]<<8|p[2];for(unsigned k=0;k<4;++k){unsigned x=v>>(18-6*k)&63;c[k]=(uint8_t)(x<<2|x>>4);}break;}
 default:memcpy(c,p,4);}
 for(unsigned k=0;k<4;++k)o[k]=c[k];}
static void normalize3(float v[3]){float n=sqrtf(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);if(n>0)for(unsigned i=0;i<3;++i)v[i]/=n;}
/* GX lighting of channel 0, color (RGB) and alpha controlled separately:
   material from the register or vertex color; with lighting, ambient plus
   each enabled light's color times clamped N.L (no attenuation), clamped,
   then material * (light + light>>7) >> 8; without, the material alone. */
static void light(nflstreet_gx_machine *m,const float pos[3],const float n[3],const float vtx[4],float out[4]){
 int mat[4],acc[4];
 for(unsigned c=0;c<4;++c){const unsigned k=c<3?0:1;mat[c]=m->ctrl[k].mat_src?(int)lroundf(vtx[c]):m->mat[c];
  acc[c]=!m->ctrl[k].enable?255:m->ctrl[k].amb_src?(int)lroundf(vtx[c]):m->amb[c];}
 for(unsigned l=0;l<8;++l){unsigned use[2]={m->ctrl[0].enable&&(m->ctrl[0].light_mask>>l&1),m->ctrl[1].enable&&(m->ctrl[1].light_mask>>l&1)};
  if(!use[0]&&!use[1])continue;const uint8_t *o=m->lights[l];
  float dir[3];for(unsigned k=0;k<3;++k)dir[k]=as_float(rd32(o+0x28+4*k))-pos[k];
  float len=sqrtf(dir[0]*dir[0]+dir[1]*dir[1]+dir[2]*dir[2]);if(len==0)memcpy(dir,n,sizeof dir);else for(unsigned k=0;k<3;++k)dir[k]/=len;
  for(unsigned c=0;c<4;++c){const unsigned k=c<3?0:1;if(!use[k])continue;
   float d=dir[0]*n[0]+dir[1]*n[1]+dir[2]*n[2];if(m->ctrl[k].diffuse==2&&d<0)d=0;if(m->ctrl[k].diffuse==0)d=1;
   acc[c]+=(int)lroundf(d*(float)o[0xc+c]);}}
 for(unsigned c=0;c<4;++c){int a=acc[c]<0?0:acc[c]>255?255:acc[c];out[c]=(float)((mat[c]*(a+(a>>7)))>>8);}}
static int load_vertex(nflstreet_gx_machine *m,const uint8_t **at,const uint8_t *end,vertex *v){
 float pos[3]={0,0,0},nrm[3]={0,0,1},vtx[4]={255,255,255,255},tex[8][2];uint32_t row=m->current;memset(tex,0,sizeof tex);
 for(unsigned attr=0;attr<21;++attr){unsigned d=m->desc[attr];if(!d)continue;
  if(attr<=8){if(d!=1||*at>=end)return 0;unsigned x=*(*at)++;if(attr==0)row=x;continue;}
  int size=attribute_size(attr,m->cnt[attr],m->type[attr]);if(size<=0)return 0;const uint8_t *p;
  if(d==1){if(end-*at<size)return 0;p=*at;*at+=size;}
  else{unsigned bytes=d==2?1u:2u;if((unsigned)(end-*at)<bytes)return 0;unsigned index=d==2?**at:rd16(*at);*at+=bytes;
   if((d==2&&index==0xff)||(d==3&&index==0xffff))continue;
   if(!(p=view(m,m->base[attr]+index*m->stride[attr],(uint32_t)size)))return 0;}
  if(attr==9){for(unsigned k=0;k<(m->cnt[9]?3u:2u);++k)pos[k]=component(p,m->type[9],m->frac[9],k);}
  else if(attr==10){if(m->cnt[10])return 0;unsigned f=m->type[10]==1?6:m->type[10]==3?14:0;for(unsigned k=0;k<3;++k)nrm[k]=component(p,m->type[10],f,k);}
  else if(attr==11)color_of(p,m->type[11],vtx);
  else if(attr>=13)for(unsigned k=0;k<(m->cnt[attr]?2u:1u);++k)tex[attr-13][k]=component(p,m->type[attr],m->frac[attr],k);}
 if(row>=64)return 0;
 const float *pm=m->xf+4*row,*nm=m->nrm+3*(row&31);float eye[3],n[3];
 for(unsigned r=0;r<3;++r){eye[r]=pm[4*r]*pos[0]+pm[4*r+1]*pos[1]+pm[4*r+2]*pos[2]+pm[4*r+3];n[r]=nm[3*r]*nrm[0]+nm[3*r+1]*nrm[1]+nm[3*r+2]*nrm[2];}
 normalize3(n);
 const float *p=m->frame.projection;
 if(m->frame.projection_type==0){v->clip[0]=p[0]*eye[0]+p[1]*eye[2];v->clip[1]=p[2]*eye[1]+p[3]*eye[2];v->clip[2]=p[4]*eye[2]+p[5];v->clip[3]=-eye[2];}
 else{v->clip[0]=p[0]*eye[0]+p[1];v->clip[1]=p[2]*eye[1]+p[3];v->clip[2]=p[4]*eye[2]+p[5];v->clip[3]=1;}
 light(m,eye,n,vtx,v->color);
 for(unsigned c=0;c<m->texgens&&c<8;++c){unsigned src=m->tg_src[c]-4u;if(src>7)return 0;
  float s=tex[src][0],t=tex[src][1];
  if(m->tg_mtx[c]==60){v->uv[c][0]=s;v->uv[c][1]=t;}
  else{const float *x=m->xf+4*m->tg_mtx[c];if(m->tg_mtx[c]>62)return 0;v->uv[c][0]=x[0]*s+x[1]*t+x[2]+x[3];v->uv[c][1]=x[4]*s+x[5]*t+x[6]+x[7];}}
 return 1;}

/* ---- pixels ---- */
static int clampi(int v,int lo,int hi){return v<lo?lo:v>hi?hi:v;}
static int compare(int a,int b,unsigned f){
 switch(f){case 0:return 0;case 1:return a<b;case 2:return a==b;case 3:return a<=b;case 4:return a>b;case 5:return a!=b;case 6:return a>=b;default:return 1;}}
/* One TEV stage, Dolphin's integer formula: (d + bias) op lerp(a, b, c),
   c expanded to 0..256, scale inside the lerp with a rounding bias. */
static int tev(int a,int b,int c,int d,const uint32_t op[5]){
 a&=255;b&=255;c&=255;int bias=op[1]==1?128:op[1]==2?-128:0,shift=op[2]==1?1:op[2]==2?2:0;
 int lerp=(a<<8)+(b-a)*(c+(c>>7)),r;
 if(op[2]==3)r=((d+bias)+(op[0]?-1:1)*(lerp>>8))>>1;
 else r=((d+bias)<<shift)+(op[0]?-1:1)*(((lerp<<shift)+(op[0]?127:128))>>8);
 return op[3]?clampi(r,0,255):clampi(r,-1024,1023);}
typedef struct {const decoded *texture;uint32_t mode;} bound;
static void pixel(nflstreet_gx_machine *m,const bound *maps,unsigned x,unsigned y,float z,const float color[4],float uv[8][2],float duv[8][4]){
 nflstreet_gx_target *t=m->target;size_t at=(size_t)y*t->width+x;int regs[4][4],ras[4],k[4];memcpy(regs,m->regs,sizeof regs);
 for(unsigned c=0;c<4;++c)ras[c]=clampi((int)lroundf(color[c]),0,255);
 static const int fractions[8]={255,223,191,159,128,96,64,32};
 for(unsigned s=0;s<m->stages;++s){int tex[4]={0,0,0,0},r[4]={0,0,0,0};
  if(m->order[s][1]!=0xff){const bound *b=maps+(m->order[s][1]&7);unsigned coord=m->order[s][0]&7;
   if(b->texture)sample(b->texture,b->mode,uv[coord],duv[coord],tex);}
  if(m->order[s][2]==4||m->order[s][2]==0)memcpy(r,ras,sizeof r);
  uint32_t ks=m->frame.ksel[s/2]>>(s&1?14:4),kc=ks&31,ka=ks>>5&31;
  for(unsigned c=0;c<3;++c)k[c]=kc<8?fractions[kc]:kc>=12?m->frame.konst[(kc-12)&3][kc>=16?(kc-16)/4:c]:0;
  k[3]=ka<8?fractions[ka]:ka>=16?m->frame.konst[(ka-16)&3][(ka-16)/4]:0;
  int out[4];
  for(unsigned c=0;c<3;++c){int in[4];
   for(unsigned i=0;i<4;++i){unsigned sel=m->cin[s][i];
    in[i]=sel==0?regs[0][c]:sel==1?regs[0][3]:sel<=7?regs[(sel-2)/2+1][sel&1?3:c]:sel==8?tex[c]:sel==9?tex[3]:sel==10?r[c]:sel==11?r[3]:sel==12?255:sel==13?128:sel==14?k[c]:0;}
   out[c]=tev(in[0],in[1],in[2],in[3],m->cop[s]);}
  {int in[4];for(unsigned i=0;i<4;++i){unsigned sel=m->ain[s][i];
    in[i]=sel==0?regs[0][3]:sel<=3?regs[sel][3]:sel==4?tex[3]:sel==5?r[3]:sel==6?k[3]:0;}
   out[3]=tev(in[0],in[1],in[2],in[3],m->aop[s]);}
  unsigned dc=m->cop[s][4]&3,da=m->aop[s][4]&3;for(unsigned c=0;c<3;++c)regs[dc][c]=out[c];regs[da][3]=out[3];}
 int o[4];for(unsigned c=0;c<4;++c)o[c]=clampi(regs[0][c],0,255);
 int a0=compare(o[3],(int)m->alpha[1],m->alpha[0]),a1=compare(o[3],(int)m->alpha[4],m->alpha[3]),pass;
 switch(m->alpha[2]){case 0:pass=a0&&a1;break;case 1:pass=a0||a1;break;case 2:pass=a0!=a1;break;default:pass=a0==a1;}
 /* Depth before texturing (z compare location 0) also updates for pixels
    the alpha compare then discards. */
 uint32_t zmode=m->zmode,depth=(uint32_t)clampi((int)z,0,0xffffff);
 if(m->zearly){if((zmode&1)&&!compare((int)depth,(int)t->depth[at],zmode>>1&7))return;if((zmode&1)&&(zmode&16))t->depth[at]=depth;}
 if(!pass)return;
 if(!m->zearly){if((zmode&1)&&!compare((int)depth,(int)t->depth[at],zmode>>1&7))return;if((zmode&1)&&(zmode&16))t->depth[at]=depth;}
 if(!(m->frame.cmode0&8))return;
 uint32_t dst=t->bgra[at];int dc[4]={(int)(dst>>16&255),(int)(dst>>8&255),(int)(dst&255),255},res[4];
 if(m->blend[0]==1){for(unsigned c=0;c<3;++c){
   int sf,df;unsigned s=m->blend[1],d=m->blend[2];
   sf=s==0?0:s==1?255:s==2?dc[c]:s==3?255-dc[c]:s==4?o[3]:s==5?255-o[3]:s==6?dc[3]:255-dc[3];
   df=d==0?0:d==1?255:d==2?o[c]:d==3?255-o[c]:d==4?o[3]:d==5?255-o[3]:d==6?dc[3]:255-dc[3];
   res[c]=clampi((o[c]*sf+dc[c]*df+127)/255,0,255);}}
 else if(m->blend[0]==3){for(unsigned c=0;c<3;++c)res[c]=clampi(dc[c]-o[c],0,255);}
 else for(unsigned c=0;c<3;++c)res[c]=o[c];
 t->bgra[at]=0xff000000u|(uint32_t)res[0]<<16|(uint32_t)res[1]<<8|(uint32_t)res[2];
 if(m->stats)++m->stats->pixels;}
static float plane(const vertex *v,unsigned p){
 switch(p){case 0:return v->clip[3]+v->clip[2];case 1:return -v->clip[2];case 2:return v->clip[3]-v->clip[0];case 3:return v->clip[3]+v->clip[0];
 case 4:return v->clip[3]-v->clip[1];default:return v->clip[3]+v->clip[1];}}
static vertex mix(const vertex *a,const vertex *b,float t){vertex o;
 for(unsigned i=0;i<4;++i){o.clip[i]=a->clip[i]+(b->clip[i]-a->clip[i])*t;o.color[i]=a->color[i]+(b->color[i]-a->color[i])*t;}
 for(unsigned i=0;i<8;++i)for(unsigned j=0;j<2;++j)o.uv[i][j]=a->uv[i][j]+(b->uv[i][j]-a->uv[i][j])*t;return o;}
typedef struct {float x,y,z,w,color[4],uv[8][2];} screen;
static float edge(const screen *a,const screen *b,float x,float y){return (x-a->x)*(b->y-a->y)-(y-a->y)*(b->x-a->x);}
static void raster(nflstreet_gx_machine *m,const bound *maps,const screen *s0,const screen *s1,const screen *s2){
 nflstreet_gx_target *t=m->target;float area=edge(s0,s1,s2->x,s2->y);if(!isfinite(area)||fabsf(area)<1e-9f)return;
 int left=(int)floorf(fminf(s0->x,fminf(s1->x,s2->x))),right=(int)ceilf(fmaxf(s0->x,fmaxf(s1->x,s2->x)));
 int top=(int)floorf(fminf(s0->y,fminf(s1->y,s2->y))),bottom=(int)ceilf(fmaxf(s0->y,fmaxf(s1->y,s2->y)));
 left=clampi(left,0,(int)t->width);right=clampi(right,0,(int)t->width);top=clampi(top,0,(int)t->height);bottom=clampi(bottom,0,(int)t->height);
 for(int y=top;y<bottom;++y)for(int x=left;x<right;++x){float px=(float)x+0.5f,py=(float)y+0.5f;
  float a=edge(s1,s2,px,py)/area,b=edge(s2,s0,px,py)/area,c=1-a-b;if(a<0||b<0||c<0)continue;
  float w=a*s0->w+b*s1->w+c*s2->w,z=a*s0->z+b*s1->z+c*s2->z,color[4],uv[8][2]={{0}},duv[8][4]={{0}};
  for(unsigned k=0;k<4;++k)color[k]=(a*s0->color[k]+b*s1->color[k]+c*s2->color[k])/w;
  /* Neighbor pixels on the same planes give the coordinate derivatives. */
  float ax=edge(s1,s2,px+1,py)/area,bx=edge(s2,s0,px+1,py)/area,cx=1-ax-bx,ay=edge(s1,s2,px,py+1)/area,by=edge(s2,s0,px,py+1)/area,cy=1-ay-by;
  float wx=ax*s0->w+bx*s1->w+cx*s2->w,wy=ay*s0->w+by*s1->w+cy*s2->w;
  for(unsigned i=0;i<m->texgens&&i<8;++i)for(unsigned j=0;j<2;++j){
   uv[i][j]=(a*s0->uv[i][j]+b*s1->uv[i][j]+c*s2->uv[i][j])/w;
   duv[i][j]=(ax*s0->uv[i][j]+bx*s1->uv[i][j]+cx*s2->uv[i][j])/wx-uv[i][j];
   duv[i][2+j]=(ay*s0->uv[i][j]+by*s1->uv[i][j]+cy*s2->uv[i][j])/wy-uv[i][j];}
  pixel(m,maps,(unsigned)x,(unsigned)y,z,color,uv,duv);}}
static void to_screen(nflstreet_gx_machine *m,const vertex *v,screen *s){
 float inv=1/v->clip[3],*vp=m->viewport;
 s->x=v->clip[0]*inv*vp[2]*0.5f+vp[0]+vp[2]*0.5f;s->y=-v->clip[1]*inv*vp[3]*0.5f+vp[1]+vp[3]*0.5f;
 s->z=(vp[5]+v->clip[2]*inv*(vp[5]-vp[4]))*16777215.0f;s->w=inv;
 for(unsigned k=0;k<4;++k)s->color[k]=v->color[k]*inv;
 for(unsigned i=0;i<8;++i)for(unsigned j=0;j<2;++j)s->uv[i][j]=v->uv[i][j]*inv;}
/* Cull on the clip-space winding (Dolphin CPUCull: front culls <= 0, back >= 0,
   for the usual negative viewport height). */
static void triangle(nflstreet_gx_machine *m,const vertex *a,const vertex *b,const vertex *c){
 ++m->stats->triangles;
 float d=(c->clip[3]*a->clip[0]-a->clip[3]*c->clip[0])*b->clip[1]+(c->clip[0]*a->clip[1]-a->clip[0]*c->clip[1])*b->clip[3]+
  (c->clip[1]*a->clip[3]-a->clip[1]*c->clip[3])*b->clip[0];
 int cull=m->cull==0?d==0:m->cull==2?d<=0:m->cull==1?d>=0:1;if(cull){++m->stats->culled;return;}
 vertex poly[CLIP_MAX],tmp[CLIP_MAX];unsigned n=3;poly[0]=*a;poly[1]=*b;poly[2]=*c;
 for(unsigned p=0;p<6&&n;++p){unsigned out=0;
  for(unsigned i=0;i<n;++i){const vertex *u=poly+i,*v=poly+(i+1)%n;float du=plane(u,p),dv=plane(v,p);
   if(du>=0&&out<CLIP_MAX)tmp[out++]=*u;
   if((du>=0)!=(dv>=0)&&out<CLIP_MAX)tmp[out++]=mix(u,v,du/(du-dv));}
  memcpy(poly,tmp,out*sizeof *poly);n=out;}
 if(n<3){++m->stats->clipped;return;}
 screen s[CLIP_MAX];for(unsigned i=0;i<n;++i)to_screen(m,poly+i,s+i);
 bound maps[8];memset(maps,0,sizeof maps);
 for(unsigned st=0;st<m->stages;++st){unsigned map=m->order[st][1];
  if(map!=0xff&&!maps[map&7].texture&&m->maps[map&7])maps[map&7].texture=texture(m,m->maps[map&7],&maps[map&7].mode);}
 for(unsigned i=1;i+1<n;++i)raster(m,maps,s,s+i,s+i+1);}
static int primitive(nflstreet_gx_machine *m,unsigned cmd,const uint8_t **at,const uint8_t *end);
/* Bytes of one vertex under the current descriptors (vertex format 0). */
static size_t vertex_bytes(const nflstreet_gx_machine *m){
 size_t n=0;for(unsigned attr=0;attr<21;++attr){unsigned d=m->desc[attr];if(!d)continue;
  if(attr<=8)n+=1;else if(d==2)n+=1;else if(d==3)n+=2;else{int k=attribute_size(attr,m->cnt[attr],m->type[attr]);if(k<=0)return 0;n+=(size_t)k;}}
 return n;}
static int immediate_flush(nflstreet_gx_machine *m){
 if(!m->imm_need||m->imm_size<m->imm_need)return 1;const uint8_t *at=m->imm+1,*end=m->imm+m->imm_size;
 m->imm_need=0;return primitive(m,m->imm[0],&at,end);}
static int primitive(nflstreet_gx_machine *m,unsigned cmd,const uint8_t **at,const uint8_t *end){
 if((size_t)(end-*at)<2)return 0;unsigned count=rd16(*at),kind=cmd&0xf8;*at+=2;
 if((cmd&7)!=0)return 0;
 vertex *v=malloc((size_t)(count?count:1)*sizeof *v);if(!v)return 0;
 for(unsigned i=0;i<count;++i)if(!load_vertex(m,at,end,v+i)){free(v);return 0;}
 ++m->stats->primitives;
 if(kind==0x80||kind==0x88){for(unsigned i=3;i<count;i+=4){triangle(m,v+i-3,v+i-2,v+i-1);triangle(m,v+i-3,v+i-1,v+i);}}
 else if(kind==0x90){for(unsigned i=2;i<count;i+=3)triangle(m,v+i-2,v+i-1,v+i);}
 else if(kind==0x98){for(unsigned i=2;i<count;++i){if(i&1)triangle(m,v+i-2,v+i,v+i-1);else triangle(m,v+i-2,v+i-1,v+i);}}
 else if(kind==0xa0){for(unsigned i=2;i<count;++i)triangle(m,v,v+i-1,v+i);}
 free(v);return 1;}
/* Display list commands: NOP, CP/XF/BP loads, indexed XF loads A..D
   (arrays 0x15..0x18), vertex cache invalidation and primitives. */
static int display_list(nflstreet_gx_machine *m,uint32_t list,uint32_t bytes){
 const uint8_t *at=view(m,list,bytes),*end;if(!bytes)return 1;if(!at)return 0;end=at+bytes;++m->stats->display_lists;
 while(at<end){unsigned cmd=*at++;
  if(cmd==0x00||cmd==0x48)continue;
  if(cmd==0x08){if(end-at<5)return 0;at+=5;continue;}
  if(cmd==0x61){if(end-at<4)return 0;at+=4;continue;}
  if(cmd==0x10){if(end-at<4)return 0;unsigned n=rd16(at)+1u,addr=rd16(at+2);at+=4;if((size_t)(end-at)<4u*n)return 0;
   for(unsigned i=0;i<n;++i,addr++){float f=as_float(rd32(at+4*i));if(addr<0x100)m->xf[addr]=f;else if(addr>=0x400&&addr<0x460)m->nrm[addr-0x400]=f;}
   at+=4*n;continue;}
  if(cmd==0x20||cmd==0x28||cmd==0x30||cmd==0x38){if(end-at<4)return 0;unsigned index=rd16(at),w=rd16(at+2),n=(w>>12)+1,addr=w&0xfff;at+=4;
   unsigned array=0x15+(cmd-0x20)/8;const uint8_t *src=view(m,m->base[array]+index*m->stride[array],4*n);if(!src)return 0;
   for(unsigned i=0;i<n;++i,addr++){float f=as_float(rd32(src+4*i));if(addr<0x100)m->xf[addr]=f;else if(addr>=0x400&&addr<0x460)m->nrm[addr-0x400]=f;}
   continue;}
  if(cmd>=0x80&&cmd<0xc0){if(!primitive(m,cmd,&at,end))return 0;continue;}
  return 0;}
 return 1;}

int nflstreet_gx_run(nflstreet_gx_machine *m,const nflstreet_gx_call *calls,unsigned count,nflstreet_gx_stats *stats){
 nflstreet_gx_stats unused;if(!m||(!calls&&count))return 0;m->stats=stats?stats:&unused;
 for(unsigned i=0;i<count;++i){const nflstreet_gx_call *c=calls+i;const uint32_t *a=c->arg;
  switch(c->op){
  case NFLSTREET_GX_SET_VTX_DESC:if(a[0]>20)return 0;m->desc[a[0]]=(uint8_t)a[1];break;
  case NFLSTREET_GX_CLEAR_VTX_DESC:memset(m->desc,0,sizeof m->desc);break;
  case NFLSTREET_GX_SET_VTX_ATTR_FMT:if(a[0]||a[1]>20)return 0;m->cnt[a[1]]=(uint8_t)a[2];m->type[a[1]]=(uint8_t)a[3];m->frac[a[1]]=(uint8_t)a[4];break;
  case NFLSTREET_GX_SET_ARRAY:if(a[0]>24)return 0;m->base[a[0]]=a[1];m->stride[a[0]]=a[2];break;
  case NFLSTREET_GX_SET_TEX_COORD_GEN:if(a[0]>7||a[1]>1)return 0;m->tg_src[a[0]]=a[2];m->tg_mtx[a[0]]=a[3];break;
  case NFLSTREET_GX_SET_NUM_TEX_GENS:m->texgens=a[0];break;
  case NFLSTREET_GX_SET_CULL_MODE:m->cull=a[0]==1?2:a[0]==2?1:a[0];break;
  case NFLSTREET_GX_LOAD_LIGHT:{unsigned l=0;while(l<8&&a[0]!=1u<<l)++l;if(l==8)return 0;
   for(unsigned k=0;k<16;++k){uint8_t *p=m->lights[l]+4*k;p[0]=(uint8_t)(c->data[k]>>24);p[1]=(uint8_t)(c->data[k]>>16);p[2]=(uint8_t)(c->data[k]>>8);p[3]=(uint8_t)c->data[k];}
   break;}
  /* Channel ids: 0 COLOR0, 2 ALPHA0, 4 COLOR0A0; channel 1 is not used by TEV here. */
  case NFLSTREET_GX_SET_CHAN_AMB_COLOR:case NFLSTREET_GX_SET_CHAN_MAT_COLOR:{if(a[0]!=0&&a[0]!=2&&a[0]!=4)break;
   uint8_t *p=c->op==NFLSTREET_GX_SET_CHAN_AMB_COLOR?m->amb:m->mat;
   for(unsigned k=a[0]==2?3u:0u;k<(a[0]==0?3u:4u);++k)p[k]=(uint8_t)(c->data[0]>>(24-8*k));break;}
  case NFLSTREET_GX_SET_NUM_CHANS:break;
  case NFLSTREET_GX_SET_CHAN_CTRL:{if(a[0]!=0&&a[0]!=2&&a[0]!=4)break;if(a[6]!=2&&a[1]&&a[4])return 0;
   for(unsigned k=a[0]==2?1u:0u;k<(a[0]==0?1u:2u);++k){m->ctrl[k].enable=a[1];m->ctrl[k].amb_src=a[2];m->ctrl[k].mat_src=a[3];m->ctrl[k].light_mask=a[4];m->ctrl[k].diffuse=a[5];}
   break;}
  case NFLSTREET_GX_LOAD_POS_MTX:if(a[0]>52)return 0;for(unsigned k=0;k<12;++k)m->xf[4*a[0]+k]=as_float(c->data[k]);break;
  case NFLSTREET_GX_LOAD_NRM_MTX:if(a[0]>29)return 0;for(unsigned k=0;k<9;++k)m->nrm[3*a[0]+k]=as_float(c->data[k]);break;
  case NFLSTREET_GX_SET_Z_MODE:m->zmode=(a[0]&1)|(a[1]&7)<<1|(a[2]&1)<<4;break;
  case NFLSTREET_GX_SET_Z_COMP_LOC:m->zearly=!a[0];break;
  case NFLSTREET_GX_SET_FOG:if(a[0])return 0;break;
  /* No indirect texturing: zero indirect stages, direct TEV stages. */
  case NFLSTREET_GX_SET_NUM_IND_STAGES:if(a[0])return 0;break;
  case NFLSTREET_GX_SET_TEV_DIRECT:break;
  /* GXBegin then the vertex bytes the game writes to the pipe. */
  /* GXBegin writes type | format as one command byte; some callers pass
     them swapped (801590F4: GXBegin(0, 0x98, n)), with the same effect. */
  case NFLSTREET_GX_BEGIN:{const uint32_t cmd=a[0]|a[1];if(m->imm_need||cmd>0xffu||(cmd&7u))return 0;size_t v=vertex_bytes(m),need=3+v*(a[2]&0xffff);if(!v)return 0;
   if(need>m->imm_capacity){uint8_t *b=realloc(m->imm,need);if(!b)return 0;m->imm=b;m->imm_capacity=need;}
   m->imm[0]=(uint8_t)cmd;m->imm[1]=(uint8_t)(a[2]>>8);m->imm[2]=(uint8_t)a[2];m->imm_size=3;m->imm_need=need;
   if(!immediate_flush(m))return 0;break;}
  case NFLSTREET_GX_WRITE:{unsigned bytes=a[0];if(!m->imm_need||(bytes!=1&&bytes!=2&&bytes!=4)||m->imm_size+bytes>m->imm_need)return 0;
   for(unsigned k=0;k<bytes;++k)m->imm[m->imm_size++]=(uint8_t)(a[1]>>(8*(bytes-1-k)));
   if(!immediate_flush(m))return 0;break;}
  /* GXSetTevOp: the SDK's register images (stage 0 at 80298AB8/80298AE0,
     later stages at 80298ACC/80298AF4) for modulate, decal, blend, replace
     and pass color. */
  case NFLSTREET_GX_SET_TEV_OP:{static const uint32_t color[2][5]={{0x08f8af,0x08a89f,0x08ac8f,0x08fff8,0x08fffa},{0x08f80f,0x08089f,0x080c8f,0x08fff8,0x08fff0}},
    alpha[2][5]={{0x08f2f0,0x08ffd0,0x08f2f0,0x08ffc0,0x08ffd0},{0x08f070,0x08ff80,0x08f070,0x08ffc0,0x08ff80}};
   if(a[0]>15||a[1]>4)return 0;uint32_t cw=color[a[0]?1:0][a[1]],aw=alpha[a[0]?1:0][a[1]];
   const uint32_t ops[5]={cw>>18&1,cw>>16&3,cw>>20&3,cw>>19&1,cw>>22&3},aops[5]={aw>>18&1,aw>>16&3,aw>>20&3,aw>>19&1,aw>>22&3};
   m->cin[a[0]][0]=cw>>12&15;m->cin[a[0]][1]=cw>>8&15;m->cin[a[0]][2]=cw>>4&15;m->cin[a[0]][3]=cw&15;
   m->ain[a[0]][0]=aw>>13&7;m->ain[a[0]][1]=aw>>10&7;m->ain[a[0]][2]=aw>>7&7;m->ain[a[0]][3]=aw>>4&7;
   for(unsigned k=0;k<5;++k){m->cop[a[0]][k]=ops[k];m->aop[a[0]][k]=aops[k];}break;}
  case NFLSTREET_GX_INIT_TEX_OBJ_TLUT:{unsigned k=0;while(k<m->name_count&&m->names[k].texture!=a[0])++k;
   if(k==m->name_count){if(k>=NAMES_MAX)return 0;++m->name_count;}m->names[k].texture=a[0];m->names[k].name=a[1];break;}
  case NFLSTREET_GX_LOAD_TEX_OBJ:if(a[1]>7)return 0;m->maps[a[1]]=a[0];break;
  case NFLSTREET_GX_LOAD_TLUT:if(a[1]>=32)return 0;m->tluts[a[1]]=a[0];break;
  case NFLSTREET_GX_SET_TEV_COLOR_IN:if(a[0]>15)return 0;for(unsigned k=0;k<4;++k)m->cin[a[0]][k]=a[1+k];break;
  case NFLSTREET_GX_SET_TEV_ALPHA_IN:if(a[0]>15)return 0;for(unsigned k=0;k<4;++k)m->ain[a[0]][k]=a[1+k];break;
  case NFLSTREET_GX_SET_TEV_COLOR_OP:case NFLSTREET_GX_SET_TEV_ALPHA_OP:if(a[0]>15||a[1]>1)return 0;
   for(unsigned k=0;k<5;++k)(c->op==NFLSTREET_GX_SET_TEV_COLOR_OP?m->cop:m->aop)[a[0]][k]=a[1+k];break;
  case NFLSTREET_GX_SET_TEV_COLOR:if(a[0]>3)return 0;for(unsigned k=0;k<4;++k)m->regs[a[0]][k]=(int)(c->data[0]>>(24-8*k)&255);break;
  case NFLSTREET_GX_SET_ALPHA_COMPARE:for(unsigned k=0;k<5;++k)m->alpha[k]=a[k];break;
  case NFLSTREET_GX_SET_TEV_ORDER:if(a[0]>15)return 0;for(unsigned k=0;k<3;++k)m->order[a[0]][k]=a[1+k];break;
  case NFLSTREET_GX_SET_NUM_TEV_STAGES:if(!a[0]||a[0]>16)return 0;m->stages=a[0];break;
  case NFLSTREET_GX_SET_BLEND_MODE:if(a[0]==2)return 0;for(unsigned k=0;k<4;++k)m->blend[k]=a[k];break;
  case NFLSTREET_GX_CALL_DISPLAY_LIST:if(!display_list(m,a[0],a[1]))return 0;break;
  case NFLSTREET_GX_SET_CURRENT_MTX:m->current=a[0];break;
  case NFLSTREET_GX_LOAD_TEX_MTX:{if(a[1]!=1||a[0]>61)return 0;for(unsigned k=0;k<8;++k)m->xf[4*a[0]+k]=as_float(c->data[k]);break;}
  case NFLSTREET_GX_SET_PROJECTION_V:
   if(m->imm_need)return 0;m->frame.projection_type=as_float(c->data[0])==0.0f?0u:1u;
   for(unsigned k=0;k<6;++k)m->frame.projection[k]=as_float(c->data[k+1]);break;
  case NFLSTREET_FIELD_GX_SET_PROJECTION:{
   if(m->imm_need)return 0;m->frame.projection_type=a[0];
   const unsigned index[6]={0,a[0]?3u:2u,5,a[0]?7u:6u,10,11};
   for(unsigned k=0;k<6;++k)m->frame.projection[k]=as_float(c->data[index[k]]);break;}
  case NFLSTREET_FIELD_GX_PEEK_Z:if(m->imm_need)return 0;break; /* observation; caller already sampled the target */
  case NFLSTREET_GX_SET_VIEWPORT:for(unsigned k=0;k<6;++k)m->viewport[k]=as_float(c->data[k]);break;
  default:return 0;}}
 return 1;}
nflstreet_gx_machine *nflstreet_gx_open(nflstreet_draw_span span,void *memory,const nflstreet_gx_frame *frame,nflstreet_gx_target *target){
 if(!span||!frame||!target||!target->bgra||!target->depth||!target->width||!target->height)return NULL;
 nflstreet_gx_machine *m=calloc(1,sizeof *m);if(!m)return NULL;
 m->span=span;m->memory=memory;m->frame=*frame;m->target=target;
 for(unsigned i=0;i<8;++i){m->tg_src[i]=frame->texgen_src[i];m->tg_mtx[i]=frame->texgen_mtx[i];}
 for(unsigned r=0;r<4;++r)for(unsigned k=0;k<4;++k)m->regs[r][k]=frame->registers[r][k];
 m->alpha[0]=m->alpha[3]=7;m->stages=1;m->texgens=1;m->zmode=frame->zmode;
 memcpy(m->viewport,frame->viewport,sizeof m->viewport);
 return m;}
void nflstreet_gx_close(nflstreet_gx_machine *m){if(!m)return;free(m->imm);
 for(unsigned i=0;i<m->cached;++i)for(unsigned l=0;l<m->cache[i].levels;++l)free(m->cache[i].level[l].rgba);free(m);}
