/* Controlled actual recipient callbacks. Device/profile services are captured. */
#include <windows.h>
#include "moderngekko/module_abi.h"
#include "storage_dialog.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint32_t be(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void put(uint8_t*p,uint32_t v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static int run(const ModernGekkoModuleDesc*m,CPUState*c,nflstreet_storage_dialog*s){
 c->lr=0;c->exception=0;for(unsigned i=0;c->pc&&i<1000000;++i){uint32_t f=c->pc;unsigned argc=99;
 if(f==0x800070ac){put(c->ram+c->gpr[3]-0x80000000,0x123);put(c->ram+c->gpr[4]-0x80000000,0x456);c->pc=c->lr;continue;}
 if(f==0x80006dc4){c->gpr[3]=s->query_slot;c->pc=c->lr;continue;}
 if(f==0x8018f084||f==0x8018f01c||f==0x8018f120)argc=1;
 else if(f==0x8018f150)argc=0;
 else if(f==0x8014e134)argc=2;
 if(argc!=99){if(s->count>=16)return 0;nflstreet_storage_request*q=s->requests+s->count++;q->function=f;q->argc=argc;for(unsigned j=0;j<argc;++j)q->args[j]=c->gpr[3+j];c->pc=c->lr;continue;}
 c->downcount=-1000000000;m->dispatch(c,c->pc);if(c->exception)return 0;
 }return !c->pc;
}
int main(int argc,char**argv){if(argc!=3)return 2;HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;const ModernGekkoModuleDesc*m=get();if(m->cpu_state_size!=sizeof(CPUState))return 3;
 uint8_t*ram=malloc(24*1024*1024),*seed=malloc(24*1024*1024);FILE*f=fopen(argv[2],"rb");if(!ram||!seed||!f||fread(seed,1,24*1024*1024,f)!=24*1024*1024)return 4;fclose(f);
 static const uint32_t values[]={0xffffffff,0,1,2,0x12345678};unsigned cases=0;
 for(unsigned kind=0;kind<6;++kind)for(unsigned a=0;a<5;++a)for(unsigned b=0;b<5;++b){memcpy(ram,seed,24*1024*1024);nflstreet_storage_dialog native={0},actual={0};native.word_5a10=values[a];native.word_573c=values[b];native.query_slot=values[(a+b)%5];for(unsigned j=0;j<5;++j){native.source_callbacks[j]=be(seed+0x2829d0+j*4);native.callbacks[j]=0xabcd0000+j;put(ram+0x3a9a40-0x6ee8+j*4,native.callbacks[j]);}actual=native;put(ram+0x3a9a40-0x5a10,native.word_5a10);put(ram+0x3a9a40-0x573c,native.word_573c);put(ram+0x17f0000,values[b]);put(ram+0x17f0004,0);
 CPUState c={0};c.ram=ram;c.ram_size=24*1024*1024;c.msr=0x2000;c.gpr[1]=0x81700000;c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;c.gpr[3]=kind==5?0:0x80000002+kind;c.gpr[4]=kind==5?0x817f0004:0x817f0000;c.pc=kind==5?0x80121f1c:0x80122140;if(!run(m,&c,&actual))return 5;
 uint32_t result=0;if(kind==5){if(!nflstreet_storage_select(&native,&result)||result!=be(ram+0x17f0004))return 6;}else if(!nflstreet_storage_common(&native,0x80000002+kind,values[b],&result)||result!=c.gpr[3])return 6;
 if(native.word_5a10!=be(ram+0x3a9a40-0x5a10)||native.word_573c!=be(ram+0x3a9a40-0x573c)||native.count!=actual.count||memcmp(native.requests,actual.requests,sizeof native.requests)){fprintf(stderr,"Mismatch kind%u a%u b%u\n",kind,a,b);return 7;}
 for(unsigned j=0;j<5;++j)if(native.callbacks[j]!=be(ram+0x3a9a40-0x6ee8+j*4))return 8;++cases;
 }
 nflstreet_storage_dialog s={0},before;s.count=16;s.word_5a10=0xffffffff;before=s;uint32_t result=77;if(nflstreet_storage_common(&s,0x80000002,0,&result)||memcmp(&s,&before,sizeof s)||result!=77)return 9;
 if(nflstreet_storage_select(&s,&result)||memcmp(&s,&before,sizeof s)||result!=77)return 10;
 printf("Original group6/entry4 common lifecycle and selector exact:%u cases;raw slot/callback fields,ordered retained service requests,delay9999;2 capacity rollback guards;async polling/card I/O/UI not executed\n",cases);free(ram);free(seed);FreeLibrary(h);return 0;
}
