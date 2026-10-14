#include "live_pose.h"
#include "draw_engine.h"
#include "../actor_target_input/locomotion_selection.h"
#include <math.h>
#include <string.h>
/* Host addresses served beside the RAM span: the bank file and this state's buffer. */
#define BANK 0x90000000u
#define HOST 0x98000000u
#define DESCRIPTOR (HOST+0x000u)  /* the bank's descriptor, its pointers relocated to BANK */
#define LIST (HOST+0x020u)        /* u16 count + 0x2C-byte entries */
#define RECORDS (HOST+0x200u)     /* 0x100 per streamed frame record */
#define CHANNELS (HOST+0xa00u)    /* four 0x7C-byte channel records for 8015EC30 */
#define CLIPS (HOST+0xc00u)       /* four 100-byte clip-set entries (80178440) */
typedef nflstreet_live_pose state;
static uint8_t *composite(void *m,uint32_t a,uint32_t n){state *s=m;
 if(a>=HOST&&a<HOST+NFLSTREET_LIVE_POSE_HOST_BYTES)return (uint64_t)a-HOST+n<=NFLSTREET_LIVE_POSE_HOST_BYTES?s->host+(a-HOST):NULL;
 if(a>=BANK&&a<HOST)return (uint64_t)a-BANK+n<=s->bank_size?(uint8_t *)(uintptr_t)(s->bank+(a-BANK)):NULL; /* read only */
 return s->ram_span(s->ram,a,n);}
static uint32_t u32(const uint8_t *p){return nflstreet_rd32(p);}
static float real(const uint8_t *p){return nflstreet_as_float(u32(p));}
void nflstreet_live_pose_reset(state *s){
 if(!s)return;nflstreet_live_locomotion_reset(&s->locomotion);memset(s->pose.local,0,sizeof s->pose.local);s->updates=s->entries=0;}
int nflstreet_live_pose_open(state *s,const uint8_t *bank,size_t n,const uint8_t *catalog,size_t cn){
 const uint8_t *profile;size_t pn;
 if(!s||!bank||n<24||n>=HOST-BANK||!nflstreet_live_locomotion_catalog_get(catalog,cn,0,&profile,&pn))return 0;
 const size_t d=u32(bank+20);if(d+24>n||!u32(bank+d)||u32(bank+d)>192)return 0;
 memset(s,0,sizeof *s);if(!nflstreet_live_locomotion_open(&s->locomotion,bank,n,profile,pn))return 0;
 s->bank=bank;s->bank_size=n;s->catalog=catalog;s->catalog_size=cn;
 nflstreet_wr32(s->host,u32(bank+d));for(unsigned k=1;k<6;++k){const uint32_t at=u32(bank+d+4*k);if(at+2u*u32(bank+d)>n)return 0;nflstreet_wr32(s->host+4*k,BANK+at);}
 return 1;}
/* 8018879C per word: the shortest 24-bit turn from b toward a by t. */
static uint32_t turn(float t,uint32_t a,uint32_t b){
 a&=0xffffffu;b&=0xffffffu;const int32_t d=(int32_t)(a-b);if(d>0x800000)b+=0x1000000u;else if(d< -0x800000)a+=0x1000000u;
 return b+(uint32_t)(int32_t)(((int64_t)(int32_t)(a-b)*nflstreet_fctiwz(t*16777216.0f))>>24);}
