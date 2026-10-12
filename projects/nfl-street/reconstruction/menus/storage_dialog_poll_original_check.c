/* Integration oracle for original80121FF8; explicit bounded device responses. */
#include <windows.h>
#include "moderngekko/module_abi.h"
#include "storage_dialog.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint32_t be(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void put(uint8_t*p,uint32_t v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static int run(const ModernGekkoModuleDesc*m,CPUState*c,const nflstreet_storage_poll_inputs*in,nflstreet_storage_dialog*s){
 unsigned sample=0,slotcalls=0;c->lr=0;c->exception=0;
 for(unsigned i=0;c->pc&&i<1000000;++i){uint32_t f=c->pc;unsigned argc=99;uint32_t a=0,b=0,value=0;
 if(f==0x800070ac){put(c->ram+c->gpr[3]-0x80000000,in->desired[0]);put(c->ram+c->gpr[4]-0x80000000,in->desired[1]);argc=0;}
 else if(f==0x80006f64){if(sample>=in->sample_count||c->gpr[3]||c->gpr[4]!=sample||c->gpr[7])return 0;put(c->ram+c->gpr[5]-0x80000000,in->samples[sample][0]);put(c->ram+c->gpr[6]-0x80000000,in->samples[sample][1]);argc=2;b=sample++;}
 else if(f==0x80006dc4){argc=slotcalls++?2:0;if(argc){a=c->gpr[3];b=c->gpr[4];}value=in->slot;}
 else if(f==0x80006de4){argc=2;a=c->gpr[3];b=c->gpr[4];value=in->status;}
 else if(f==0x80006d6c){argc=2;a=c->gpr[3];b=c->gpr[4];value=in->type2_query;}
 else if(f==0x8018f5e8){argc=1;a=c->gpr[4];memset(c->ram+c->gpr[3]-0x80000000,0x5a,32);}
 else if(f==0x8018f7b8){if(c->ram[c->gpr[5]-0x80000000])return 0;argc=1;a=c->gpr[4];value=in->probe_result;}
 if(argc!=99){if(s->count>=16)return 0;s->requests[s->count++]=(nflstreet_storage_request){f,argc,{a,b}};c->gpr[3]=value;c->pc=c->lr;continue;}
 c->downcount=-1000000000;m->dispatch(c,c->pc);if(c->exception)return 0;
 }return !c->pc;
}
int main(int argc,char**argv){if(argc!=3)return 2;HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;const ModernGekkoModuleDesc*m=get();if(m->cpu_state_size!=sizeof(CPUState))return 3;
 uint8_t*ram=malloc(24*1024*1024),*seed=malloc(24*1024*1024);FILE*f=fopen(argv[2],"rb");if(!ram||!seed||!f||fread(seed,1,24*1024*1024,f)!=24*1024*1024)return 4;fclose(f);
 unsigned cases=0;for(unsigned type=0;type<4;++type)for(unsigned delay=1;delay<=3;delay+=2)for(unsigned slot=0;slot<2;++slot)for(unsigned q=0;q<2;++q)for(unsigned probe=0;probe<2;++probe){
 memcpy(ram,seed,24*1024*1024);nflstreet_storage_poll_inputs in={0};in.desired[0]=type;in.desired[1]=0x456;in.sample_count=delay;for(unsigned i=0;i<delay;++i){in.samples[i][0]=i==delay-1?type:0xff;in.samples[i][1]=i==delay-1?0x456:i;}in.slot=slot?0xffffffff:1;in.status=0x12345678;in.type2_query=q;in.probe_result=probe?0xffffffff:7;
 nflstreet_storage_dialog native={0},actual={0};nflstreet_storage_poll_output out={0};CPUState c={0};c.ram=ram;c.ram_size=24*1024*1024;c.msr=0x2000;c.gpr[1]=0x81700000;c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;for(unsigned j=0;j<4;++j)c.gpr[3+j]=0x817f0000+j*4;c.pc=0x80121ff8;
 if(!run(m,&c,&in,&actual)||c.gpr[3]!=1)return 5;if(!nflstreet_storage_poll(&native,&in,&out))return 6;
 if(out.slot!=be(ram+0x17f0000)||out.type!=be(ram+0x17f0004)||out.status!=be(ram+0x17f0008)||out.id!=be(ram+0x17f000c)||out.iterations!=be(ram+0x3a9a40-0x5a14)||native.count!=actual.count||memcmp(native.requests,actual.requests,sizeof native.requests)){fprintf(stderr,"Poll differs:type%u delay%u slot%u q%u probe%u\n",type,delay,slot,q,probe);return 7;}++cases;
 }
 nflstreet_storage_dialog guard={0},before=guard;nflstreet_storage_poll_inputs in={0};in.sample_count=1;in.desired[0]=1;nflstreet_storage_poll_output out={1,2,3,4,5},old=out;if(nflstreet_storage_poll(&guard,&in,&out)||memcmp(&guard,&before,sizeof guard)||memcmp(&out,&old,sizeof out))return 8;
 guard.count=16;before=guard;in.desired[0]=0;if(nflstreet_storage_poll(&guard,&in,&out)||memcmp(&guard,&before,sizeof guard)||memcmp(&out,&old,sizeof out))return 9;
 printf("Original80121FF8 polling exact:%u supplied-response cases;output slot/type/status/id,counter,ordered scalar service boundaries;2 bounded/queue rollback guards;actual device I/O and recipient UI separate\n",cases);free(ram);free(seed);FreeLibrary(h);return 0;
}
