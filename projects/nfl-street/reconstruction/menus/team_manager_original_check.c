#include <windows.h>
#include "moderngekko/module_abi.h"
#include "font.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM_SIZE (24u*1024u*1024u)
#define MANAGER 0x817f0000u
#define SCREEN 0x81100000u
#define SOURCE_SIZE 253987u
#define FONT_RESOURCE 0x817f3100u
#define REGISTRY 0x017b0000u
static uint32_t be(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void put(uint8_t*p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
static unsigned event_callbacks,provider_calls,resource_misses,last_port,last_event,last_argc,script_dispatches,vm_entries,class_calls,router_hits,callback_hits;
static uint32_t callback_args[7];
static uint32_t callback_return_pc,callback_return_out;
static unsigned line_allocations;
static unsigned archive_flag_calls, archive_reuse_calls, resource_flush_calls, resource_release_calls, resource_acquire_calls;
static unsigned async_service_calls, async_lookup_calls;
static uint32_t async_service_args[16], async_service_status[16], async_lookup_args[16][3], async_lookup_status[16];
static uint32_t archive_flag_indices[16], resource_release_args[16][3], resource_acquire_args[16][3];
static uint32_t supplied_archive_flags;
static int first_wait_trace_seen;
static int script_callback_boundary=1;
static int constructor_boundary=1;
static int source_relocation_boundary;
static int execute_kind3_queue;
static int print_focus_vm;
static int trace_game_callbacks;
static int trace_source_frames;
static int trace_source_context;
static uint32_t common_mode_return_pc;
static uint32_t transition_clock_return_pc;
static unsigned deferred_queue_calls,deferred_queue_kinds[64];
typedef int (*team_manager_step_hook)(CPUState *);
static team_manager_step_hook team_manager_external_step_hook;
void nflstreet_team_manager_set_external_step_hook(team_manager_step_hook hook){team_manager_external_step_hook=hook;}
typedef struct{uint32_t pc,lr,sp,gpr[8],owner_stack;} trace_entry;
static trace_entry trace[20];static unsigned trace_next,trace_count;
typedef struct{uint32_t lr,state,last_source_pc;} vm_trace_frame;
static vm_trace_frame vm_trace_stack[16];static unsigned vm_trace_depth;
static void dump_trace(void){for(unsigned j=0;j<trace_count;++j){trace_entry*t=&trace[(trace_next-trace_count+j)%20];fprintf(stderr," trace pc%08x lr%08x sp%08x r3-r10=%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x owner+74=%08x\n",t->pc,t->lr,t->sp,t->gpr[0],t->gpr[1],t->gpr[2],t->gpr[3],t->gpr[4],t->gpr[5],t->gpr[6],t->gpr[7],t->owner_stack);}}
static int load_exact(const char *path, void *dst, size_t n) {
 FILE *f=fopen(path,"rb"); if(!f)return 0;
 int ok=fread(dst,1,n,f)==n; fclose(f); return ok;
}
static int save_exact(const char *path,const void *src,size_t n){FILE*f=fopen(path,"wb");if(!f)return 0;int ok=fwrite(src,1,n,f)==n;if(fclose(f))ok=0;return ok;}
static int load_fonts(const char *folder,uint8_t *ram){
 for(unsigned i=0;i<4;++i){char path[4096];int z=snprintf(path,sizeof path,"%s/uis_font/%05u.bin",folder,i);if(z<0||(size_t)z>=sizeof path)return 0;
  FILE*f=fopen(path,"rb");if(!f)return 0;if(fseek(f,0,SEEK_END)){fclose(f);return 0;}long sz=ftell(f);if(sz<1||sz>65536||fseek(f,0,SEEK_SET)){fclose(f);return 0;}
  unsigned at=0x01740000u+i*0x10000u;if(fread(ram+at,1,(size_t)sz,f)!=(size_t)sz){fclose(f);return 0;}fclose(f);nflstreet_font font;if(!nflstreet_font_open(ram+at,(size_t)sz,&font))return 0;
  unsigned rec=0x017f3000u+i*0x20u,asset=0x017f3100u+i*0x20u;
  uint32_t words[5]={0,0x80000000u+at,0x80000000u+at+font.glyphs,0x80000000u+at+font.kerning,0x80000000u+at+font.atlas};
  for(unsigned j=0;j<5;++j){ram[rec+j*4]=words[j]>>24;ram[rec+j*4+1]=words[j]>>16;ram[rec+j*4+2]=words[j]>>8;ram[rec+j*4+3]=words[j];}
  uint32_t p=0x80000000u+rec;ram[asset]=p>>24;ram[asset+1]=p>>16;ram[asset+2]=p>>8;ram[asset+3]=p;
 }
 return 1;
}
static int run(const ModernGekkoModuleDesc*m,CPUState*c,uint32_t pc,
               const uint32_t *args,unsigned argc,uint32_t *result) {
 c->pc=pc;c->lr=0;c->exception=0;c->downcount=c->cycle_budget=100000000;
 vm_trace_depth=0;
 for(unsigned i=0;i<argc&&i<8;++i)c->gpr[3+i]=args[i];
 for(unsigned i=8;i<argc;++i){uint32_t at=c->gpr[1]+8+(i-8)*4;if(at<0x80000000u||at-0x80000000u>c->ram_size-4)return 0;uint8_t*p=c->ram+at-0x80000000u;uint32_t v=args[i];p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
 for(unsigned i=0;c->pc&&i<10000000;++i){
  if(trace_source_context&&common_mode_return_pc&&c->pc==common_mode_return_pc){fprintf(stderr,"8010B60C common-mode return=%08x\n",c->gpr[3]);common_mode_return_pc=0;}
  if(trace_source_context&&c->pc==0x8010b60c){common_mode_return_pc=c->lr;uint32_t mode=be(c->ram+0x803a2a50u-0x80000000u);fprintf(stderr,"8010B60C entry lr=%08x stored-mode=%08x\n",c->lr,mode);}
  if(trace_source_context&&c->pc==0x80137b84){uint32_t obj=be(c->ram+0x803a2f00u-0x80000000u);fprintf(stderr,"80137B84 invoked ticks=%u arg0=%08x transition-object=%08x audio-enabled=%u",c->gpr[3],c->gpr[3],obj,c->ram[0x803a2f04u-0x80000000u]);if(obj>=0x80000000u&&obj-0x80000000u<=c->ram_size-0x14)fprintf(stderr," handle=%08x deadline=%08x state=%02x",be(c->ram+obj-0x80000000u),be(c->ram+obj-0x80000000u+0x0c),c->ram[obj-0x80000000u+0x13]);fputc('\n',stderr);}
  if(trace_source_context&&transition_clock_return_pc&&c->pc==transition_clock_return_pc){fprintf(stderr,"801AF464 return clock=%08x\n",c->gpr[3]);transition_clock_return_pc=0;}
  if(trace_source_context&&c->pc==0x801af464){uint32_t object=be(c->ram+0x803a3594u-0x80000000u);transition_clock_return_pc=c->lr;fprintf(stderr,"801AF464 entry lr=%08x timer-object=%08x",c->lr,object);if(object>=0x80000000u&&object-0x80000000u<=c->ram_size-0x24)fprintf(stderr," stored-clock=%08x",be(c->ram+object-0x80000000u+0x20));fputc('\n',stderr);}
  if(trace_source_context&&c->pc==0x801d49f0)fprintf(stderr,"801D49F0 broadcast args=%08x,%08x,%08x,%08x\n",c->gpr[3],c->gpr[4],c->gpr[5],c->gpr[6]);
  if(trace_game_callbacks&&callback_return_pc&&c->pc==callback_return_pc){uint32_t value=0;if(callback_return_out>=0x80000000u&&callback_return_out-0x80000000u<=c->ram_size-4)value=be(c->ram+callback_return_out-0x80000000u);fprintf(stderr,"E9D4 return gpr3=%08x byref[%08x]=%08x\n",c->gpr[3],callback_return_out,value);callback_return_pc=0;}
  if(trace_source_frames){while(vm_trace_depth&&c->pc==vm_trace_stack[vm_trace_depth-1].lr)--vm_trace_depth;
   if(c->pc==0x801d16a0&&vm_trace_depth<16){vm_trace_frame*f=&vm_trace_stack[vm_trace_depth++];f->lr=c->lr;f->state=c->gpr[5];f->last_source_pc=0;}
   if(vm_trace_depth){vm_trace_frame*f=&vm_trace_stack[vm_trace_depth-1];uint32_t p=f->state;if(p>=0x80000000u&&p-0x80000000u<=c->ram_size-20){uint32_t source_pc=be(c->ram+p-0x80000000u+16);if(source_pc!=f->last_source_pc){f->last_source_pc=source_pc;if(source_pc>=SCREEN&&source_pc-SCREEN<SOURCE_SIZE){uint8_t*op=c->ram+source_pc-0x80000000u-1;uint32_t sp=be(c->ram+p-0x80000000u+12);fprintf(stderr,"SCRIPT state=%08x source_pc=%08x op=%02x next=",p,source_pc,*op);for(unsigned k=0;k<8;++k)fprintf(stderr,"%s%02x",k?" ":"",op[1+k]);fprintf(stderr," sp=%08x stack=",sp);if(sp>=0x80000000u&&sp-0x80000000u<=c->ram_size-24){for(unsigned k=0;k<6;++k)fprintf(stderr,"%s%08x",k?",":"",be(c->ram+sp-0x80000000u+k*4));}else fprintf(stderr,"<unmapped>");fputc('\n',stderr);}}}}
  }
  trace_entry*t=&trace[trace_next++%20];t->pc=c->pc;t->lr=c->lr;t->sp=c->gpr[1];for(unsigned j=0;j<8;++j)t->gpr[j]=c->gpr[3+j];t->owner_stack=be(c->ram+MANAGER-0x80000000u+0x68+0x0c);if(trace_count<20)++trace_count;
  if(team_manager_external_step_hook){int hook_result=team_manager_external_step_hook(c);if(hook_result<0)return 0;if(hook_result>0)continue;}
  if(c->pc==0x801f471c&&!first_wait_trace_seen){first_wait_trace_seen=1;fprintf(stderr,"first 801F471C wait entry; preceding instruction trace:\n");dump_trace();}
  if(c->pc==0x801d399c)++script_dispatches;
  if(c->pc==0x801d399c&&trace_source_frames){fprintf(stderr,"D399C args r3-r10=%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x lr=%08x\n",c->gpr[3],c->gpr[4],c->gpr[5],c->gpr[6],c->gpr[7],c->gpr[8],c->gpr[9],c->gpr[10],c->lr);}
  if(c->pc==0x801dd234)++archive_reuse_calls;
  if(c->pc==0x801d16a0){++vm_entries;if(trace_source_frames){fprintf(stderr,"D16A0 args r3-r10=%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x lr=%08x\n",c->gpr[3],c->gpr[4],c->gpr[5],c->gpr[6],c->gpr[7],c->gpr[8],c->gpr[9],c->gpr[10],c->lr);if(c->gpr[4]>=0x80000000u&&c->gpr[4]-0x80000000u<c->ram_size-32){uint8_t*p=c->ram+c->gpr[4]-0x80000000u;fprintf(stderr,"D16A0 r4 words: %08x %08x %08x %08x %08x %08x %08x %08x\n",be(p),be(p+4),be(p+8),be(p+12),be(p+16),be(p+20),be(p+24),be(p+28));}if(c->gpr[6]>=0x80000000u&&c->gpr[6]-0x80000000u<c->ram_size-32){uint8_t*p=c->ram+c->gpr[6]-0x80000000u;fprintf(stderr,"D16A0 row/current-link fields: %08x,%04x,%04x,%08x,%08x\n",be(p),(unsigned)((p[4]<<8)|p[5]),(unsigned)((p[6]<<8)|p[7]),be(p+8),be(p+12));}}if(print_focus_vm){print_focus_vm=0;fprintf(stderr,"focus VM entry r3-r10=%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x\n",c->gpr[3],c->gpr[4],c->gpr[5],c->gpr[6],c->gpr[7],c->gpr[8],c->gpr[9],c->gpr[10]);if(c->gpr[4]>=0x80000000u&&c->gpr[4]-0x80000000u<c->ram_size-64){uint8_t*p=c->ram+c->gpr[4]-0x80000000u;fprintf(stderr,"focus VM stack at r4: %08x %08x %08x %08x %08x %08x %08x %08x\n",be(p),be(p+4),be(p+8),be(p+12),be(p+16),be(p+20),be(p+24),be(p+28));}}}
  if(c->pc==0x801d60dc||c->pc==0x801d9244)++class_calls;
  if(c->pc==0x8012a414||c->pc==0x8010c3bc)++router_hits;
  if(c->pc==0x8011e9d4){++callback_hits;for(unsigned j=0;j<7;++j)callback_args[j]=c->gpr[3+j];if(trace_game_callbacks){uint32_t p=c->gpr[4];fprintf(stderr,"E9D4 property=%08x words=",c->gpr[3]);if(p>=0x80000000u&&p-0x80000000u<=c->ram_size-24){for(unsigned j=0;j<6;++j)fprintf(stderr,"%s%08x",j?",":"",be(c->ram+p-0x80000000u+j*4));}else fprintf(stderr,"<unmapped:%08x>",p);callback_return_pc=c->lr;callback_return_out=c->gpr[6];uint32_t outvalue=0;if(callback_return_out>=0x80000000u&&callback_return_out-0x80000000u<=c->ram_size-4)outvalue=be(c->ram+callback_return_out-0x80000000u);fprintf(stderr," param3=%08x out=%08x current-out=%08x lr=%08x\n",c->gpr[5],c->gpr[6],outvalue,c->lr);}}
  if(c->pc==0x801ab4b8){
   if(async_lookup_calls<16){for(unsigned j=0;j<3;++j)async_lookup_args[async_lookup_calls][j]=c->gpr[3+j];async_lookup_status[async_lookup_calls]=be(c->ram+0x8034859c-0x80000000u);}
   ++async_lookup_calls;c->gpr[3]=0;c->pc=c->lr;
  } else if(c->pc==0x801ab608){
   if(async_service_calls<16){async_service_args[async_service_calls]=c->gpr[3];async_service_status[async_service_calls]=be(c->ram+0x8034859c-0x80000000u);}
   ++async_service_calls;c->gpr[3]=0;c->pc=c->lr;
  } else if(c->pc==0x801d5aec){
   if(archive_flag_calls<16)archive_flag_indices[archive_flag_calls]=c->gpr[4];
   ++archive_flag_calls;c->gpr[3]=supplied_archive_flags;c->pc=c->lr;
  } else if(c->pc==0x801878b0){
   ++resource_flush_calls;c->pc=c->lr;
  } else if(c->pc==0x801d5918){
   if(resource_release_calls<16)for(unsigned j=0;j<3;++j)resource_release_args[resource_release_calls][j]=c->gpr[3+j];
   ++resource_release_calls;c->pc=c->lr;
  } else if(c->pc==0x8012ac68){
   if(c->gpr[3]!=0||c->gpr[4]!=7){fprintf(stderr,"provider key %x/%x\n",c->gpr[3],c->gpr[4]);return 0;}
   ++provider_calls;c->gpr[3]=SCREEN;c->pc=c->lr;
  } else if(c->pc==0x801ce994&&constructor_boundary){c->pc=c->lr;
  } else if(c->pc==0x801cf34c){
   ++event_callbacks;last_port=c->gpr[7];last_event=c->gpr[8];last_argc=c->gpr[9];
   if(script_callback_boundary)c->pc=c->lr;
   else {m->dispatch(c,c->pc);if(c->exception){fprintf(stderr,"pc %08x exception %08x\n",c->pc,c->exception);return 0;}}
  } else if(c->pc==0x801d5884&&c->gpr[4]==2&&c->gpr[5]<4){c->gpr[3]=FONT_RESOURCE+c->gpr[5]*0x20;c->pc=c->lr;
  } else if(c->pc==0x801d5884){++resource_misses;m->dispatch(c,c->pc);if(c->exception){fprintf(stderr,"D5884 pc %08x exception %08x\n",c->pc,c->exception);return 0;}
  } else if(c->pc==0x801cf8c8&&source_relocation_boundary){c->pc=c->lr;
  } else if(c->pc==0x801d5c24){if(c->gpr[3]!=1)return 0;c->gpr[3]=MANAGER;c->pc=c->lr;
  } else if(c->pc==0x801d54a4){
   if(resource_acquire_calls<16)for(unsigned j=0;j<3;++j)resource_acquire_args[resource_acquire_calls][j]=c->gpr[3+j];
   ++resource_acquire_calls;c->pc=c->lr;
  } else if(c->pc==0x8018b7b8){if(line_allocations>=128||c->gpr[3]>1020||c->gpr[4])return 0;c->gpr[3]=0x81610000u+line_allocations++*1024u;c->pc=c->lr;
  } else if(c->pc==0x801d4b3c){
   if(execute_kind3_queue)m->dispatch(c,c->pc);
   else {if(deferred_queue_calls<64)deferred_queue_kinds[deferred_queue_calls]=c->gpr[6];++deferred_queue_calls;c->pc=c->lr;}
  } else {m->dispatch(c,c->pc);if(c->exception){fprintf(stderr,"pc %08x exception %08x\n",c->pc,c->exception);dump_trace();return 0;}}
 }
 if(c->pc){fprintf(stderr,"stopped %08x lr%08x r3%08x r4%08x r5%08x archiveFlags=%u flushes=%u releases=%u acquires=%u\n",c->pc,c->lr,c->gpr[3],c->gpr[4],c->gpr[5],archive_flag_calls,resource_flush_calls,resource_release_calls,resource_acquire_calls);dump_trace();return 0;}
 if(result)*result=c->gpr[3];return 1;
}
int main(int argc,char**argv){
 if(argc<5)return 2;
 const char*prepared_dir=NULL,*prepared_ram_path=NULL,*focus_snapshot_dir=NULL,*event_snapshot_dir=NULL;int flags_seen=0,activation_probe=0,full_activation_probe=0,dispatch_current_row=0,stop_at_select22=0,event7_only=0,mode_override=-1;
 for(int i=5;i<argc;++i){
  if(!strcmp(argv[i],"--prepared-dir")&&i+1<argc)prepared_dir=argv[++i];
  else if(!strcmp(argv[i],"--prepared-ram")&&i+1<argc)prepared_ram_path=argv[++i];
  else if(!strcmp(argv[i],"--activate-probe"))activation_probe=1;
  else if(!strcmp(argv[i],"--full-activation-probe")){activation_probe=1;full_activation_probe=1;}
  else if(!strcmp(argv[i],"--focus-snapshot-dir")&&i+1<argc)focus_snapshot_dir=argv[++i];
  else if(!strcmp(argv[i],"--trace-game-callbacks"))trace_game_callbacks=1;
  else if(!strcmp(argv[i],"--trace-source-frames"))trace_source_frames=1;
  else if(!strcmp(argv[i],"--trace-source-context"))trace_source_context=1;
  else if(!strcmp(argv[i],"--dispatch-current-row"))dispatch_current_row=1;
  else if(!strcmp(argv[i],"--stop-at-select22"))stop_at_select22=1;
  else if(!strcmp(argv[i],"--event7-only"))event7_only=1;
  else if(!strcmp(argv[i],"--event-snapshot-dir")&&i+1<argc)event_snapshot_dir=argv[++i];
  else if(!strcmp(argv[i],"--override-original-mode")&&i+1<argc){char*end=NULL;unsigned long v=strtoul(argv[++i],&end,0);if(!end||*end||v>8)return 2;mode_override=(int)v;}
  else if(!flags_seen){char*end=NULL;unsigned long v=strtoul(argv[i],&end,0);if(!end||*end||v>0x7fffffffu)return 2;supplied_archive_flags=(uint32_t)v;flags_seen=1;}
  else return 2;
 }
 if((prepared_dir==NULL)!=(prepared_ram_path==NULL))return 2;
 const int prepared_mode=prepared_dir!=NULL;
 HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;
 ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;
 const ModernGekkoModuleDesc*m=get();if(m->cpu_state_size!=sizeof(CPUState))return 3;
 uint8_t*ram=calloc(1,RAM_SIZE);if(!ram)return 4;
 if(!load_exact(argv[2],ram,RAM_SIZE)||!load_exact(argv[3],ram+SCREEN-0x80000000u,SOURCE_SIZE)||!load_fonts(argv[4],ram))return 4;
 CPUState c={0};c.ram=ram;c.ram_size=RAM_SIZE;c.msr=0x2000;c.gpr[1]=0x81700000;c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;
 uint32_t a[9],size=0;
 a[0]=12;a[1]=9;a[2]=0xa0;a[3]=7;a[4]=0x280;a[5]=0xa0;
 if(!run(m,&c,0x801cfb4c,a,6,&size)||size>0x10000)return 5;
 a[0]=MANAGER;a[1]=12;a[2]=9;a[3]=0xa0;a[4]=7;a[5]=0x280;a[6]=0xa0;a[7]=0x10;
 if(!run(m,&c,0x801cfb80,a,8,NULL))return 6;
 a[0]=MANAGER;if(!run(m,&c,0x8012aa2c,a,1,NULL))return 7;
 /* Verified entry7 constructor inputs: loaded identity7, link5, owned source at SCREEN. */
 uint32_t rec=0x017f0100u;ram[0x017f0048]=0;ram[0x017f0049]=0;ram[0x017f004a]=0;ram[0x017f004b]=9;
 a[0]=0x817b0000u;ram[0x017f004c]=0x81;ram[0x017f004d]=0x7b;ram[0x017f004e]=0;ram[0x017f004f]=0;
 ram[rec+4]=0;ram[rec+5]=0;ram[rec+6]=0;ram[rec+7]=7;ram[rec+8]=0;ram[rec+9]=0;ram[rec+10]=0;ram[rec+11]=5;
 ram[rec+16]=0x81;ram[rec+17]=0x10;ram[rec+18]=0;ram[rec+19]=0;
 memset(ram+REGISTRY,0,36);ram[REGISTRY]=0x80;ram[REGISTRY+1]=0x1d;ram[REGISTRY+2]=0x60;ram[REGISTRY+3]=0xdc;ram[REGISTRY+0x20]=0x80;ram[REGISTRY+0x21]=0x1d;ram[REGISTRY+0x22]=0x92;ram[REGISTRY+0x23]=0x44;
 /* D0264 performs the normal wrapper path; its D007C/CFD90/CF8C8 and
    D3828 table work execute in the original module. The file provider is
    the explicit boundary at the installed 8012AC68 resolver. */
 a[0]=MANAGER;a[1]=0;a[2]=7;a[3]=0;a[4]=0;
 constructor_boundary=1;script_callback_boundary=1;
 if(!run(m,&c,0x801d0264,a,5,NULL)){fprintf(stderr,"D0264 failed pc=%08x lr=%08x ex=%08x sp=%08x r3=%08x r4=%08x r5=%08x\n",c.pc,c.lr,c.exception,c.gpr[1],c.gpr[3],c.gpr[4],c.gpr[5]);return 8;}
 uint32_t count=be(ram+MANAGER-0x80000000+0x38);
 uint32_t table=be(ram+MANAGER-0x80000000+0x3c);
 if(count!=1||table<0x80000000||table-0x80000000>RAM_SIZE-0x14){fprintf(stderr,"registration count/table %u/%08x\n",count,table);return 9;}
 uint8_t*r=ram+table-0x80000000;
 if(((unsigned)r[4]<<8|r[5])!=0||((unsigned)r[6]<<8|r[7])!=7){fprintf(stderr,"registered pair %u/%u\n",((unsigned)r[4]<<8|r[5]),((unsigned)r[6]<<8|r[7]));return 10;}
 uint8_t registered_row[20];memcpy(registered_row,r,sizeof registered_row);
 unsigned registered_provider_calls=provider_calls;
 /* Separate controlled stage: either original construction, or a materialized
    already-constructed native source with original relocation/construction skipped. */
 if(prepared_mode){if(!load_exact(prepared_ram_path,ram,RAM_SIZE))return 13;}
 else if(!load_exact(argv[2],ram,RAM_SIZE)||!load_exact(argv[3],ram+SCREEN-0x80000000u,SOURCE_SIZE)||!load_fonts(argv[4],ram))return 13;
 memset(&c,0,sizeof c);c.ram=ram;c.ram_size=RAM_SIZE;c.msr=0x2000;c.gpr[1]=0x81700000;c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;
 a[0]=12;a[1]=9;a[2]=0xa0;a[3]=7;a[4]=0x280;a[5]=0xa0;if(!run(m,&c,0x801cfb4c,a,6,&size))return 14;
 a[0]=MANAGER;a[1]=12;a[2]=9;a[3]=0xa0;a[4]=7;a[5]=0x280;a[6]=0xa0;a[7]=0x10;if(!run(m,&c,0x801cfb80,a,8,NULL))return 14;
 a[0]=MANAGER;if(!run(m,&c,0x8012aa2c,a,1,NULL))return 14;
 if(prepared_mode){for(unsigned i=0;i<10;++i)put(ram+0x00282e34u+i*4u,i==0?0:UINT32_MAX);}
 const uint32_t source_record=0x817f0100u;uint32_t row=table;
 put(ram+source_record-0x80000000u+0x104,7);put(ram+source_record-0x80000000u+0x108,5);put(ram+source_record-0x80000000u+0x110,SCREEN);
 if(prepared_mode){
  source_relocation_boundary=1;constructor_boundary=1;
  a[0]=MANAGER;a[1]=0;a[2]=7;a[3]=0;a[4]=0;
  if(!run(m,&c,0x801d0264,a,5,NULL)){fprintf(stderr,"Prepared-source D0264 failed pc=%08x lr=%08x ex=%08x\n",c.pc,c.lr,c.exception);return 14;}
  uint32_t prepared_count=be(ram+MANAGER-0x80000000u+0x38);row=be(ram+MANAGER-0x80000000u+0x3c);
  if(prepared_count!=1||row<0x80000000u||row-0x80000000u>RAM_SIZE-0x14){fprintf(stderr,"prepared registration count/table %u/%08x\n",prepared_count,row);return 14;}
  uint8_t*pr=ram+row-0x80000000u;if(((unsigned)pr[4]<<8|pr[5])!=0||((unsigned)pr[6]<<8|pr[7])!=7){fprintf(stderr,"prepared registered pair %u/%u\n",((unsigned)pr[4]<<8|pr[5]),((unsigned)pr[6]<<8|pr[7]));return 14;}
  if(activation_probe){
   if(full_activation_probe){static const uint32_t callbacks[9]={0x801d60dc,0x8012cb94,0x801da46c,0x8012c570,0x801dc660,0x8012c5d8,0x8012ca3c,0x8012b394,0x801d9244};for(unsigned ci=0;ci<9;++ci)put(ram+0x017b0380u+ci*4u,callbacks[ci]);script_callback_boundary=0;}
   uint8_t*activation_source=malloc(SOURCE_SIZE);if(!activation_source)return 14;memcpy(activation_source,ram+SCREEN-0x80000000u,SOURCE_SIZE);unsigned activate_callbacks=event_callbacks,activate_vms=vm_entries,activate_classes=class_calls;
   execute_kind3_queue=1;uint32_t activate[3]={MANAGER,0,7};
   if(!run(m,&c,0x801d087c,activate,3,NULL)){free(activation_source);fprintf(stderr,"D087C kind-3 activation failed pc=%08x lr=%08x ex=%08x\n",c.pc,c.lr,c.exception);return 14;}
   execute_kind3_queue=0;
   uint32_t inner=be(pr+0x10),node_table=be(ram+inner-0x80000000u+4),root0=be(ram+node_table-0x80000000u),root22=be(ram+node_table-0x80000000u+22u*20u);
   unsigned activation_source_diff=0;for(unsigned ai=0;ai<SOURCE_SIZE;++ai)activation_source_diff+=activation_source[ai]!=(ram+SCREEN-0x80000000u)[ai];free(activation_source);
   fprintf(stderr,"D087C kind3 activation (0,7): manager flags=%08x current=%08x queue=%08x/%08x row+10=%08x innercount=%08x innerrows=%08x root0=%08x selected0=%08x root22=%08x selected22=%08x FFFC-callbacks=%u VM=%u classes=%u source-diff=%u/%u\n",be(ram+MANAGER-0x80000000u+4),be(ram+MANAGER-0x80000000u+0x30),be(ram+MANAGER-0x80000000u+0xb0),be(ram+MANAGER-0x80000000u+0xb4),inner,be(ram+inner-0x80000000u),node_table,root0,be(ram+(root0-0x80000000u)+4),root22,be(ram+(root22-0x80000000u)+4),event_callbacks-activate_callbacks,vm_entries-activate_vms,class_calls-activate_classes,activation_source_diff,SOURCE_SIZE);
   if(be(ram+MANAGER-0x80000000u+0x30)!=0||inner!=SCREEN||be(ram+inner-0x80000000u)!=317||be(ram+(root0-0x80000000u)+4)!=1||be(ram+MANAGER-0x80000000u+0xb0)!=be(ram+MANAGER-0x80000000u+0xb4)){fprintf(stderr,"D087C activation state invariant failed\n");return 14;}
   if(focus_snapshot_dir){char path[4096];int z=snprintf(path,sizeof path,"%s/original-context%u-after-activation.bin",focus_snapshot_dir,prepared_ram_path&&strstr(prepared_ram_path,"context15")?15:prepared_ram_path&&strstr(prepared_ram_path,"context16")?16:14);if(z<0||(size_t)z>=sizeof path||!save_exact(path,ram+SCREEN-0x80000000u,SOURCE_SIZE)){fprintf(stderr,"cannot save activation source snapshot\n");return 14;}}
   uint32_t root21=be(ram+(node_table-0x80000000u)+21u*20u),root28=be(ram+(node_table-0x80000000u)+28u*20u);
   uint32_t focus_args[7]={MANAGER,1,0,root21,0,7,0};unsigned source_callbacks_before=callback_hits,source_vm_before=vm_entries;print_focus_vm=1;
   if(!run(m,&c,0x801d0e18,focus_args,7,NULL)){fprintf(stderr,"D0E18 prepared node21 ancestor select failed pc=%08x lr=%08x ex=%08x\n",c.pc,c.lr,c.exception);return 14;}
   fprintf(stderr,"D0E18 select ancestor node21: selected21=%08x callback-delta=%u vm-delta=%u queue=%u/%u\n",be(ram+(root21-0x80000000u)+4),callback_hits-source_callbacks_before,vm_entries-source_vm_before,be(ram+MANAGER-0x80000000u+0xb0),be(ram+MANAGER-0x80000000u+0xb4));
   if(be(ram+(root21-0x80000000u)+4)!=1){fprintf(stderr,"D0E18 ancestor-select invariant failed\n");return 14;}
   if(focus_snapshot_dir){char path[4096];int z=snprintf(path,sizeof path,"%s/original-context%u-after-select21.bin",focus_snapshot_dir,prepared_ram_path&&strstr(prepared_ram_path,"context15")?15:prepared_ram_path&&strstr(prepared_ram_path,"context16")?16:14);if(z<0||(size_t)z>=sizeof path||!save_exact(path,ram+SCREEN-0x80000000u,SOURCE_SIZE)){fprintf(stderr,"cannot save node21 focus snapshot\n");return 14;}}
   focus_args[3]=root22;source_callbacks_before=callback_hits;source_vm_before=vm_entries;
   if(!run(m,&c,0x801d0e18,focus_args,7,NULL)){fprintf(stderr,"D0E18 prepared node22 select failed pc=%08x lr=%08x ex=%08x\n",c.pc,c.lr,c.exception);return 14;}
   fprintf(stderr,"D0E18 select node22 under node21: selected21=%08x selected22=%08x callback-delta=%u vm-delta=%u queue=%u/%u\n",be(ram+(root21-0x80000000u)+4),be(ram+(root22-0x80000000u)+4),callback_hits-source_callbacks_before,vm_entries-source_vm_before,be(ram+MANAGER-0x80000000u+0xb0),be(ram+MANAGER-0x80000000u+0xb4));
   if(be(ram+(root22-0x80000000u)+4)!=1){fprintf(stderr,"D0E18 child-select invariant failed\n");return 14;}
   if(focus_snapshot_dir){char path[4096];int z=snprintf(path,sizeof path,"%s/original-context%u-after-select22.bin",focus_snapshot_dir,prepared_ram_path&&strstr(prepared_ram_path,"context15")?15:prepared_ram_path&&strstr(prepared_ram_path,"context16")?16:14);if(z<0||(size_t)z>=sizeof path||!save_exact(path,ram+SCREEN-0x80000000u,SOURCE_SIZE)){fprintf(stderr,"cannot save node22 focus snapshot\n");return 14;}}
   if(!stop_at_select22){source_callbacks_before=callback_hits;source_vm_before=vm_entries;unsigned class_before=class_calls;
   focus_args[3]=root28;print_focus_vm=1;
   if(!run(m,&c,0x801d0e18,focus_args,7,NULL)){fprintf(stderr,"D0E18 prepared node28 select failed pc=%08x lr=%08x ex=%08x\n",c.pc,c.lr,c.exception);return 14;}
   fprintf(stderr,"D0E18 select node28 after node21->22: selected21=%08x selected22=%08x selected28=%08x callback-delta=%u vm-delta=%u class-delta=%u queue=%u/%u deferred=%u types=",be(ram+(root21-0x80000000u)+4),be(ram+(root22-0x80000000u)+4),be(ram+(root28-0x80000000u)+4),callback_hits-source_callbacks_before,vm_entries-source_vm_before,class_calls-class_before,be(ram+MANAGER-0x80000000u+0xb0),be(ram+MANAGER-0x80000000u+0xb4),deferred_queue_calls);
   for(unsigned qi=0;qi<deferred_queue_calls&&qi<64;++qi)fprintf(stderr,"%s%u",qi?",":"",deferred_queue_kinds[qi]);fprintf(stderr,"\n");
   if(be(ram+(root22-0x80000000u)+4)!=0||be(ram+(root28-0x80000000u)+4)!=1||vm_entries-source_vm_before!=2||class_calls-class_before!=5||deferred_queue_calls!=0){fprintf(stderr,"D0E18 previous-deselect/target-select invariant failed\n");return 14;}
   if(focus_snapshot_dir){char path[4096];int z=snprintf(path,sizeof path,"%s/original-context%u-after-select28.bin",focus_snapshot_dir,prepared_ram_path&&strstr(prepared_ram_path,"context15")?15:prepared_ram_path&&strstr(prepared_ram_path,"context16")?16:14);if(z<0||(size_t)z>=sizeof path||!save_exact(path,ram+SCREEN-0x80000000u,SOURCE_SIZE)){fprintf(stderr,"cannot save node28 focus snapshot\n");return 14;}}
   }
  }
 }else{
  a[0]=SCREEN;if(!run(m,&c,0x801cf8c8,a,1,NULL))return 14;
  constructor_boundary=0;line_allocations=0;a[0]=MANAGER;a[1]=source_record;a[2]=0;a[3]=UINT32_MAX;
  if(!run(m,&c,0x801ce994,a,4,NULL)){fprintf(stderr,"Controlled CE994 failed pc=%08x lr=%08x ex=%08x\n",c.pc,c.lr,c.exception);return 14;}
  memcpy(ram+row-0x80000000u,registered_row,sizeof registered_row);put(ram+row-0x80000000u+16,SCREEN);
  ram[MANAGER-0x80000000u+0x38]=0;ram[MANAGER-0x80000000u+0x39]=0;ram[MANAGER-0x80000000u+0x3a]=0;ram[MANAGER-0x80000000u+0x3b]=1;
  put(ram+MANAGER-0x80000000u+0x3c,row);
 }
 uint8_t*event_baseline=prepared_mode?malloc(RAM_SIZE):NULL;uint8_t*prepared_source_start=prepared_mode?malloc(SOURCE_SIZE):NULL;uint8_t*native_source_start=prepared_mode?malloc(SOURCE_SIZE):NULL;CPUState event_cpu={0};
 if(prepared_mode){char native_start_path[4096];int z=snprintf(native_start_path,sizeof native_start_path,"%s/native-pre-input-source.bin",prepared_dir);if(!event_baseline||!prepared_source_start||!native_source_start||z<0||(size_t)z>=sizeof native_start_path||!load_exact(native_start_path,native_source_start,SOURCE_SIZE)){fprintf(stderr,"cannot load native prepared source baseline\n");return 14;}memcpy(event_baseline,ram,RAM_SIZE);memcpy(prepared_source_start,ram+SCREEN-0x80000000u,SOURCE_SIZE);event_cpu=c;unsigned baseline_differences=0;for(unsigned k=0;k<SOURCE_SIZE;++k)if(prepared_source_start[k]!=native_source_start[k])++baseline_differences;printf("Prepared-source baseline after original D0264 registration: %u/%u bytes differ from native pre-input source.\n",baseline_differences,SOURCE_SIZE);}
 script_callback_boundary=0;
 event_callbacks=0;ram[0x17f2000]=0x3f;ram[0x17f2001]=0x80;ram[0x17f2002]=0;ram[0x17f2003]=0;script_callback_boundary=0;
 for(unsigned event=event7_only?7:2;event<=(event7_only?7:5);++event){
  if(prepared_mode){memcpy(ram,event_baseline,RAM_SIZE);c=event_cpu;c.ram=ram;c.ram_size=RAM_SIZE;trace_next=trace_count=0;memset(trace,0,sizeof trace);}
  if(mode_override>=0)put(ram+0x003a2a50u,(uint32_t)mode_override);
  if(event_snapshot_dir&&event7_only&&event==7){char p[4096];int z=snprintf(p,sizeof p,"%s/original-mode%u-context14-pre-event7.mem1",event_snapshot_dir,mode_override<0?0u:(unsigned)mode_override);if(z<0||(size_t)z>=sizeof p||!save_exact(p,ram,RAM_SIZE))return 16;z=snprintf(p,sizeof p,"%s/original-mode%u-context14-pre-event7-source.bin",event_snapshot_dir,mode_override<0?0u:(unsigned)mode_override);if(z<0||(size_t)z>=sizeof p||!save_exact(p,ram+SCREEN-0x80000000u,SOURCE_SIZE))return 16;}
  put(ram+0x017f2000u,0x3f800000u);
  event_callbacks=script_dispatches=vm_entries=class_calls=router_hits=callback_hits=0;archive_flag_calls=archive_reuse_calls=resource_flush_calls=resource_release_calls=resource_acquire_calls=async_service_calls=async_lookup_calls=0;first_wait_trace_seen=0;memset(async_service_args,0,sizeof async_service_args);memset(async_service_status,0,sizeof async_service_status);memset(async_lookup_args,0,sizeof async_lookup_args);memset(async_lookup_status,0,sizeof async_lookup_status);memset(archive_flag_indices,0,sizeof archive_flag_indices);memset(resource_release_args,0,sizeof resource_release_args);memset(resource_acquire_args,0,sizeof resource_acquire_args);memset(callback_args,0,sizeof callback_args);a[0]=MANAGER;a[1]=0;a[2]=event;a[3]=1;a[4]=0x817f2000;a[5]=dispatch_current_row?0:1;
  if(!run(m,&c,0x801d094c,a,6,NULL)||event_callbacks==0||last_port!=0||last_event!=event||last_argc!=1){fprintf(stderr,"entry dispatch missing event%u calls%u args%u/%u/%u\n",event,event_callbacks,last_port,last_event,last_argc);return 12;}
  if(event_snapshot_dir&&event7_only&&event==7){char p[4096];int z=snprintf(p,sizeof p,"%s/original-mode%u-context14-post-event7.mem1",event_snapshot_dir,mode_override<0?0u:(unsigned)mode_override);if(z<0||(size_t)z>=sizeof p||!save_exact(p,ram,RAM_SIZE))return 16;z=snprintf(p,sizeof p,"%s/original-mode%u-context14-post-event7-source.bin",event_snapshot_dir,mode_override<0?0u:(unsigned)mode_override);if(z<0||(size_t)z>=sizeof p||!save_exact(p,ram+SCREEN-0x80000000u,SOURCE_SIZE))return 16;fprintf(stderr,"event7 state metadata: port=0 event=7 argc=1 float=1.0 map[0]=%08x map[1]=%08x mode=%08x VM-stack=817f2380 manager-row(group/screen/link)=0/7/ffffffff\n",be(ram+0x00282e34u),be(ram+0x00282e38u),be(ram+0x003a2a50u));}
  printf("event%u: CF34C=%u D399C=%u VM=%u class=%u router=%u E9D4=%u async-lookups=%u async-service-boundaries=%u archive-flag-lookups=%u supplied-flags=%08x reuse-callbacks=%u flushes=%u releases=%u acquires=%u args=%08x,%08x,%08x,%08x,%08x,%08x,%08x\n",event,event_callbacks,script_dispatches,vm_entries,class_calls,router_hits,callback_hits,async_lookup_calls,async_service_calls,archive_flag_calls,supplied_archive_flags,archive_reuse_calls,resource_flush_calls,resource_release_calls,resource_acquire_calls,callback_args[0],callback_args[1],callback_args[2],callback_args[3],callback_args[4],callback_args[5],callback_args[6]);
  for(unsigned j=0;j<async_lookup_calls&&j<16;++j)printf("  async lookup[%u] args=%08x,%08x,%08x status=%08x supplied result=00000000\n",j,async_lookup_args[j][0],async_lookup_args[j][1],async_lookup_args[j][2],async_lookup_status[j]);
  for(unsigned j=0;j<async_service_calls&&j<16;++j)printf("  async service[%u] param=%08x status=%08x supplied result=00000000\n",j,async_service_args[j],async_service_status[j]);
  for(unsigned j=0;j<archive_flag_calls&&j<16;++j)printf("  archive-flag lookup[%u]=%08x supplied=%08x\n",j,archive_flag_indices[j],supplied_archive_flags);
  for(unsigned j=0;j<resource_release_calls&&j<16;++j)printf("  resource-release[%u]=%08x,%08x,%08x\n",j,resource_release_args[j][0],resource_release_args[j][1],resource_release_args[j][2]);
  for(unsigned j=0;j<resource_acquire_calls&&j<16;++j)printf("  resource-acquire[%u]=%08x,%08x,%08x\n",j,resource_acquire_args[j][0],resource_acquire_args[j][1],resource_acquire_args[j][2]);
  if(prepared_mode&&!event7_only){char expected_path[4096];int z=snprintf(expected_path,sizeof expected_path,"%s/native-event-%02u-source.bin",prepared_dir,event);if(z<0||(size_t)z>=sizeof expected_path)return 15;uint8_t*expected=malloc(SOURCE_SIZE);if(!expected||!load_exact(expected_path,expected,SOURCE_SIZE)){free(expected);fprintf(stderr,"missing prepared native event source %s\n",expected_path);return 15;}const uint8_t*actual=ram+SCREEN-0x80000000u;unsigned absolute_differences=0,delta_differences=0,printed=0;for(unsigned k=0;k<SOURCE_SIZE;++k){if(actual[k]!=expected[k])++absolute_differences;int original_changed=actual[k]!=prepared_source_start[k];int native_changed=expected[k]!=native_source_start[k];if(original_changed!=native_changed||(original_changed&&actual[k]!=expected[k])){if(printed<24){printf("  event-delta mismatch +%x start-original=%02x end-original=%02x start-native=%02x end-native=%02x\n",k,prepared_source_start[k],actual[k],native_source_start[k],expected[k]);++printed;}++delta_differences;}}printf("  native event-%02u source: delta mismatches=%u/%u bytes; absolute post-source differences=%u/%u\n",event,delta_differences,SOURCE_SIZE,absolute_differences,SOURCE_SIZE);free(expected);}
 }
 if(prepared_mode)printf("Original manager dispatch comparison: D0264(0,7) registered the already-prepared native source with CF8C8/CE994 skipped; D094C events2..5 were independently reset and compared byte-for-byte against native event snapshots. Archive flag result %08x and archive/async services were explicit supplied boundaries; this is not normal archive-loader startup.\n",supplied_archive_flags);
 else printf("Original manager and callback stages: D0264(0,7) registration path passed with %u file-provider lookup(s); separately, CF8C8/CE994 constructed raw entry7 using original class callbacks and supplied font/resources. D094C events2..5 ran on the controlled registered row with archive-flag result %08x and archive flush/release/acquire boundaries supplied explicitly; this does not claim the normal archive-loader startup join.\n",registered_provider_calls,supplied_archive_flags);
 free(event_baseline);free(prepared_source_start);free(native_source_start);
 free(ram);FreeLibrary(h);return 0;
}