int nflstreet_live_pose_step(state *s,nflstreet_draw_span span,void *memory,uint32_t gameplay,unsigned set,double updates){
 if(!s||!s->bank||!span||!gameplay)return 0;s->ram_span=span;s->ram=memory;
 const uint8_t *g=span(memory,gameplay,0x30c),*profile;size_t pn;if(!g)return 0;
 uint32_t st[21],in[12];for(unsigned i=0;i<21;++i)st[i]=u32(g+0x1a8+4*i);for(unsigned i=0;i<12;++i)in[i]=u32(g+0x200+4*i);
 const nflstreet_curve_inputs input={nflstreet_as_float(st[7]),nflstreet_as_float(st[13]),nflstreet_as_float(st[16]),nflstreet_as_float(st[17]),(uint8_t)(in[7]>>8)};
 int16_t raw[384];unsigned components;
 if(!nflstreet_live_locomotion_catalog_get(s->catalog,s->catalog_size,set,&profile,&pn)||
    !nflstreet_live_locomotion_step_set(&s->locomotion,profile,pn,&input,st[6],st[8],updates,raw,384,&components))return 0;
 /* The pose list (80176650 layout) from the four channels. */
 const uint8_t *ch=s->locomotion.channels;float weights[4],fractions[4];
 for(unsigned k=0;k<4;++k){const uint8_t *c=ch+124*k;weights[k]=c[1]?real(c+0x30):0.0f;fractions[k]=c[1]?real(c+0x4c+0x14):0.0f;}
 uint8_t slots[32],which[32];float normalized[32];unsigned n;
 if(!nflstreet_locomotion_pose_weights(weights,fractions,4,slots,which,normalized,&n)||!n||n>NFLSTREET_LIVE_POSE_ENTRIES)return 0;
 nflstreet_actor_pose_context *p=&s->pose;p->span=composite;p->memory=s;
 uint8_t *list=s->host+(LIST-HOST);memset(list,0,4+0x2cu*NFLSTREET_LIVE_POSE_ENTRIES);nflstreet_wr16(list,n);
 const uint32_t table=u32(s->bank+16),frames=(uint32_t)s->bank[4]<<8|s->bank[5];
 for(unsigned i=0;i<n;++i){uint8_t *e=list+4+0x2c*i;const uint8_t *c=ch+124u*slots[i];const unsigned frame=nflstreet_rd16(c+0x4c+(which[i]?0x12:0x10));
  if(frame>=frames||table+4*frame+4>s->bank_size)return 0;const uint32_t record=RECORDS+0x100u*i;
  if(!nflstreet_actor_pose_stream(p,record,BANK+u32(s->bank+table+4*frame),DESCRIPTOR,0))return 0; /* 8015E938 */
  const float f=fractions[slots[i]];nflstreet_wr32(e,nflstreet_float_bits(weights[slots[i]]*(which[i]?f:1.0f-f)));
  nflstreet_wr32(e+4,nflstreet_float_bits(normalized[i]));e[8]=0;e[9]=slots[i];nflstreet_wr16(e+0x20,c[6]);nflstreet_wr16(e+0x22,frame);
  nflstreet_wr32(e+0x24,0);nflstreet_wr32(e+0x28,record);}
 /* 80020FDC: the visual's +18/+1C/+20 from the gameplay actor, then the blend. */
 const uint32_t visual=u32(g+4);uint8_t *v=span(memory,visual,0x24);if(!v)return 0;
 nflstreet_wr32(v+0x18,u32(g+0x300));nflstreet_wr32(v+0x1c,u32(g+0x304));nflstreet_wr32(v+0x20,g[0x308]);
 if(!nflstreet_actor_pose_blend(p,visual,LIST))return 0;
 /* Channel roots (host inference, see live_pose.h) and the clip set: one key, weight 1. */
 uint8_t *rc=s->host+(CHANNELS-HOST),*clips=s->host+(CLIPS-HOST);memset(rc,0,4*0x7c);memset(clips,0,400);
 nflstreet_wr16(clips+4,1);nflstreet_wr16(clips+6,1);nflstreet_wr32(clips+0x50,nflstreet_float_bits(1.0f));
 for(unsigned k=0;k<4;++k){const uint8_t *c=ch+124*k;uint8_t *r=rc+0x7c*k;int at[2]={-1,-1};
  for(unsigned i=0;i<n;++i)if(slots[i]==k)at[which[i]]=(int)i;if(at[0]<0&&at[1]<0)continue;
  r[1]=2;nflstreet_wr16(r+8,1);nflstreet_wr32(r+0x30,u32(c+0x30));
  for(unsigned w=0;w<3;++w){const uint8_t *prev=at[0]>=0?list+4+0x2c*at[0]:NULL,*next=at[1]>=0?list+4+0x2c*at[1]:NULL;
   uint32_t value=next&&prev?turn(fractions[k],u32(next+0xc+4*w),u32(prev+0xc+4*w)):u32((next?next:prev)+0xc+4*w);
   if(w==1&&((next?next:prev)[8]&4))value=0;nflstreet_wr32(r+0x10+4*w,value);}}
 if(!nflstreet_actor_pose_root(p,visual,CHANNELS,4,CLIPS)||!nflstreet_actor_pose_twist(p,gameplay))return 0;
 s->entries=n;++s->updates;return 1;}
