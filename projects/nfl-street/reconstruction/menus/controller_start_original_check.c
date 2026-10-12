#include <windows.h>
#include "moderngekko/module_abi.h"
#include "controller_screen.h"
#include "construct.h"
#include "screen.h"
#include "events.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM_SIZE (24u*1024u*1024u)
#define BASE 0x81100000u
#define FILE_AT 0x1100000u
static unsigned be(const uint8_t*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static void put(uint8_t*p,unsigned v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static uint8_t*load(const char*p,size_t*n){FILE*f=fopen(p,"rb");if(!f)return NULL;if(fseek(f,0,SEEK_END))return NULL;long z=ftell(f);if(z<32||z>8*1024*1024)return NULL;rewind(f);uint8_t*b=malloc((size_t)z);if(!b||fread(b,1,(size_t)z,f)!=(size_t)z){free(b);fclose(f);return NULL;}fclose(f);*n=(size_t)z;return b;}
static int seed(nflstreet_controller_screen*s){
 for(unsigned i=0;i<4;++i){s->game.selection.mapping[i]=s->game.next_mapping[i]=(int8_t)i;s->game.selection.slots[i]=4;s->game.handles[i]=s->game.devices.bindings[i]=UINT32_MAX;snprintf(s->game.selection.profiles[i],20,"Supplied profile %u",i);}
 strcpy(s->game.selection.special[0],"Supplied empty");strcpy(s->game.selection.special[1],"Supplied create");strcpy(s->game.selection.special[2],"Supplied special");s->game.selection.profile_count=4;s->game.devices.query_mode=1;s->game.devices.profile_mode=6;s->game.devices.active_port=0;s->game.devices.status[0]=2;s->supplied_static_archives=(1u<<1)|(1u<<3)|(1u<<9)|(1u<<10);s->classes.shared.rates.clock=16;s->classes.shared.link_enabled=1;s->classes.shared.linked_screen=5;return 1;
}
typedef struct{nflstreet_controller_screen state;unsigned callbacks,requests,acquisitions,queried_device; nflstreet_ui_service_request request_log[4096];uint32_t acquire_log[512];} original_services;
static int emit_device(original_services*x,unsigned fn,unsigned argc,unsigned a,unsigned b){nflstreet_controller_devices*d=&x->state.game.devices;if(d->count>=NFLSTREET_CONTROLLER_DEVICE_EFFECTS)return 0;d->effects[d->count++]=(nflstreet_controller_effect){fn,argc,{a,b,0}};return 1;}
static int record_construct(original_services*x,unsigned pc,uint32_t target,uint32_t property){
 if(x->callbacks>=4096||x->requests>=4096)return 0;nflstreet_ui_service_request*q=&x->request_log[x->requests++];memset(q,0,sizeof *q);q->kind=0;q->class_id=pc==0x801d9244?8:0;q->target=target;q->property=property;++x->callbacks;return 1;
}
static int compare_service(void*ctx,uint8_t*b,size_t n,uint32_t base,const nflstreet_ui_service_request*q,uint32_t*out){original_services*x=ctx;if(x->requests>=4096)return 0;x->request_log[x->requests++]=*q;return nflstreet_controller_screen_service(&x->state,b,n,base,q,out);}
static int run_constructor(const ModernGekkoModuleDesc*m,CPUState*c,uint8_t*b,size_t n,original_services*x){
 (void)b;(void)n;
 c->pc=0x801ce994;c->lr=0;c->downcount=c->cycle_budget=100000000;
 for(unsigned i=0;c->pc&&i<1000000;++i){if(c->pc==0x801d60dc||c->pc==0x801d9244){unsigned pc=c->pc;if(c->gpr[5]||c->gpr[6]||c->gpr[7]||!record_construct(x,pc,c->gpr[3],c->gpr[4]))return 0;c->downcount=-1000000000;m->dispatch(c,pc);}
  else if(c->pc==0x801d5c24){if(c->gpr[3]!=1)return 0;c->gpr[3]=0x817f0000;c->pc=c->lr;}
  else if(c->pc==0x801d5884){if(c->gpr[4]!=2||c->gpr[5]>3)return 0;c->gpr[3]=0x817f3100+c->gpr[5]*0x20;c->pc=c->lr;}
  else if(c->pc==0x801d54a4){if(x->acquisitions>=512)return 0;x->acquire_log[x->acquisitions++]=(c->gpr[4]&65535)<<16|(c->gpr[5]&65535);c->pc=c->lr;}
  else {c->downcount=-1000000000;m->dispatch(c,c->pc);}
  if(c->exception){fprintf(stderr,"Original constructor exception pc=%08x exception=%08x\n",c->pc,c->exception);return 0;}}
 if(c->pc){fprintf(stderr,"Original constructor stopped pc=%08x lr=%08x r3=%08x r4=%08x r5=%08x r6=%08x r7=%08x ex=%08x\n",c->pc,c->lr,c->gpr[3],c->gpr[4],c->gpr[5],c->gpr[6],c->gpr[7],c->exception);return 0;}return 1;
}
static int event_call(const ModernGekkoModuleDesc*m,CPUState*c,uint8_t*b,size_t n,original_services*x,uint32_t fn,uint32_t event){
 (void)n;
 c->pc=fn;c->lr=0;c->gpr[3]=0x817f0000;c->gpr[4]=0x817f0100;c->gpr[5]=0x817f0068;c->gpr[6]=0;c->gpr[7]=0;c->gpr[8]=event;c->gpr[9]=0;c->gpr[10]=0;c->downcount=c->cycle_budget=100000000;
 if(fn==0x801cf610)c->gpr[4]=16;
 if(event>=2&&event<=7){c->pc=0x801d094c;c->gpr[4]=0;c->gpr[5]=event;c->gpr[6]=1;c->gpr[7]=0x817f3300;c->gpr[8]=0;put(b+0x17f3300,0x3f800000);}
 for(unsigned i=0;c->pc&&i<1000000;++i){unsigned pc=c->pc;
  if(pc==0x8012cb94){if(c->gpr[3]&&c->gpr[4]==0){if(c->gpr[5]!=1||x->state.classes.shared.audio_count>=128)return 0;x->state.classes.shared.audio[x->state.classes.shared.audio_count++]=be(b+c->gpr[6]-0x80000000);}c->downcount=-1000000000;m->dispatch(c,pc);}
  else if(pc==0x8013d854){c->pc=c->lr;} /* Captured request, playback boundary supplied. */
  else if(pc==0x801d5aec){c->gpr[3]=0;c->pc=c->lr;}
  else if(pc==0x801d4b3c&&c->gpr[6]==3){if(c->gpr[8]||x->state.classes.shared.link_count>=16)return 0;x->state.classes.shared.links[x->state.classes.shared.link_count++]=be(b+c->gpr[7]-0x80000000);c->pc=c->lr;}
  else if(pc==0x801d4b3c){if(c->gpr[6]!=9||c->gpr[8]>16||x->state.classes.shared.broadcast_count>=128){fprintf(stderr,"Outgoing boundary kind%u argc%u payload%x\n",c->gpr[6],c->gpr[8],c->gpr[7]);return 0;}nflstreet_ui_service_request*q=&x->state.classes.shared.broadcasts[x->state.classes.shared.broadcast_count++];q->kind=2;q->property=be(b+c->gpr[7]-0x80000000);q->argc=c->gpr[8];for(unsigned j=0;j<q->argc;++j)q->args[j]=be(b+c->gpr[9]-0x80000000+j*4);c->pc=c->lr;}
  else if(pc==0x801d5c24){c->gpr[3]=0x817f0000;c->pc=c->lr;}
  else if(pc==0x801d5884){if(c->gpr[4]!=2||c->gpr[5]>3)return 0;c->gpr[3]=0x817f3100+c->gpr[5]*0x20;c->pc=c->lr;}
  else if(pc==0x80127394){c->gpr[3]=x->state.game.selection.profile_count;c->pc=c->lr;}
  else if(pc==0x801273b4){if(c->gpr[3]>=4||c->gpr[4]<0x80000000||c->gpr[4]-0x80000000>RAM_SIZE-20)return 0;strcpy((char*)b+c->gpr[4]-0x80000000,x->state.game.selection.profiles[c->gpr[3]]);c->pc=c->lr;}
  else if(pc==0x80199e58){if(!emit_device(x,pc,0,0,0))return 0;c->pc=c->lr;}
  else if(pc==0x8018013c){if(c->gpr[3]>=8||c->gpr[4]||!emit_device(x,pc,2,c->gpr[3],c->gpr[4]))return 0;x->queried_device=c->gpr[3];c->pc=c->lr;}
  else if(pc==0x80199bcc){unsigned d=x->queried_device;if(d>=8)return 0;c->gpr[3]=x->state.game.devices.status[d];c->pc=c->lr;}
  else if(pc==0x80129aa4){if(c->gpr[3]>=10||!emit_device(x,pc,2,c->gpr[3],c->gpr[4]))return 0;put(b+0x282e34+c->gpr[3]*4,c->gpr[4]);x->state.game.devices.bindings[c->gpr[3]]=c->gpr[4];c->pc=c->lr;}
  else if(pc==0x80129ab8){if(c->gpr[3]>=10)return 0;c->downcount=-1000000000;m->dispatch(c,pc);}
  else if(pc==0x80127798){if(c->gpr[4]!=4||c->gpr[3]<0x80000000||c->gpr[3]-0x80000000>RAM_SIZE-4)return 0;memcpy(b+c->gpr[3]-0x80000000,x->state.game.next_mapping,4);c->pc=c->lr;}
  else if(pc==0x8010c390){c->gpr[3]=x->state.game.devices.query_mode;c->pc=c->lr;}
  else if(pc==0x8010b60c){c->gpr[3]=x->state.game.devices.profile_mode;c->pc=c->lr;}
  else if(pc==0x80009e5c||pc==0x80009e6c){c->gpr[3]=0;c->pc=c->lr;}
  else if(pc==0x80129acc){c->downcount=-1000000000;m->dispatch(c,pc);}
  else if(pc==0x80127910){if(!emit_device(x,pc,1,c->gpr[3],0))return 0;c->pc=c->lr;}
  else if(pc==0x801278b4){if(!emit_device(x,pc,2,c->gpr[3],c->gpr[4]))return 0;c->pc=c->lr;}
  else {c->downcount=-1000000000;m->dispatch(c,pc);}
  if(c->exception){fprintf(stderr,"Original event exception pc=%08x exception=%08x\n",c->pc,c->exception);return 0;}}
 if(c->pc){fprintf(stderr,"Original event stopped pc=%08x\n",c->pc);return 0;}return 1;
}
static int compare_state(const uint8_t*file,size_t n,uint8_t*ram,const nflstreet_controller_screen*native,original_services*actual){
 if(memcmp(file,ram+FILE_AT,n)){for(size_t i=0;i<n;++i)if(file[i]!=ram[FILE_AT+i]){fprintf(stderr,"Source file differs %zx %x/%x\n",i,file[i],ram[FILE_AT+i]);break;}return 0;}
 for(unsigned i=0;i<4;++i){actual->state.game.selection.slots[i]=be(ram+0x282590+i*4);actual->state.game.selection.sides[i]=be(ram+0x2f506c+i*4);actual->state.game.selection.mapping[i]=(int8_t)ram[0x3a2be0+i];unsigned found=0;while(found<10&&be(ram+0x282e34+found*4)!=i)++found;actual->state.game.handles[i]=found==10?UINT32_MAX:found;}
 memcpy(actual->state.game.devices.connected,ram+0x3a4000,8);actual->state.game.committed.choice=be(ram+0x3a2be4);
 for(unsigned i=0;i<native->classes.shared.rates.count;++i)for(unsigned j=0;j<13;++j){uint32_t value=native->classes.shared.rates.records[i].words[j];if(j==7)value=0x817f0100;if(value!=be(ram+0x17e0000+i*52+j*4)){fprintf(stderr,"Rate differs record%u word%u %x/%x\n",i,j,value,be(ram+0x17e0000+i*52+j*4));return 0;}}
 if(native->classes.shared.link_count!=actual->state.classes.shared.link_count||memcmp(native->classes.shared.links,actual->state.classes.shared.links,sizeof native->classes.shared.links)){fprintf(stderr,"Linked requests differ\n");return 0;}
 if(native->classes.shared.audio_count!=actual->state.classes.shared.audio_count||memcmp(native->classes.shared.audio,actual->state.classes.shared.audio,sizeof native->classes.shared.audio)){fprintf(stderr,"Audio requests differ\n");return 0;}
 if(native->classes.shared.broadcast_count!=actual->state.classes.shared.broadcast_count||memcmp(native->classes.shared.broadcasts,actual->state.classes.shared.broadcasts,sizeof native->classes.shared.broadcasts)){fprintf(stderr,"Broadcast differs count%u/%u\n",native->classes.shared.broadcast_count,actual->state.classes.shared.broadcast_count);for(unsigned i=0;i<native->classes.shared.broadcast_count;++i)fprintf(stderr," Request%u prop%x/%x argc%u/%u args%x,%x/%x,%x\n",i,native->classes.shared.broadcasts[i].property,actual->state.classes.shared.broadcasts[i].property,native->classes.shared.broadcasts[i].argc,actual->state.classes.shared.broadcasts[i].argc,native->classes.shared.broadcasts[i].args[0],native->classes.shared.broadcasts[i].args[1],actual->state.classes.shared.broadcasts[i].args[0],actual->state.classes.shared.broadcasts[i].args[1]);return 0;}
 if(memcmp(&native->game,&actual->state.game,sizeof native->game)||native->classes.shared.entry_flags!=be(ram+0x17f0100)||native->classes.shared.rates.count!=be(ram+0x17f0054)){fprintf(stderr,"FFFE owned state mismatch device-effects=%u/%u flags=%08x/%08x rates=%u/%u\n",native->game.devices.count,actual->state.game.devices.count,native->classes.shared.entry_flags,be(ram+0x17f0100),native->classes.shared.rates.count,be(ram+0x17f0054));return 0;}
 return native->classes.shared.rates.flags==be(ram+0x17f0004);
}
int main(int argc,char**argv){
 if(argc!=5)return 2;HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;const ModernGekkoModuleDesc*m=get();if(m->cpu_state_size!=sizeof(CPUState))return 3;
 uint8_t*ram=malloc(RAM_SIZE);FILE*f=fopen(argv[2],"rb");if(!ram||!f||fread(ram,1,RAM_SIZE,f)!=RAM_SIZE)return 4;fclose(f);size_t n=0;uint8_t*source=load(argv[3],&n);if(!source||n!=38682)return 4;unsigned relocated;if(!nflstreet_ui_relocate(source,n,BASE,&relocated))return 5;uint8_t*native_file=malloc(n);if(!native_file)return 4;memcpy(native_file,source,n);
 memcpy(ram+FILE_AT,source,n);memset(ram+0x17f0000,0,0x3000);put(ram+0x17f0008,16);put(ram+0x17f000c,0x8012a414);put(ram+0x17f0104,6);put(ram+0x17f0108,5);put(ram+0x17f0038,1);put(ram+0x17f003c,0x817f0100);put(ram+0x17f0030,0);put(ram+0x17f0110,BASE);put(ram+0x17f0048,9);put(ram+0x17f004c,0x817b0000);put(ram+0x17f0054,0);put(ram+0x17f0058,0x817e0000);memset(ram+0x17e0000,0,64*52);put(ram+0x17f0074,0x817c0000);put(ram+0x17f0088,0x817c0400);memset(ram+0x17b0000,0,36);put(ram+0x17b0000,0x801d60dc);put(ram+0x17b0020,0x801d9244);put(ram+0x17b0004,0x8012cb94);memset(ram+0x17c0000,0,1024);put(ram+0x17c0004,0xfffe);put(ram+0x17c0008,BASE);
 nflstreet_controller_screen native={0};seed(&native);original_services actual={0};seed(&actual.state);
 for(unsigned i=0;i<3;++i){const char*text=(char*)ram+0x256de0+(i+4)*10+12;const char*end=memchr(text,0,20);if(!end)return 5;memcpy(native.game.selection.special[i],text,(size_t)(end-text)+1);memcpy(actual.state.game.selection.special[i],native.game.selection.special[i],20);}
 for(unsigned i=0;i<4;++i){char path[4096];snprintf(path,sizeof path,"%s/uis_font/%05u.bin",argv[4],i);size_t z=0;uint8_t*fb=load(path,&z);nflstreet_font*nf=i==3?&native.classes.shared.font:&native.classes.fonts[i];nflstreet_font*af=i==3?&actual.state.classes.shared.font:&actual.state.classes.fonts[i];unsigned at=0x1740000+i*0x10000;if(!fb||z>0x10000)return 5;memcpy(ram+at,fb,z);free(fb);if(!nflstreet_font_open(ram+at,z,nf))return 5;*af=*nf;unsigned rec=0x17f3000+i*0x20,asset=0x17f3100+i*0x20;put(ram+rec+4,0x80000000+at);put(ram+rec+8,0x80000000+at+nf->glyphs);put(ram+rec+12,0x80000000+at+nf->kerning);put(ram+rec+16,0x80000000+at+nf->atlas);put(ram+asset,0x80000000+rec);}
 for(unsigned i=0;i<4;++i){put(ram+0x282590+i*4,native.game.selection.slots[i]);put(ram+0x2f506c+i*4,native.game.selection.sides[i]);ram[0x3a2be0+i]=(uint8_t)native.game.selection.mapping[i];}memcpy(ram+0x3a4000,native.game.devices.connected,8);for(unsigned i=0;i<10;++i)put(ram+0x282e34+i*4,native.game.devices.bindings[i]);put(ram+0x3a2be4,0);
 original_services nativelog={0};nativelog.state=native;if(!nflstreet_ui_construct(native_file,n,BASE,0,UINT32_MAX,0x101,compare_service,&nativelog,sizeof nativelog))return 6;native=nativelog.state;
 CPUState c={0};c.ram=ram;c.ram_size=RAM_SIZE;c.msr=0x2000;c.gpr[1]=0x81700000;c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;c.gpr[3]=0x817f0000;c.gpr[4]=0x817f0100;c.gpr[5]=0;c.gpr[6]=UINT32_MAX;
 if(!run_constructor(m,&c,source,n,&actual)){fprintf(stderr,"Original class construction unsupported\n");return 7;}
 if(actual.requests!=nativelog.requests||memcmp(actual.request_log,nativelog.request_log,nativelog.requests*sizeof nativelog.request_log[0])){fprintf(stderr,"Class callback request trace differs native=%u original=%u\n",nativelog.requests,actual.requests);return 8;}
 if(memcmp(native_file,ram+FILE_AT,n)){for(size_t i=0;i<n;++i)if(native_file[i]!=ram[FILE_AT+i]){fprintf(stderr,"Construction file diff at %zx native=%02x original=%02x\n",i,native_file[i],ram[FILE_AT+i]);break;}for(unsigned i=0;i<actual.requests;++i){nflstreet_ui_service_request*q=&actual.request_log[i];if(q->target>=BASE+0x242c&&q->target<BASE+0x2530)fprintf(stderr,"Original class request #%u kind=%u class=%u property=%08x target=%08x argc=%u\n",i,q->kind,q->class_id,q->property,q->target,q->argc);}return 8;}
 uint8_t*joined_file=malloc(n);if(!joined_file)return 4;memcpy(joined_file,native_file,n);nflstreet_controller_screen joined=native;
 if(!nflstreet_controller_screen_start(joined_file,n,BASE,&joined))return 17;
 uint8_t*rollback=malloc(n);if(!rollback)return 4;memcpy(rollback,native_file,n);nflstreet_controller_screen rejected=native;rejected.supplied_static_archives=0;nflstreet_controller_screen before_reject=rejected;
 if(nflstreet_controller_screen_start(rollback,n,BASE,&rejected)||memcmp(rollback,native_file,n)||memcmp(&rejected,&before_reject,sizeof rejected))return 18;free(rollback);
 memcpy(native_file,ram+FILE_AT,n);put(ram+0x17b0000,0x801d60dc);put(ram+0x17b0020,0x801d9244);put(ram+0x17b0004,0x8012cb94);put(ram+0x17f0074,0x817c0000);put(ram+0x17f0068+12,0x817c0000);put(ram+0x1700008,0x817f3200);
 unsigned executed=0;if(!nflstreet_ui_events_memory(native_file,n,BASE,6,5,&native.classes.shared.entry_flags,0,0xfffe,0,0,NULL,nflstreet_controller_screen_service,&native,sizeof native,&executed,NULL,nflstreet_controller_screen_memory)){fprintf(stderr,"Native FFFE event unsupported\n");return 9;}
 /* Original group0/entry6 dispatcher and its actual 8011BC68 callback remain in the DLL. */
 if(!event_call(m,&c,ram,n,&actual,0x801cf34c,0xfffffffe)){fprintf(stderr,"Original child-first FFFE dispatcher reached an unsupported service boundary\n");return 10;}
 if(memcmp(native_file,ram+FILE_AT,n)){for(size_t i=0;i<n;++i)if(native_file[i]!=ram[FILE_AT+i]){fprintf(stderr,"FFFE file diff at %zx native=%02x original=%02x\n",i,native_file[i],ram[FILE_AT+i]);break;}return 11;}
 if(!compare_state(native_file,n,ram,&native,&actual))return 12;
 unsigned creation=executed,root=be(native_file+be(native_file+4)-BASE)-BASE;put(native_file+root+4,1);put(ram+FILE_AT+root+4,1);
 if(!nflstreet_ui_events_memory(native_file,n,BASE,6,5,&native.classes.shared.entry_flags,0,0xfffc,0,0,NULL,nflstreet_controller_screen_service,&native,sizeof native,&executed,NULL,nflstreet_controller_screen_memory)||!event_call(m,&c,ram,n,&actual,0x801cf34c,0xfffffffc)||!compare_state(native_file,n,ram,&native,&actual))return 15;
 if(memcmp(joined_file,native_file,n)||memcmp(&joined,&native,sizeof native))return 19;free(joined_file);
 unsigned activation=executed,device_requests=native.game.devices.count;
 for(unsigned frame=1;frame<=160;++frame){native.game.devices.count=actual.state.game.devices.count=0;memset(native.game.devices.effects,0,sizeof native.game.devices.effects);memset(actual.state.game.devices.effects,0,sizeof actual.state.game.devices.effects);
  if(!nflstreet_controller_screen_advance(native_file,n,BASE,&native,16)||!event_call(m,&c,ram,n,&actual,0x801cf610,0)||!compare_state(native_file,n,ram,&native,&actual)){fprintf(stderr,"Controller timed update differs frame%u\n",frame);return 16;}device_requests+=native.game.devices.count;
 }
 static const unsigned controls[]={2,3,5,4,7};
 for(unsigned phase=0;phase<sizeof controls/sizeof *controls;++phase){native.game.devices.count=actual.state.game.devices.count=0;memset(native.game.devices.effects,0,sizeof native.game.devices.effects);memset(actual.state.game.devices.effects,0,sizeof actual.state.game.devices.effects);
  if(!nflstreet_controller_screen_input(native_file,n,BASE,&native,0,(uint16_t)controls[phase],1.0f)||!event_call(m,&c,ram,n,&actual,0x801cf34c,controls[phase])||!compare_state(native_file,n,ram,&native,&actual)){fprintf(stderr,"Input differs phase%u event%u\n",phase,controls[phase]);return 20;}
  printf("Source input event%u exact: slot%u side%x commit%u pending%u broadcasts%u\n",controls[phase],native.game.selection.slots[0],native.game.selection.sides[0],native.game.committed.count,native.classes.shared.pending_count,native.classes.shared.broadcast_count);
  for(unsigned frame=0;frame<20;++frame){native.game.devices.count=actual.state.game.devices.count=0;memset(native.game.devices.effects,0,sizeof native.game.devices.effects);memset(actual.state.game.devices.effects,0,sizeof actual.state.game.devices.effects);if(!nflstreet_controller_screen_advance(native_file,n,BASE,&native,16)||!event_call(m,&c,ram,n,&actual,0x801cf610,0)||!compare_state(native_file,n,ram,&native,&actual)){fprintf(stderr,"Input tick differs phase%u frame%u\n",phase,frame);return 21;}}
 }
 if(native.classes.shared.broadcast_count!=2||native.classes.shared.broadcasts[1].property!=1||native.classes.shared.broadcasts[1].argc!=2||native.classes.shared.broadcasts[1].args[0]!=UINT32_MAX||native.classes.shared.broadcasts[1].args[1]!=UINT32_MAX||native.classes.shared.link_count!=1||native.classes.shared.links[0]!=5){fprintf(stderr,"Completion payload differs from supplied-context outcome broadcasts%u property%x argc%u args%x,%x links%u target%x\n",native.classes.shared.broadcast_count,native.classes.shared.broadcasts[1].property,native.classes.shared.broadcasts[1].argc,native.classes.shared.broadcasts[1].args[0],native.classes.shared.broadcasts[1].args[1],native.classes.shared.link_count,native.classes.shared.links[0]);return 22;}
 puts("Controller source inputs exact:5 translated press events and100 timedupdates;profile cycle, neutral/side selection and delayed completion1(FFFFFFFF,FFFFFFFF), linked5 request match; original801D094C/801CF34C/VM/callback dispatched. Selection recipients and hardware translation remain external.");
 printf("Controller construction and child-first FFFE exact:38682-byte owned source comparison;native scripts=%u;original class callbacks=%u. Selected FFFC scripts=%u;160 timedupdates exact;device requests=%u;broadcasts=%u. Supplied fonts0..3, static archives1/3/9/10 and device/profile inputs; original class, VM and 8011BC68 code dispatched. Public startup join and unsupported-archive rollback pass. No live original UI, GX rendering, Windows discovery, or selected-match startup claim.\n",creation,actual.callbacks,activation,device_requests,1u);
 free(native_file);free(source);free(ram);FreeLibrary(h);return 0;
}
