/* Original full rate construction/update/cancellation. VM service boundary is
   captured and supplies an explicit multiplier; never a full menu-flow claim. */
#include <windows.h>
#include "moderngekko/module_abi.h"
#include "rate.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned be(const uint8_t*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static void put(uint8_t*p,unsigned v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static unsigned word(float f){unsigned w;memcpy(&w,&f,4);return w;}
typedef struct {unsigned screen,root,script,argc,args[3];} request;
typedef struct {request requests[256];unsigned count;float multiplier;} effects;
static int service(void*ctx,nflstreet_ui_rates*rates,uint8_t*owned,size_t size,unsigned base,unsigned screen,unsigned root,unsigned script,unsigned argc,const unsigned*args,float*result){(void)rates;(void)owned;(void)size;(void)base;effects*e=ctx;if(e->count==256||argc>3)return 0;request*r=e->requests+e->count++;memset(r,0,sizeof *r);r->screen=screen;r->root=root;r->script=script;r->argc=argc;memcpy(r->args,args,argc*4);*result=e->multiplier;return 1;}
static int execute(const ModernGekkoModuleDesc*m,CPUState*c,effects*e,unsigned fn){c->pc=fn;c->lr=0;c->exception=0;c->downcount=c->cycle_budget=100000000;
 for(unsigned i=0;c->pc&&i<1000000;++i){if(c->pc==0x801d399c){unsigned args[3]={0};if(c->gpr[8]>3)return 0;for(unsigned j=0;j<c->gpr[8];++j)args[j]=be(c->ram+c->gpr[9]-0x80000000+j*4);float result;if(!service(e,NULL,NULL,0,0,c->gpr[4],c->gpr[5],c->gpr[7],c->gpr[8],args,&result))return 0;
   /* Twelfth ABI argument is result pointer at caller SP+20. */
   unsigned p=be(c->ram+c->gpr[1]-0x80000000+0x14);if(p)put(c->ram+p-0x80000000,word(result));c->gpr[3]=0;c->pc=c->lr;
  }else m->dispatch(c,c->pc);if(c->exception)break;
 }return !c->pc&&!c->exception;
}
static void store(uint8_t*ram,const nflstreet_ui_rates*s){put(ram+0x17f0004,s->flags);put(ram+0x17f0008,s->clock);put(ram+0x17f0054,s->count);put(ram+0x17f0058,0x817e0000);for(unsigned i=0;i<64;++i)for(unsigned j=0;j<13;++j)put(ram+0x17e0000+i*52+j*4,s->records[i].words[j]);}
static int same(uint8_t*ram,const nflstreet_ui_rates*s){if(be(ram+0x17f0004)!=s->flags||be(ram+0x17f0054)!=s->count)return 0;for(unsigned i=0;i<64;++i)for(unsigned j=0;j<13;++j)if(be(ram+0x17e0000+i*52+j*4)!=s->records[i].words[j]){fprintf(stderr,"Rate record %u word %u native=%08x original=%08x\n",i,j,s->records[i].words[j],be(ram+0x17e0000+i*52+j*4));return 0;}return 1;}
int main(int argc,char**argv){if(argc!=3)return 2;HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;const ModernGekkoModuleDesc*m=get();if(m->cpu_state_size!=sizeof(CPUState))return 3;
 uint8_t*ram=malloc(24*1024*1024);FILE*f=fopen(argv[2],"rb");if(!ram||!f||fread(ram,1,24*1024*1024,f)!=24*1024*1024)return 4;fclose(f);
 CPUState c={0};c.gpr[1]=0x81700000;c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;c.ram=ram;c.ram_size=24*1024*1024;c.msr=0x2000;
 unsigned constructors=0,updates=0,cancels=0,requests=0;unsigned properties[]={0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0,UINT32_MAX};
 for(unsigned p=0;p<sizeof properties/sizeof *properties;++p)for(unsigned sign=0;sign<2;++sign){uint8_t object[256]={0};nflstreet_ui_rates s={0};s.clock=16;s.flags=0x81;float start=sign?1:0,target=sign?0:1;put(object+nflstreet_ui_property_offset(properties[p]),word(start));memcpy(ram+0x1100000,object,sizeof object);memset(ram+0x17f0000,0,0x200);memset(ram+0x17f1000,0,20);store(ram,&s);
  if(!nflstreet_ui_rate_add(&s,object,sizeof object,0x81100000,0x817f1000,0x81100000,19,0x81100000,properties[p],target,64,0x81100800,0))return 5;
  c.gpr[3]=0x817f0000;c.gpr[4]=0x817f1000;c.gpr[5]=0x81100000;c.gpr[6]=0x81100000;c.gpr[7]=19;c.gpr[8]=0x81100800;c.gpr[9]=0;c.gpr[10]=64;put(ram+0x1700008,properties[p]);c.fpr[1]=target;effects e={0};if(!execute(m,&c,&e,0x801d1348)||!same(ram,&s))return 6;++constructors;
  /* Replace matching identity, then construct a second distinct slot. */
  for(unsigned j=0;j<2;++j){unsigned id=j?20:19;if(!nflstreet_ui_rate_add(&s,object,sizeof object,0x81100000,0x817f1000,0x81100000,id,0x81100000,properties[p],target,64,0x81100800,0))return 5;c.gpr[3]=0x817f0000;c.gpr[4]=0x817f1000;c.gpr[5]=0x81100000;c.gpr[6]=0x81100000;c.gpr[7]=id;c.gpr[8]=0x81100800;c.gpr[9]=0;c.gpr[10]=64;put(ram+0x1700008,properties[p]);c.fpr[1]=target;if(!execute(m,&c,&e,0x801d1348)||!same(ram,&s))return 6;++constructors;}
  for(unsigned j=0;j<5;++j){effects a={0},b={0};a.multiplier=b.multiplier=1;c.gpr[3]=0x817f0000;c.gpr[4]=16;if(!nflstreet_ui_rate_update(&s,object,sizeof object,0x81100000,0x817f1000,16,0,service,&a)||!execute(m,&c,&b,0x801cf610)||!same(ram,&s)||memcmp(object,ram+0x1100000,sizeof object)||a.count!=b.count||memcmp(a.requests,b.requests,a.count*sizeof *a.requests)){fprintf(stderr,"Update mismatch property=%x sign=%u step=%u requests=%u/%u\n",properties[p],sign,j,a.count,b.count);return 7;}++updates;requests+=a.count;}
  c.gpr[3]=0x817f0000;c.gpr[4]=0x817f1000;if(!nflstreet_ui_rate_cancel(&s,0x817f1000)||!execute(m,&c,&e,0x801cf590)||!same(ram,&s))return 8;++cancels;
 }
 /* Script-driven timers and multi-interval updates, including clamp of negative result. */
 for(unsigned mode=0;mode<4;++mode){uint8_t object[256]={0};nflstreet_ui_rates s={0};s.count=2;s.clock=16;s.flags=0x81;
  for(unsigned i=0;i<2;++i){unsigned r[13]={i+1,0x81100800,0,0,16,0,0x81100000,0x817f1000,mode==3?0:0x24,word(1),word(.25f),0x81100900,0x81100000};memcpy(s.records[i].words,r,sizeof r);}memcpy(ram+0x1100000,object,sizeof object);memset(ram+0x17f0000,0,0x200);store(ram,&s);
  for(unsigned j=0;j<4;++j){effects a={0},b={0};a.multiplier=b.multiplier=mode==0?0:mode==1?.5f:mode==2?-1:1;c.gpr[3]=0x817f0000;c.gpr[4]=48;if(!nflstreet_ui_rate_update(&s,object,sizeof object,0x81100000,0x817f1000,48,0,service,&a)||!execute(m,&c,&b,0x801cf610)||!same(ram,&s)||memcmp(object,ram+0x1100000,sizeof object)||a.count!=b.count||memcmp(a.requests,b.requests,a.count*sizeof *a.requests)){fprintf(stderr,"Timer mismatch mode=%u step=%u count=%u/%u\n",mode,j,a.count,b.count);return 9;}++updates;requests+=a.count;}
 }
 /* Actual801D1490 preserves unused words;801D1564 marks only first match. */
 for(unsigned mode=0;mode<8;++mode){nflstreet_ui_rates s={0};s.count=2;s.clock=16;
  for(unsigned i=0;i<2;++i)for(unsigned j=0;j<13;++j)s.records[i].words[j]=j+100*i;
  s.records[0].words[0]=17;s.records[0].words[6]=0x81100000;s.records[1].words[0]=18;s.records[1].words[6]=0x81100000;store(ram,&s);memset(ram+0x17f1000,0,20);
  nflstreet_ui_service_request q={0};q.kind=4;q.argc=4;q.args[0]=0x81100000;q.args[1]=mode?100:16;q.args[2]=0x800;q.args[3]=mode<4?17:19;
  c.gpr[3]=0x817f0000;c.gpr[4]=0x817f1000;c.gpr[5]=q.args[0];c.gpr[6]=q.args[3];c.gpr[7]=0x81100800;c.gpr[8]=q.args[1];effects e={0};
  uint8_t object[4096]={0};if(!nflstreet_ui_rate_request(&s,object,sizeof object,0x81100000,0x817f1000,&q)||!execute(m,&c,&e,0x801d1490)||!same(ram,&s))return 10;++constructors;
  q.kind=5;q.argc=2;q.args[1]=mode&1?99:17;c.gpr[3]=0x817f0000;c.gpr[4]=q.args[0];c.gpr[5]=q.args[1];
  if(!nflstreet_ui_rate_request(&s,object,sizeof object,0x81100000,0x817f1000,&q)||!execute(m,&c,&e,0x801d1564)||!same(ram,&s))return 11;++cancels;
 }
 printf("Original rate stages exact: %u constructions/replacements, %u full-record/object updates, %u cancellations, %u ordered script requests; script results explicitly supplied, full menu activation not claimed\n",constructors,updates,cancels,requests);free(ram);FreeLibrary(h);return 0;}
