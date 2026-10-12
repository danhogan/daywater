#include "player_appearance_check_fixture.h"
#include "player_bound_startup.h"
#include "player_model_binding_check_fixture.h"
#ifndef NFLSTREET_CHECK_FACTORY
#define NFLSTREET_CHECK_FACTORY 1
#endif
#if NFLSTREET_CHECK_FACTORY
#define FACTORY_TABLE 0x815b0000u
static uint32_t factory_owner(const uint8_t *ram){
    uint32_t table=be(ram+(0x803a9a40u-0x4f14u-BASE));
    const uint8_t *record=ptr((uint8_t *)ram,table,20);
    return record&&be(record)==NFLSTREET_ACTOR_MODEL_STORAGE?be(record+0x10):0;
}
typedef struct {uint8_t *bytes;uint32_t owner,calls;} object_allocation;
static int allocate_object(void *opaque,uint32_t owner,uint32_t size,nflstreet_factory_memory *memory){
    object_allocation *a=opaque;if(owner!=a->owner||size!=NFLSTREET_ACTOR_MODEL_STORAGE||a->calls++)return 0;
    *memory=(nflstreet_factory_memory){ACTOR,a->bytes,NFLSTREET_ACTOR_MODEL_STORAGE};return 1;
}
#endif
typedef struct {allocations model;appearance_trace appearance;binding_fixture binding;uint32_t order[4096];unsigned order_count;} bound_trace;
static int model_allocate_bound(void*opaque,const nflstreet_actor_model_allocation*q,nflstreet_actor_model_memory*out){
 bound_trace*t=opaque;if(!allocate(&t->model,q,out))return 0;
 out->bytes=ptr(t->binding.ram,out->token,HEAP_STRIDE);memset(out->bytes,0xa5,HEAP_STRIDE);return 1;
}
static int appearance_allocate_bound(void*opaque,nflstreet_player_appearance_allocation_kind kind,uint32_t count,uint32_t stride,nflstreet_player_appearance_memory*out){
 bound_trace*t=opaque;if(!appearance_allocate(&t->appearance,kind,count,stride,out))return 0;
 out->bytes=ptr(t->binding.ram,out->token,512);memset(out->bytes,0xa5,512);return 1;
}
static int appearance_service_bound(void*opaque,const nflstreet_player_appearance_request*q,uint8_t*a,size_t size,uint32_t*out){
 bound_trace*t=opaque;if(q->kind==NFLSTREET_APPEARANCE_BIND_MODEL)return 0;if(t->order_count>=4096)return 0;t->order[t->order_count++]=0x10000u+(uint32_t)q->kind;return appearance_service(&t->appearance,q,a,size,out);
}
static int binding_service_bound(void*opaque,const nflstreet_player_model_request*q,uint8_t*a,size_t size,uint32_t*out){
 bound_trace*t=opaque;if(t->order_count>=4096)return 0;t->order[t->order_count++]=0x20000u+(uint32_t)q->kind;return binding_service(&t->binding,q,a,size,out);
}
static void bound_finish(bound_trace*t){
 for(unsigned i=0;i<t->model.count;++i)memcpy(t->model.blocks[i].storage,ptr(t->binding.ram,HEAP+i*HEAP_STRIDE,HEAP_STRIDE),HEAP_STRIDE);
 memcpy(t->appearance.node_memory,ptr(t->binding.ram,NODE_HEAP,512),512);memcpy(t->appearance.channel_memory,ptr(t->binding.ram,CHANNEL_HEAP,512),512);
 for(unsigned i=0;i<t->binding.allocations;++i)memcpy(t->binding.blocks[i].bytes,ptr(t->binding.ram,t->binding.blocks[i].token,BIND_STRIDE),BIND_STRIDE);
}
static int bound_original(const ModernGekkoModuleDesc*m,bound_trace*t,const uint8_t*initial,const uint8_t*descriptor){
 uint8_t*ram=t->binding.ram;memcpy(ptr(ram,ACTOR,NFLSTREET_ACTOR_MODEL_STORAGE),initial,NFLSTREET_ACTOR_MODEL_STORAGE);memcpy(ptr(ram,DESCRIPTOR,DESCRIPTOR_SIZE),descriptor,DESCRIPTOR_SIZE);
 put(ptr(ram,MODEL_TOKEN+0x18,4),NODE_LIST_TOKEN);put(ptr(ram,0x815d0200u+0x18,4),0x11330000u);for(unsigned i=0;i<2;++i)put(ptr(ram,SIDE_TOKEN+i*0x1000u+0x18,4),0x11220000u+i);
 put(ptr(ram,0x803a9a40u-0x58e8u,4),2);put(ptr(ram,0x803a9a40u-0x6238u,4),3);memset(ptr(ram,REGISTRY_TOKEN,REGISTRY_SIZE),0xa5,REGISTRY_SIZE);memset(ptr(ram,BINDINGS_TOKEN,REGISTRY_SIZE),0xa5,REGISTRY_SIZE);
 uint32_t owner=factory_owner(ram);if(!owner)return 0;put(ptr(ram,0x803a9a40u-0x4f14u,4),FACTORY_TABLE);uint8_t*record=ptr(ram,FACTORY_TABLE,20);memset(record,0,20);put(record,NFLSTREET_ACTOR_MODEL_STORAGE);put(record+8,0x800e1f7cu);put(record+0x10,owner);
 CPUState c={0};c.ram=ram;c.ram_size=RAM_SIZE;c.msr=0x2000;c.gpr[1]=0x81700000u;c.gpr[2]=0x803b9a40u;c.gpr[13]=0x803a9a40u;c.gpr[3]=0;c.gpr[4]=0;c.gpr[5]=DESCRIPTOR;c.pc=0x801956a4u;c.cycle_budget=200000000;
 uint32_t channel_count=0;int channel_request=0;unsigned object_calls=0,factory_clear=0,initial_clear=0;
 for(unsigned step=0;c.pc&&step<1500000;++step){
  unsigned app_before=t->appearance.count,bind_before=t->binding.count;int handled=0;
  if(c.pc==0x800e0734u&&c.gpr[3])handled=binding_boundary(&c,&t->binding);
  else if(c.pc!=0x801630b0u)handled=appearance_boundary(&c,ram,&t->appearance);
  if(!handled&&c.pc!=0x800e0734u)handled=binding_boundary(&c,&t->binding);
  if(handled<0)return 0;if(handled){if(t->order_count>=4096)return 0;if(t->appearance.count==app_before+1)t->order[t->order_count++]=0x10000u+(uint32_t)t->appearance.entries[app_before].request.kind;
   else if(t->binding.count==bind_before+1)t->order[t->order_count++]=0x20000u+(uint32_t)t->binding.requests[bind_before].request.kind;else return 0;continue;}
  if(c.pc==0x801812d4u){if(c.gpr[3]!=owner||object_calls++)return 0;c.gpr[3]=ACTOR;c.pc=c.lr;continue;}
  if(c.pc==0x8015e910u){channel_count=c.gpr[3]&0xffffu;channel_request=1;}
  if(c.pc==0x801c9784u){
   if(c.gpr[3]==45&&c.gpr[4]==4){if(t->appearance.node_allocations++)return 0;memset(ptr(ram,NODE_HEAP,512),0xa5,512);c.gpr[3]=NODE_HEAP;c.pc=c.lr;continue;}
   uint32_t token=0;nflstreet_player_model_span span;if(!binding_allocate(&t->binding,c.gpr[3],c.gpr[4],&token,&span))return 0;c.gpr[3]=token;c.pc=c.lr;continue;}
  if(c.pc==0x801c97c4u){if(!binding_release(&t->binding,c.gpr[3]))return 0;c.pc=c.lr;continue;}
  if(c.pc==0x8018b7b8u){
   if(c.gpr[3]==256&&!channel_request){if(c.gpr[4]||t->appearance.channel_allocations++)return 0;memset(ptr(ram,CHANNEL_HEAP,512),0xa5,512);c.gpr[3]=CHANNEL_HEAP;c.pc=c.lr;continue;}
   allocations*at=&t->model;if(at->count>=MAX_BLOCKS||c.gpr[3]>HEAP_STRIDE||c.gpr[4])return 0;unsigned i=at->count++;block*b=at->blocks+i;b->size=c.gpr[3];b->count=channel_request?channel_count:c.gpr[3]/64;b->kind=channel_request?NFLSTREET_ACTOR_CHANNEL:NFLSTREET_ACTOR_TRANSFORMS;
   memset(ptr(ram,HEAP+i*HEAP_STRIDE,HEAP_STRIDE),0xa5,HEAP_STRIDE);c.gpr[3]=HEAP+i*HEAP_STRIDE;c.pc=c.lr;channel_request=0;continue;}
  if(c.pc==0x8017bc54u){if(c.gpr[3]==ACTOR){if(!c.gpr[4]&&c.gpr[5]==NFLSTREET_ACTOR_MODEL_STORAGE){if(factory_clear++)return 0;}
    else {if(c.gpr[4]!=NFLSTREET_ACTOR_MODEL_STORAGE||c.gpr[5])return 0;++initial_clear;}}
   uint8_t*p=ptr(ram,c.gpr[3],c.gpr[5]);if(!p)return 0;memset(p,(uint8_t)c.gpr[4],c.gpr[5]);c.pc=c.lr;continue;}
  c.downcount=-1000000000;m->dispatch(&c,c.pc);if(c.exception){fprintf(stderr,"boundstartup exception%08x pc%08x\n",c.exception,c.pc);return 0;}
 }
 if(c.pc||object_calls!=1||factory_clear!=1||initial_clear!=1||c.gpr[3]!=ACTOR)return 0;bound_finish(t);return 1;
}
static int bound_case(const ModernGekkoModuleDesc*m,const uint8_t*baseline,const nflstreet_player_constructor_input*source,
 const nflstreet_player_appearance_catalog*catalog,unsigned mask,unsigned prefill,unsigned mutation,unsigned shared){
 bound_trace*o=calloc(1,sizeof *o),*n=calloc(1,sizeof *n);if(!o||!n){free(o);free(n);return 0;}o->binding.ram=malloc(RAM_SIZE);n->binding.ram=malloc(RAM_SIZE);if(!o->binding.ram||!n->binding.ram)return 0;
 memcpy(o->binding.ram,baseline,RAM_SIZE);uint8_t*ram=o->binding.ram;nflstreet_player_constructor_input input=*source;uint8_t descriptor[DESCRIPTOR_SIZE];memcpy(descriptor,source->bytes,sizeof descriptor);
 nflstreet_actor_model_resource*resources[]={&input.models.skeleton,&input.models.primary[0],&input.models.primary[1],&input.models.secondary[0],&input.models.secondary[1],&input.models.composite};
 for(unsigned i=0;i<6;++i){if(!(mask&(1u<<i)))memset(resources[i],0,sizeof *resources[i]);put(descriptor+4+4*i,resources[i]->token);}input.bytes=descriptor;
 uint8_t initial[NFLSTREET_ACTOR_MODEL_STORAGE];for(unsigned i=0;i<sizeof initial;++i)initial[i]=prefill?(uint8_t)(i*29+0x49):0;
 uint8_t*source_slot=ptr(ram,0x803a9a40u-0x7bbcu,4);uint8_t*source_list=source_slot?ptr(ram,be(source_slot),4):NULL;uint8_t*source_actor=source_list?ptr(ram,be(source_list),NFLSTREET_ACTOR_MODEL_STORAGE):NULL;if(!source_actor)return 0;
 o->binding.resources[0]=(uint32_t)(source_actor-ram)+BASE+0x430;o->binding.resources[1]=(uint32_t)(source_actor-ram)+BASE+0x474;memcpy(n->binding.resources,o->binding.resources,sizeof o->binding.resources);
 o->appearance.mutation=n->appearance.mutation=mutation;o->binding.mutation=n->binding.mutation=mutation;
 if(!shared)memset(ptr(ram,SHARED_TOKEN,68),0,68);
 /* Native namespace starts with the same explicit catalog/resource fixtures as
    the original. The factory descriptor replacement is private fixture setup. */
 memcpy(ptr(ram,ACTOR,sizeof initial),initial,sizeof initial);memcpy(ptr(ram,DESCRIPTOR,sizeof descriptor),descriptor,sizeof descriptor);
 put(ptr(ram,MODEL_TOKEN+0x18,4),NODE_LIST_TOKEN);put(ptr(ram,0x815d0200u+0x18,4),0x11330000u);for(unsigned i=0;i<2;++i)put(ptr(ram,SIDE_TOKEN+i*0x1000u+0x18,4),0x11220000u+i);
 put(ptr(ram,0x803a9a40u-0x58e8u,4),2);put(ptr(ram,0x803a9a40u-0x6238u,4),3);memset(ptr(ram,REGISTRY_TOKEN,REGISTRY_SIZE),0xa5,REGISTRY_SIZE);memset(ptr(ram,BINDINGS_TOKEN,REGISTRY_SIZE),0xa5,REGISTRY_SIZE);
 memcpy(n->binding.ram,ram,RAM_SIZE);
 if(!bound_original(m,o,initial,descriptor)){fprintf(stderr,"bound original executionfailed\n");return 0;}
 uint8_t*nram=n->binding.ram;uint32_t owner=factory_owner(baseline);put(ptr(nram,0x803a9a40u-0x4f14u,4),FACTORY_TABLE);uint8_t*record=ptr(nram,FACTORY_TABLE,20);memset(record,0,20);put(record,NFLSTREET_ACTOR_MODEL_STORAGE);put(record+8,0x800e1f7cu);put(record+0x10,owner);
 nflstreet_player_appearance_registry registry={ptr(nram,REGISTRY_TOKEN,REGISTRY_SIZE),REGISTRY_SIZE,REGISTRY_TOKEN,2,ptr(nram,BINDINGS_TOKEN,REGISTRY_SIZE),REGISTRY_SIZE,3};
 nflstreet_player_appearance_context appearance={ACTOR,NODE_LIST_TOKEN,catalog,&registry,appearance_service_bound,n,appearance_allocate_bound,n};
 nflstreet_player_model_binding_context binding={be(ptr(nram,0x803a9a40u-0x685cu,4)),TABLE_TOKEN,ptr(nram,SCRATCH_TOKEN,76),76,ptr(nram,SHARED_TOKEN,68),68,binding_resolve,&n->binding,binding_allocate,&n->binding,binding_release,&n->binding,binding_service_bound,n};
 object_allocation object={ptr(nram,ACTOR,NFLSTREET_ACTOR_MODEL_STORAGE),owner,0};nflstreet_factory_memory result={0};
 int ok=nflstreet_player_bound_create(owner,allocate_object,&object,&input,model_allocate_bound,n,&appearance,&binding,&result)&&object.calls==1&&result.token==ACTOR&&result.bytes==object.bytes;
 bound_finish(n);
 if(registry.actor_count!=be(ptr(ram,0x803a9a40u-0x58e8u,4))||registry.binding_count!=be(ptr(ram,0x803a9a40u-0x6238u,4)))ok=0;
 put(ptr(nram,0x803a9a40u-0x58e8u,4),registry.actor_count);put(ptr(nram,0x803a9a40u-0x6238u,4),registry.binding_count);
 /* CPU call-frame scratch is outside owned constructor/model state. */
 memset(ptr(ram,0x816f0000u,0x20000),0,0x20000);memset(ptr(nram,0x816f0000u,0x20000),0,0x20000);
 if(ok)ok=!memcmp(ram,nram,RAM_SIZE)&&!memcmp(&o->model,&n->model,sizeof o->model)&&!memcmp(&o->appearance,&n->appearance,sizeof o->appearance)&&
  o->order_count==n->order_count&&!memcmp(o->order,n->order,sizeof o->order)&&o->binding.count==n->binding.count&&o->binding.allocations==n->binding.allocations&&!memcmp(o->binding.requests,n->binding.requests,sizeof o->binding.requests)&&!memcmp(o->binding.blocks,n->binding.blocks,sizeof o->binding.blocks)&&
  registry.actor_count==be(ptr(ram,0x803a9a40u-0x58e8u,4))&&registry.binding_count==be(ptr(ram,0x803a9a40u-0x6238u,4));
 if(!ok){fprintf(stderr,"bound mismatch mask%u prefill%u mutation%u shared%u app%u/%u bind%u/%u alloc%u/%u\n",mask,prefill,mutation,shared,o->appearance.count,n->appearance.count,o->binding.count,n->binding.count,o->binding.allocations,n->binding.allocations);
  for(unsigned i=0;i<RAM_SIZE;++i)if(ram[i]!=nram[i]){fprintf(stderr,"RAMbyte%08x original%02x native%02x\n",BASE+i,ram[i],nram[i]);break;}}
 else printf("bound-startup mask=%u prefilled=%u mutation=%u shared=%u: actor1A90 +%u model blocks +two appearance allocations +%u binding allocations +two registries +%u/%u requests exact ordered=%u\n",mask,prefill,mutation,shared,o->model.count,o->binding.allocations,o->appearance.count,o->binding.count,o->order_count);
 free(o->binding.ram);free(n->binding.ram);free(o);free(n);return ok;
}
int main(int argc,char**argv){
    if(argc!=3){fprintf(stderr,"Usage: player-startup-original-check.exe originalDLL savedRAM\n");return 2;}
    HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);
    if(!get){FreeLibrary(h);return 3;}const ModernGekkoModuleDesc*m=get();if(!m||m->cpu_state_size!=sizeof(CPUState)){FreeLibrary(h);return 3;}
    uint8_t*baseline=load(argv[2]);if(!baseline){FreeLibrary(h);return 4;}uint8_t*slot=ptr(baseline,0x803a9a40u-0x7bbcu,4);uint8_t*list=slot?ptr(baseline,be(slot),4):NULL;uint8_t*a=list?ptr(baseline,be(list),NFLSTREET_ACTOR_MODEL_STORAGE):NULL;
    if(!a){free(baseline);FreeLibrary(h);return 4;}nflstreet_player_constructor_input input={0};uint8_t descriptor[DESCRIPTOR_SIZE]={0};input.bytes=descriptor;input.size=sizeof descriptor;
    nflstreet_actor_model_descriptor*d=&input.models;d->skeleton=resource(baseline,be(a+0x64));d->primary[0]=resource(baseline,be(a+0xbc));d->primary[1]=resource(baseline,be(a+0x104));
    d->secondary[0]=resource(baseline,be(a+0x194));d->secondary[1]=resource(baseline,be(a+0x210));d->composite=resource(baseline,be(a+0x70));d->initial_word_bits=be(ptr(baseline,0x8025283cu,4));
    put(descriptor+0x1c,be(a+0x135c));for(unsigned i=0;i<7;++i)put(descriptor+0x20+i*4,be(a+0x400+i*4));memcpy(descriptor+0x40,a+0x1360,5);
    input.tail.repeated_float_bits=be(ptr(baseline,0x80252654u,4));input.tail.default_float_bits=be(ptr(baseline,0x802526b0u,4));for(unsigned i=0;i<4;++i)input.tail.defaults[i]=be(ptr(baseline,0x80270c84u+4*i,4));
    uint32_t indexed[2],maps[2][25],side_textures[2]={0x11220000u,0x11220001u},finish[25][7];
    for(unsigned i=0;i<2;++i){indexed[i]=be(ptr(baseline,0x8028dd64u+4*i,4));for(unsigned j=0;j<25;++j)maps[i][j]=be(ptr(baseline,0x8028cf08u+100*i+4*j,4));}
    for(unsigned i=0;i<25;++i)for(unsigned j=0;j<7;++j)finish[i][j]=be(ptr(baseline,0x8028dd90u+28*i+4*j,4));
    nflstreet_player_appearance_catalog catalog={indexed,2,maps,2,be(ptr(baseline,0x80268d88u,4)),0x8028d480u,side_textures,2,finish,25};
    int ok=1;for(unsigned mask=0;mask<64&&ok;++mask)for(unsigned prefill=0;prefill<2&&ok;++prefill)for(unsigned mutation=0;mutation<2&&ok;++mutation)for(unsigned shared=0;shared<2&&ok;++shared)ok=bound_case(m,baseline,&input,&catalog,mask,prefill,mutation,shared);
    free(baseline);FreeLibrary(h);if(ok)puts("Joined original801956A4-800E1F7C-801639C8-801630B0 bound startup differential passed512 cases; required unresolved resource/render services explicit, no complete fresh player or playable match.");return ok?0:5;
}
