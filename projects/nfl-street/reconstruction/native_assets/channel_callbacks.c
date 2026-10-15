/* See channel_callbacks.md. Generated bodies are native C basic blocks,
   with constant call targets and explicit operations, NOT a runtime opcode
   decoder or a call into the original module. Keep operation order intact. */
#include "channel_callbacks.h"
#include "gekko_float.h"
#include <limits.h>
#include <stdlib.h>
#define CB_R13 0x803a9a40u
typedef struct {
 nflstreet_channel_callbacks *out;nflstreet_actor_channels *channels;
 uint32_t r[32],lr,ctr,cr,ca,pc;double f[32],ps[32];unsigned budget,depth;
} cb_machine;
static void cb_fail(cb_machine *s,uint32_t address){
 if(s->out->ok){s->out->failure_pc=s->pc;s->out->failure_address=address;}
 s->out->ok=0;s->channels->ok=0;
}
static int cb_tick(cb_machine *s,uint32_t pc){s->pc=pc;if(!s->out->ok)return 0;
 if(!s->budget--){s->out->failure_budget_exhausted=1;cb_fail(s,pc);return 0;}return 1;}
static uint8_t *cb_mem(cb_machine *s,uint32_t a,uint32_t n){
 if(a>=NFLSTREET_CALLBACK_STACK_LO&&(uint64_t)a+n<=NFLSTREET_CALLBACK_STACK_HI)
  return s->out->stack+(a-NFLSTREET_CALLBACK_STACK_LO);
 uint8_t *p=s->channels->span(s->channels->memory,a,n);if(!p)cb_fail(s,a);return p;
}
static uint32_t cb_load(cb_machine *s,uint32_t a,unsigned n){uint8_t *p=cb_mem(s,a,n);uint32_t v=0;
 if(p)for(unsigned i=0;i<n;++i)v=(v<<8)|p[i];return v;}
static void cb_store(cb_machine *s,uint32_t a,uint32_t v,unsigned n){uint8_t *p=cb_mem(s,a,n);
 if(p)for(unsigned i=n;i;--i){p[i-1]=(uint8_t)v;v>>=8;}}
static double cb_lfd(cb_machine *s,uint32_t a){const uint64_t hi=cb_load(s,a,4),lo=cb_load(s,a+4,4);return nflstreet_gk_value((hi<<32)|lo);}
static double cb_lfs(cb_machine *s,uint32_t a){uint32_t b=cb_load(s,a,4);float f;memcpy(&f,&b,4);return (double)f;}
static void cb_stfd(cb_machine *s,uint32_t a,double v){uint64_t b=nflstreet_gk_bits(v);cb_store(s,a,(uint32_t)(b>>32),4);cb_store(s,a+4,(uint32_t)b,4);}
static void cb_stfs(cb_machine *s,uint32_t a,double v){float f=(float)v;uint32_t b;memcpy(&b,&f,4);cb_store(s,a,b,4);}
static double cb_fctiwz(double v){const uint32_t r=(uint32_t)nflstreet_gk_fctiwz(v);
 return nflstreet_gk_value(0xfff8000000000000ull|r|((r==0&&signbit(v))?0x100000000ull:0));}
static uint32_t cb_rot(uint32_t v,unsigned n){n&=31;return n?(v<<n)|(v>>(32-n)):v;}
static uint32_t cb_mask(unsigned lo,unsigned hi){uint32_t m=0;for(unsigned i=lo;;i=(i+1)&31){m|=0x80000000u>>i;if(i==hi)break;}return m;}
static void cb_cmp(cb_machine *s,unsigned field,int64_t a,int64_t b){const unsigned shift=28-4*field;
 s->cr=(s->cr&~(15u<<shift))|((a<b?8u:a>b?4u:2u)<<shift);}
static void cb_fcmp(cb_machine *s,unsigned field,double a,double b){const unsigned shift=28-4*field;
 const uint32_t v=isnan(a)||isnan(b)?1u:a<b?8u:a>b?4u:2u;s->cr=(s->cr&~(15u<<shift))|(v<<shift);}
static uint32_t cb_bit(cb_machine *s,unsigned bit){return (s->cr>>(31-bit))&1u;}
static void cb_setbit(cb_machine *s,unsigned bit,uint32_t v){const uint32_t mask=0x80000000u>>bit;s->cr=(s->cr&~mask)|(v?mask:0);}
static unsigned cb_clz(uint32_t v){unsigned n=0;if(!v)return 32;while(!(v&0x80000000u)){++n;v<<=1;}return n;}
static uint32_t cb_sra(cb_machine *s,uint32_t v,unsigned n){const unsigned k=n>31?31:n;
 s->ca=(v>>31)&&n&&(n>=32?v:(v&((1u<<n)-1u)));return (uint32_t)((int32_t)v>>k);}
static uint32_t cb_div(uint32_t a,uint32_t b,int sign){if(!b)return 0;if(sign){if(a==0x80000000u&&b==0xffffffffu)return a;return (uint32_t)((int32_t)a/(int32_t)b);}return a/b;}
static void cb_addcarry(cb_machine *s,unsigned d,uint32_t a,uint32_t b,uint32_t carry){uint64_t v=(uint64_t)a+b+carry;s->r[d]=(uint32_t)v;s->ca=(uint32_t)(v>>32);}
static void cb_mtcr(cb_machine *s,unsigned mask,uint32_t v){for(unsigned i=0;i<8;++i)if(mask&(0x80u>>i)){const uint32_t m=15u<<(28-4*i);s->cr=(s->cr&~m)|(v&m);}}
static __attribute__((noinline)) void cb_dispatch(cb_machine *,uint32_t);
/* The source-derived functions, including indirect curve targets. */
#include "channel_callbacks_native.inc"
void nflstreet_channel_callbacks_init(nflstreet_channel_callbacks *c){if(c){memset(c,0,sizeof *c);c->ok=1;}}
static void cb_begin(cb_machine *s,nflstreet_channel_callbacks *out,nflstreet_actor_channels *c,uint32_t sp){
 memset(s,0,sizeof *s);s->out=out;s->channels=c;s->r[1]=sp;s->r[2]=0x803b9a40u;s->r[13]=CB_R13;s->budget=5000000u;
}
uint32_t nflstreet_channel_callbacks_event(void *u,nflstreet_actor_channels *c,uint32_t actor,uint32_t command,uint32_t key){
 nflstreet_channel_callbacks *out=u;if(!out||!c)return 0;if(!out->ok){c->ok=0;return 0;}
 cb_machine s;cb_begin(&s,out,c,0x80f46ec0u);s.r[3]=actor;s.r[4]=command;s.r[5]=key;s.lr=0x80173a64u;
 cb_dispatch(&s,0x8001532cu);return out->ok?s.r[3]:0;
}
void nflstreet_channel_callbacks_group(void *u,nflstreet_actor_channels *c,uint32_t fn,uint32_t res,uint32_t key,uint32_t set,uint32_t ch,uint32_t actor,uint32_t mode){
 nflstreet_channel_callbacks *out=u;if(!out||!c)return;if(!out->ok){c->ok=0;return;}
 cb_machine s;cb_begin(&s,out,c,mode==3?0x80f46f10u:0x80f46f28u);
 s.r[3]=res;s.r[4]=key;s.r[5]=set;s.r[6]=ch;s.r[7]=actor;s.r[8]=mode;s.lr=0x8017793cu;
 cb_dispatch(&s,fn);
}
