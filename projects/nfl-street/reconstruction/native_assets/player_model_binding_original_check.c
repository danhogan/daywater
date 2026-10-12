#define main bounded_model_checker_main
#include "actor_model_init_original_check.c"
#undef main
#include "player_model_binding.h"
#include "player_model_binding_check_fixture.h"
static int binding_original(const ModernGekkoModuleDesc*m,binding_fixture*f){
 CPUState c={0};c.ram=f->ram;c.ram_size=RAM_SIZE;c.msr=0x2000;c.gpr[1]=0x81700000u;c.gpr[2]=0x803b9a40u;c.gpr[13]=0x803a9a40u;c.gpr[3]=ACTOR;c.pc=0x801630b0u;c.cycle_budget=200000000;
 for(unsigned step=0;c.pc&&step<1000000;++step){int handled=binding_boundary(&c,f);if(handled<0)return 0;if(handled)continue;
  if(c.pc==0x801c9784u){uint32_t token=0;nflstreet_player_model_span span;if(!binding_allocate(f,c.gpr[3],c.gpr[4],&token,&span))return 0;c.gpr[3]=token;c.pc=c.lr;continue;}
  if(c.pc==0x801c97c4u){if(!binding_release(f,c.gpr[3]))return 0;c.pc=c.lr;continue;}
  c.downcount=-1000000000;m->dispatch(&c,c.pc);if(c.exception){fprintf(stderr,"model binding exception%08x pc%08x\n",c.exception,c.pc);return 0;}}
 return c.pc==0;
}
static void bind_finish(binding_fixture*f){for(unsigned i=0;i<f->allocations;++i)memcpy(f->blocks[i].bytes,ptr(f->ram,f->blocks[i].token,BIND_STRIDE),BIND_STRIDE);}
static int binding_case(const ModernGekkoModuleDesc*m,const uint8_t*baseline,unsigned prefill,unsigned null_initial,unsigned shared_present,unsigned mutation){
 binding_fixture*o=calloc(1,sizeof *o),*n=calloc(1,sizeof *n);if(!o||!n){free(o);free(n);return 0;}o->ram=malloc(RAM_SIZE);n->ram=malloc(RAM_SIZE);if(!o->ram||!n->ram){free(o->ram);free(n->ram);free(o);free(n);return 0;}
 memcpy(o->ram,baseline,RAM_SIZE);uint8_t*slot=ptr(o->ram,0x803a9a40u-0x7bbcu,4);uint8_t*list=slot?ptr(o->ram,be(slot),4):NULL;uint8_t*source=list?ptr(o->ram,be(list),NFLSTREET_ACTOR_MODEL_STORAGE):NULL;if(!source)return 0;
 o->resources[0]=(uint32_t)(source-o->ram)+BASE+0x430;o->resources[1]=(uint32_t)(source-o->ram)+BASE+0x474;o->null_initial=n->null_initial=null_initial;o->mutation=n->mutation=mutation;
 uint8_t*a=ptr(o->ram,ACTOR,NFLSTREET_ACTOR_MODEL_STORAGE);for(unsigned i=0;i<NFLSTREET_ACTOR_MODEL_STORAGE;++i)a[i]=prefill?(uint8_t)(i*17+0x39):0;
 put(a+0x400,be(source+0x400));put(a+0x404,be(source+0x404));put(a+0x410,be(source+0x410));put(a+0x414,be(source+0x414));a[0x1362]=source[0x1362];a[0x1364]=source[0x1364];
 if(!shared_present)memset(ptr(o->ram,SHARED_TOKEN,68),0,68);memcpy(n->ram,o->ram,RAM_SIZE);memcpy(n->resources,o->resources,sizeof n->resources);
 if(!binding_original(m,o)){fprintf(stderr,"originalbindingexecutionfailed\n");return 0;}
 nflstreet_player_model_binding_context ctx={be(ptr(n->ram,0x803a9a40u-0x685cu,4)),TABLE_TOKEN,ptr(n->ram,SCRATCH_TOKEN,76),76,ptr(n->ram,SHARED_TOKEN,68),68,binding_resolve,n,binding_allocate,n,binding_release,n,binding_service,n};
 int ok=nflstreet_player_model_bind(ptr(n->ram,ACTOR,NFLSTREET_ACTOR_MODEL_STORAGE),NFLSTREET_ACTOR_MODEL_STORAGE,&ctx);bind_finish(o);bind_finish(n);
 memset(ptr(o->ram,0x816f0000u,0x20000),0,0x20000);memset(ptr(n->ram,0x816f0000u,0x20000),0,0x20000);
 if(ok)ok=o->count==n->count&&o->allocations==n->allocations&&!memcmp(o->requests,n->requests,sizeof o->requests)&&!memcmp(o->blocks,n->blocks,sizeof o->blocks)&&!memcmp(o->ram,n->ram,RAM_SIZE);
 if(!ok){fprintf(stderr,"binding mismatch prefill%u null%u shared%u mutation%u req%u/%u alloc%u/%u\n",prefill,null_initial,shared_present,mutation,o->count,n->count,o->allocations,n->allocations);
  for(unsigned i=0;i<RAM_SIZE;++i)if(o->ram[i]!=n->ram[i]){fprintf(stderr,"firstRAMbyte%08x original%02x native%02x\n",BASE+i,o->ram[i],n->ram[i]);break;}
  for(unsigned i=0;i<o->count&&i<n->count;++i)if(memcmp(o->requests+i,n->requests+i,sizeof(binding_request))){fprintf(stderr,"firstreq%u kinds%u/%u\n",i,o->requests[i].request.kind,n->requests[i].request.kind);break;}}
 else printf("model-binding prefilled=%u null=%u shared=%u mutation=%u: actor1A90 +%u allocations +%u ordered requests +catalog/shared/source mutations exact\n",prefill,null_initial,shared_present,mutation,o->allocations,o->count);
 free(o->ram);free(n->ram);free(o);free(n);return ok;
}
int main(int argc,char**argv){
 if(argc!=3)return 2;HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;const ModernGekkoModuleDesc*m=get();if(!m||m->cpu_state_size!=sizeof(CPUState))return 3;
 uint8_t*baseline=load(argv[2]);if(!baseline)return 4;int ok=1;
 for(unsigned p=0;p<2&&ok;++p)for(unsigned z=0;z<2&&ok;++z)for(unsigned s=0;s<2&&ok;++s)for(unsigned t=0;t<2&&ok;++t)ok=binding_case(m,baseline,p,z,s,t);
 free(baseline);FreeLibrary(h);if(ok)puts("Original801630B0 model binding differential passed16 cases; actual source model records, explicit cache/render/name services, no complete asset loading or renderer.");return ok?0:5;
}
