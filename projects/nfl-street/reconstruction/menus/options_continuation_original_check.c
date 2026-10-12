#include <windows.h>
#include "options_screen.h"
#include "events.h"
#include "moderngekko/module_abi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned be(const unsigned char*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static void put(unsigned char*p,unsigned v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static unsigned load_count;static nflstreet_ui_service_request loads[8];
static int navigation_run(const ModernGekkoModuleDesc*m,CPUState*c,nflstreet_ui_options_screen*actual){uint8_t*ram=c->ram;
for(unsigned i=0;c->pc&&i<1000000;++i){unsigned fn=c->pc,ac=99,value=0;
switch(fn){case 0x80004668:ac=0;break;case 0x8014e29c:ac=1;if(c->gpr[3]>=24)return 7;value=actual->profile[c->gpr[3]];break;case 0x801a6178:ac=2;break;case 0x801440f0:case 0x80143fc0:case 0x80143ee0:case 0x801441f4:ac=1;break;case 0x801415b0:ac=2;break;}
if(fn==0x8014e134){ac=2;if(c->gpr[3]>=24)return 7;actual->profile[c->gpr[3]]=c->gpr[4];}
if(fn==0x80137d38||fn==0x80141d54||fn==0x80199bcc)ac=0;
if(fn==0x80141818||fn==0x80137c08||fn==0x80143eb8||fn==0x801a65d0)ac=1;
if(fn==0x8012c498){ac=0;value=actual->query_c498;}if(fn==0x8012c440)ac=2;
if(fn==0x801416b0||fn==0x8018013c)ac=2;
if(fn==0x8014164c||fn==0x80137a50)ac=3;
if(ac!=99){if(actual->effect_count>=128)return 7;unsigned e=actual->effect_count++;actual->effects[e].function=fn;actual->effects[e].argc=ac;for(unsigned j=0;j<ac;++j)actual->effects[e].args[j]=c->gpr[3+j];c->gpr[3]=value;c->pc=c->lr;continue;}
if(c->pc==0x8012cb94){if(c->gpr[3]&&c->gpr[4]==0){if(actual->shared.audio_count>=128||c->gpr[5]!=1)return 7;actual->shared.audio[actual->shared.audio_count++]=be(ram+c->gpr[6]-0x80000000);}c->downcount=-1000000000;m->dispatch(c,c->pc);continue;}
if(c->pc==0x8013d854){c->pc=c->lr;continue;}
if(c->pc==0x801d5aec){c->gpr[3]=0;c->pc=c->lr;continue;}
if(c->pc==0x801d5c24){c->gpr[3]=0x817f0000;c->pc=c->lr;continue;}
if(c->pc==0x801d5884){if(c->gpr[4]!=2||(c->gpr[5]!=0&&c->gpr[5]!=2))return 7;c->gpr[3]=c->gpr[5]?0x817f3120:0x817f3100;c->pc=c->lr;continue;}
if(c->pc==0x801d4b3c){if(c->gpr[6]==0){if(load_count>=8||c->gpr[8]>16)return 7;uint8_t*p=ram+c->gpr[7]-0x80000000;nflstreet_ui_service_request*q=loads+load_count++;q->kind=9;q->target=be(p);q->source=q->link=be(p+4);q->argc=c->gpr[8];for(unsigned j=0;j<q->argc;++j)q->args[j]=be(ram+c->gpr[9]-0x80000000+j*4);c->pc=c->lr;continue;}if(c->gpr[6]==3){++actual->return_count;actual->return_identity=be(ram+c->gpr[7]-0x80000000);c->pc=c->lr;continue;}if(c->gpr[6]==5){if(actual->shared.pending_count>=128||c->gpr[8]!=1)return 7;unsigned payload=c->gpr[7]-0x80000000;nflstreet_ui_service_request*q=&actual->shared.pending[actual->shared.pending_count++];q->kind=7;q->property=(uint32_t)(int32_t)(int16_t)(be(ram+payload)>>16);q->target=be(ram+payload+8);q->argc=2;q->args[0]=be(ram+payload+4);q->args[1]=be(ram+c->gpr[9]-0x80000000);}else{if(c->gpr[6]!=9||c->gpr[8]>16||actual->shared.broadcast_count>=128)return 7;nflstreet_ui_service_request*q=&actual->shared.broadcasts[actual->shared.broadcast_count++];q->kind=2;q->property=be(ram+c->gpr[7]-0x80000000);q->argc=c->gpr[8];for(unsigned j=0;j<q->argc;++j)q->args[j]=be(ram+c->gpr[9]-0x80000000+j*4);}c->pc=c->lr;continue;}
if(c->pc==0x801d54a4||c->pc==0x801d5918){printf("Unexpected resource I/O boundary %x\n",c->pc);return 7;}
c->downcount=-1000000000;m->dispatch(c,c->pc);if(c->exception)return 8;}
return c->pc?9:0;}
int main(int argc,char**argv){if(argc!=5&&argc!=6)return 2;HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);const ModernGekkoModuleDesc*m=get();unsigned char*ram=malloc(24*1024*1024);FILE*f=fopen(argv[2],"rb");if(!ram||!f||fread(ram,1,24*1024*1024,f)!=24*1024*1024)return 4;fclose(f);f=fopen(argv[3],"rb");if(!f||fread(ram+0x1100000,1,112344,f)!=112344)return 4;fclose(f);
memset(ram+0x17f0000,0,0x3000);put(ram+0x17f0008,16);put(ram+0x17f000c,0x8012a414);put(ram+0x17f0104,0x00050000);put(ram+0x17f0108,2);put(ram+0x17f0110,0x81100000);put(ram+0x17f0048,9);put(ram+0x17f004c,0x817b0000);memset(ram+0x17b0000,0,36);put(ram+0x17b0000,0x801d60dc);put(ram+0x17b0020,0x801d9244);put(ram+0x17b0004,0x8012cb94);put(ram+0x17f0038,1);put(ram+0x17f003c,0x817f0100);memset(ram+0x17c0000,0,1024);put(ram+0x17c0004,0xfffe);put(ram+0x17c0008,0x81104adc);put(ram+0x17f200c,0x817c0010);put(ram+0x17f2010,0x81117832);
CPUState c={0};c.ram=ram;c.ram_size=24*1024*1024;c.msr=0x2000;c.gpr[1]=0x81700000;c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;c.gpr[3]=0x817f0000;c.gpr[4]=0x817c0000;c.gpr[5]=0x817f2000;c.gpr[6]=0x817f0100;c.gpr[7]=0x81104adc;c.pc=0x801d16a0;c.cycle_budget=100000000;
for(unsigned e=0;e<=2;e+=2){char path[4096];snprintf(path,sizeof path,"%s/%05u.bin",argv[4],e);f=fopen(path,"rb");if(!f)return 4;fseek(f,0,SEEK_END);long z=ftell(f);rewind(f);unsigned at=e?0x1790000:0x17a0000;if(z<32||z>65536||fread(ram+at,1,z,f)!=(size_t)z)return 4;fclose(f);nflstreet_font font;if(!nflstreet_font_open(ram+at,z,&font))return 4;unsigned rec=e?0x17f3020:0x17f3000,asset=e?0x17f3120:0x17f3100;put(ram+rec+4,0x80000000+at);put(ram+rec+8,0x80000000+at+font.glyphs);put(ram+rec+12,0x80000000+at+font.kerning);put(ram+rec+16,0x80000000+at+font.atlas);put(ram+asset,0x80000000+rec);}
put(ram+0x17f0054,0);put(ram+0x17f0058,0x817e0000);memset(ram+0x17e0000,0,64*52);put(ram+0x17f0074,0x817c0000);put(ram+0x17f0088,0x817c0400);

nflstreet_ui_options_screen native={0},actual={0};native.shared.rates.clock=actual.shared.rates.clock=16;
for(unsigned e=0;e<=2;e+=2){unsigned at=e?0x1790000:0x17a0000;char path[4096];snprintf(path,sizeof path,"%s/%05u.bin",argv[4],e);f=fopen(path,"rb");fseek(f,0,SEEK_END);long z=ftell(f);fclose(f);if(!nflstreet_font_open(ram+at,z,&native.fonts[e]))return 4;actual.fonts[e]=native.fonts[e];}
static const unsigned map[21]={23,0,9,7,11,10,12,13,3,4,5,6,15,16,17,18,1,2,1,19,20};for(unsigned i=0;i<21;++i)if(be(ram+0x282aac+i*4)!=map[i])return 4;

uint8_t*owned=malloc(112344);memcpy(owned,ram+0x1100000,112344);
unsigned executed=0;if(!nflstreet_ui_events(owned,112344,0x81100000,0x50000,&native.shared.entry_flags,0,0xfffe,0,0,NULL,nflstreet_ui_options_screen_service,&native,sizeof native,&executed)){puts("Native child activation failed");return 5;}
put(ram+0x17f0074,0x817c0000);put(ram+0x17f0068+12,0x817c0000);put(ram+0x1700008,0x817f3200);c.pc=0x801cf34c;c.gpr[3]=0x817f0000;c.gpr[4]=0x817f0100;c.gpr[5]=0x817f0068;c.gpr[6]=0;c.gpr[7]=0;c.gpr[8]=0xfffffffe;c.gpr[9]=0;c.gpr[10]=0;
for(unsigned i=0;c.pc&&i<1000000;++i){unsigned fn=c.pc,ac=99,value=0;
switch(fn){case 0x80004668:ac=0;break;case 0x8014e29c:ac=1;if(c.gpr[3]>=24)return 7;value=actual.profile[c.gpr[3]];break;case 0x801a6178:ac=2;break;case 0x801440f0:case 0x80143fc0:case 0x80143ee0:case 0x801441f4:ac=1;break;case 0x801415b0:ac=2;break;}
if(ac!=99){if(actual.options.count>=64)return 7;actual.options.requests[actual.options.count++]=(nflstreet_ui_options_request){fn,ac,{ac?c.gpr[3]:0,ac>1?c.gpr[4]:0}};c.gpr[3]=value;c.pc=c.lr;continue;}
if(c.pc==0x801d5aec){c.gpr[3]=0;c.pc=c.lr;continue;}
if(c.pc==0x801d5c24){c.gpr[3]=0x817f0000;c.pc=c.lr;continue;}
if(c.pc==0x801d5884){if(c.gpr[4]!=2||(c.gpr[5]!=0&&c.gpr[5]!=2))return 7;c.gpr[3]=c.gpr[5]?0x817f3120:0x817f3100;c.pc=c.lr;continue;}
if(c.pc==0x801d4b3c){if(c.gpr[6]!=9||c.gpr[8]>16||actual.shared.broadcast_count>=128)return 7;nflstreet_ui_service_request*q=&actual.shared.broadcasts[actual.shared.broadcast_count++];q->kind=2;q->property=be(ram+c.gpr[7]-0x80000000);q->argc=c.gpr[8];for(unsigned j=0;j<q->argc;++j)q->args[j]=be(ram+c.gpr[9]-0x80000000+j*4);c.pc=c.lr;continue;}
if(c.pc==0x801d54a4||c.pc==0x801d5918){printf("Unexpected resource I/O boundary %x\n",c.pc);return 7;}
c.downcount=-1000000000;m->dispatch(&c,c.pc);if(c.exception)return 8;}
if(c.pc)return 9;
put(owned+0x4adc+4,1);put(ram+0x1100000+0x4adc+4,1);unsigned activated=0;
if(!nflstreet_ui_events(owned,112344,0x81100000,0x50000,&native.shared.entry_flags,0,0xfffc,0,0,NULL,nflstreet_ui_options_screen_service,&native,sizeof native,&activated)){puts("Native activation event failed");return 21;}
c.pc=0x801cf34c;c.lr=0;c.gpr[3]=0x817f0000;c.gpr[4]=0x817f0100;c.gpr[5]=0x817f0068;c.gpr[6]=0;c.gpr[7]=0;c.gpr[8]=0xfffffffc;c.gpr[9]=0;c.gpr[10]=0;put(ram+0x1700008,0x817f3200);
for(unsigned i=0;c.pc&&i<1000000;++i){unsigned fn=c.pc,ac=99,value=0;
switch(fn){case 0x80004668:ac=0;break;case 0x8014e29c:ac=1;if(c.gpr[3]>=24)return 7;value=actual.profile[c.gpr[3]];break;case 0x801a6178:ac=2;break;case 0x801440f0:case 0x80143fc0:case 0x80143ee0:case 0x801441f4:ac=1;break;case 0x801415b0:ac=2;break;}
if(ac!=99){if(actual.options.count>=64)return 7;actual.options.requests[actual.options.count++]=(nflstreet_ui_options_request){fn,ac,{ac?c.gpr[3]:0,ac>1?c.gpr[4]:0}};c.gpr[3]=value;c.pc=c.lr;continue;}
if(c.pc==0x801d5aec){c.gpr[3]=0;c.pc=c.lr;continue;}
if(c.pc==0x801d5c24){c.gpr[3]=0x817f0000;c.pc=c.lr;continue;}
if(c.pc==0x801d5884){if(c.gpr[4]!=2||(c.gpr[5]!=0&&c.gpr[5]!=2))return 7;c.gpr[3]=c.gpr[5]?0x817f3120:0x817f3100;c.pc=c.lr;continue;}
if(c.pc==0x801d4b3c){if(c.gpr[6]!=9||c.gpr[8]>16||actual.shared.broadcast_count>=128)return 7;nflstreet_ui_service_request*q=&actual.shared.broadcasts[actual.shared.broadcast_count++];q->kind=2;q->property=be(ram+c.gpr[7]-0x80000000);q->argc=c.gpr[8];for(unsigned j=0;j<q->argc;++j)q->args[j]=be(ram+c.gpr[9]-0x80000000+j*4);c.pc=c.lr;continue;}
if(c.pc==0x801d54a4||c.pc==0x801d5918){printf("Unexpected resource I/O boundary %x\n",c.pc);return 7;}
c.downcount=-1000000000;m->dispatch(&c,c.pc);if(c.exception)return 8;}
if(c.pc)return 22;

unsigned r=0x3a9a40;actual.options.word_6da8=be(ram+r-0x6da8);actual.options.byte_6db0=ram[r-0x6db0];actual.options.word_6da0=be(ram+r-0x6da0);actual.options.word_6dac=be(ram+r-0x6dac);actual.options.word_6d9c=be(ram+r-0x6d9c);actual.options.byte_59f8=ram[r-0x59f8];
if(memcmp(owned,ram+0x1100000,112344)||memcmp(&native.options,&actual.options,sizeof native.options)||memcmp(native.shared.broadcasts,actual.shared.broadcasts,sizeof actual.shared.broadcasts)||native.shared.broadcast_count!=actual.shared.broadcast_count||native.shared.entry_flags!=be(ram+0x17f0100)||native.shared.rates.count!=be(ram+0x17f0054)){for(unsigned i=0;i<112344;++i)if(owned[i]!=ram[0x1100000+i]){printf("Child file diff=%x %x/%x\n",i,owned[i],ram[0x1100000+i]);break;}printf("Child join mismatch rates=%u/%u\n",native.shared.rates.count,be(ram+0x17f0054));return 10;}
for(unsigned i=0;i<native.shared.rates.count;++i)for(unsigned j=0;j<13;++j){unsigned v=native.shared.rates.records[i].words[j];if(j==7)v=0x817f0100;if(v!=be(ram+0x17e0000+i*52+j*4)){printf("Rate mismatch %u/%u %x/%x\n",i,j,v,be(ram+0x17e0000+i*52+j*4));return 12;}}
unsigned first_broadcast_tick=0,previous_rates=native.shared.rates.count,previous_broadcasts=actual.shared.broadcast_count;
for(unsigned frame=1;frame<=160;++frame){
 if(!nflstreet_ui_options_screen_advance(owned,112344,0x81100000,&native,16)){fprintf(stderr,"Native Options tick failed frame=%u\n",frame);return 15;}
 c.pc=0x801cf610;c.lr=0;c.gpr[3]=0x817f0000;c.gpr[4]=16;
for(unsigned i=0;c.pc&&i<1000000;++i){unsigned fn=c.pc,ac=99,value=0;
switch(fn){case 0x80004668:ac=0;break;case 0x8014e29c:ac=1;if(c.gpr[3]>=24)return 7;value=actual.profile[c.gpr[3]];break;case 0x801a6178:ac=2;break;case 0x801440f0:case 0x80143fc0:case 0x80143ee0:case 0x801441f4:ac=1;break;case 0x801415b0:ac=2;break;}
if(ac!=99){if(actual.options.count>=64)return 7;actual.options.requests[actual.options.count++]=(nflstreet_ui_options_request){fn,ac,{ac?c.gpr[3]:0,ac>1?c.gpr[4]:0}};c.gpr[3]=value;c.pc=c.lr;continue;}
if(c.pc==0x801d5aec){c.gpr[3]=0;c.pc=c.lr;continue;}
if(c.pc==0x801d5c24){c.gpr[3]=0x817f0000;c.pc=c.lr;continue;}
if(c.pc==0x801d5884){if(c.gpr[4]!=2||(c.gpr[5]!=0&&c.gpr[5]!=2))return 7;c.gpr[3]=c.gpr[5]?0x817f3120:0x817f3100;c.pc=c.lr;continue;}
if(c.pc==0x801d4b3c){if(c.gpr[6]==5){if(actual.shared.pending_count>=128||c.gpr[8]!=1)return 7;unsigned payload=c.gpr[7]-0x80000000;nflstreet_ui_service_request*q=&actual.shared.pending[actual.shared.pending_count++];q->kind=7;q->property=(uint32_t)(int32_t)(int16_t)(be(ram+payload)>>16);q->target=be(ram+payload+8);q->argc=2;q->args[0]=be(ram+payload+4);q->args[1]=be(ram+c.gpr[9]-0x80000000);}else{if(c.gpr[6]!=9||c.gpr[8]>16||actual.shared.broadcast_count>=128)return 7;nflstreet_ui_service_request*q=&actual.shared.broadcasts[actual.shared.broadcast_count++];q->kind=2;q->property=be(ram+c.gpr[7]-0x80000000);q->argc=c.gpr[8];for(unsigned j=0;j<q->argc;++j)q->args[j]=be(ram+c.gpr[9]-0x80000000+j*4);}c.pc=c.lr;continue;}
if(c.pc==0x801d54a4||c.pc==0x801d5918){printf("Unexpected resource I/O boundary %x\n",c.pc);return 7;}
c.downcount=-1000000000;m->dispatch(&c,c.pc);if(c.exception)return 8;}

 if(c.pc)return 16;
 if(actual.shared.broadcast_count!=previous_broadcasts){if(!first_broadcast_tick)first_broadcast_tick=frame;printf("Original outgoing request at tick%u:kind=%u property=%x argc=%u arg0=%x\n",frame,actual.shared.broadcasts[previous_broadcasts].kind,actual.shared.broadcasts[previous_broadcasts].property,actual.shared.broadcasts[previous_broadcasts].argc,actual.shared.broadcasts[previous_broadcasts].args[0]);previous_broadcasts=actual.shared.broadcast_count;}if(native.shared.rates.count!=previous_rates){printf("Scheduled record count at tick%u:%u ->%u\n",frame,previous_rates,native.shared.rates.count);previous_rates=native.shared.rates.count;}
 if(memcmp(owned,ram+0x1100000,112344)||native.shared.entry_flags!=be(ram+0x17f0100)||native.shared.rates.count!=be(ram+0x17f0054)||native.shared.rates.flags!=be(ram+0x17f0004)||native.shared.broadcast_count!=actual.shared.broadcast_count||native.shared.pending_count!=actual.shared.pending_count||memcmp(native.shared.broadcasts,actual.shared.broadcasts,sizeof actual.shared.broadcasts)||memcmp(native.shared.pending,actual.shared.pending,sizeof actual.shared.pending)||memcmp(native.options.requests,actual.options.requests,sizeof actual.options.requests)||native.options.count!=actual.options.count){fprintf(stderr,"Options tick mismatch frame=%u rates=%u/%u pending=%u/%u\n",frame,native.shared.rates.count,be(ram+0x17f0054),native.shared.pending_count,actual.shared.pending_count);for(unsigned i=0;i<112344;++i)if(owned[i]!=ram[0x1100000+i]){fprintf(stderr,"File diff=%x %x/%x\n",i,owned[i],ram[0x1100000+i]);break;}return 17;}
 for(unsigned i=0;i<native.shared.rates.count;++i)for(unsigned j=0;j<13;++j){unsigned v=native.shared.rates.records[i].words[j];if(j==7)v=0x817f0100;if(v!=be(ram+0x17e0000+i*52+j*4)){fprintf(stderr,"Tick rate mismatch frame=%u record=%u word=%u\n",frame,i,j);return 18;}}
}
ram[0x3a9a40-0x6f78]=ram[0x3a9a40-0x7fcc]=ram[0x3a9a40-0x6dd0]=0;put(ram+0x3a9a40-0x6f7c,0);
 native.shared.callback_byte_6f78=0;native.coroutines.capacity=4;native.effect_count=actual.effect_count=0;memset(native.effects,0,sizeof native.effects);memset(actual.effects,0,sizeof actual.effects);
 put(ram+0x17f005c,4);put(ram+0x17f0060,0);put(ram+0x17f0064,0x817d0000);memset(ram+0x17d0000,0,320);
 if(!nflstreet_ui_options_screen_control(owned,112344,0x81100000,&native,6)){puts("Native Back suspension failed");return 24;}
 c.pc=0x801cf34c;c.lr=0;c.gpr[3]=0x817f0000;c.gpr[4]=0x817f0100;c.gpr[5]=0x817f0068;c.gpr[6]=0;c.gpr[7]=6;c.gpr[8]=6;c.gpr[9]=1;c.gpr[10]=0x817f3300;put(ram+0x17f3300,0x3f800000);put(ram+0x1700008,0x817f3200);
 if(navigation_run(m,&c,&actual))return 25;
 nflstreet_ui_continuation*frame=native.coroutines.frames;
 if(native.coroutines.count!=1||be(ram+0x17f0060)!=1||load_count!=1||memcmp(&frame->request,loads,sizeof frame->request)||memcmp(owned,ram+0x1100000,112344)||native.shared.entry_flags!=be(ram+0x17f0100)||frame->vm.pc!=be(ram+0x17d0010)||0x817c0000+4*frame->vm.sp!=be(ram+0x17d000c)||frame->vm.root!=be(ram+0x17d0014)){fprintf(stderr,"Back suspension mismatch count=%u/%u queue=%u PC=%x/%x SP=%u/%x\n",native.coroutines.count,be(ram+0x17f0060),load_count,frame->vm.pc,be(ram+0x17d0010),frame->vm.sp,be(ram+0x17d000c));return 26;}
 for(unsigned i=0;i<frame->vm.sp;++i)if(frame->vm.words[i]!=be(ram+0x17c0000+4*i)){fprintf(stderr,"Active frozen stack differs at%u %x/%x\n",i,frame->vm.words[i],be(ram+0x17c0000+4*i));return 27;}
uint8_t*saved_ram=malloc(24*1024*1024),*saved_file=malloc(112344);if(!saved_ram||!saved_file)return 4;memcpy(saved_ram,ram,24*1024*1024);memcpy(saved_file,owned,112344);nflstreet_ui_options_screen saved_native=native,saved_actual=actual;
 for(unsigned result=0;result<3;++result){memcpy(ram,saved_ram,24*1024*1024);memcpy(owned,saved_file,112344);native=saved_native;actual=saved_actual;native.effect_count=actual.effect_count=0;memset(native.effects,0,sizeof native.effects);memset(actual.effects,0,sizeof actual.effects);load_count=0;memset(loads,0,sizeof loads);
  unsigned completed=0;if(!nflstreet_ui_options_screen_complete(owned,112344,0x81100000,&native,0x60004,result,&completed)){fprintf(stderr,"Native actual continuation failed result=%u\n",result);return 28;}
  c.pc=0x801d0310;c.lr=0;c.gpr[3]=0x817f0000;c.gpr[4]=6;c.gpr[5]=4;c.gpr[6]=result;if(navigation_run(m,&c,&actual))return 29;
  if(!completed||memcmp(owned,ram+0x1100000,112344)||native.shared.entry_flags!=be(ram+0x17f0100)||native.coroutines.count!=be(ram+0x17f0060)||native.return_count!=actual.return_count||native.return_identity!=actual.return_identity||native.effect_count!=actual.effect_count||memcmp(native.effects,actual.effects,sizeof native.effects)){fprintf(stderr,"Actual continuation mismatch result=%u frames=%u/%u effects=%u/%u\n",result,native.coroutines.count,be(ram+0x17f0060),native.effect_count,actual.effect_count);return 30;}
  for(unsigned tick=0;tick<160;++tick){if(!nflstreet_ui_options_screen_advance(owned,112344,0x81100000,&native,16))return 31;c.pc=0x801cf610;c.lr=0;c.gpr[3]=0x817f0000;c.gpr[4]=16;if(navigation_run(m,&c,&actual))return 32;
   if(memcmp(owned,ram+0x1100000,112344)||native.shared.entry_flags!=be(ram+0x17f0100)||native.shared.rates.count!=be(ram+0x17f0054)||native.return_count!=actual.return_count||native.return_identity!=actual.return_identity||native.effect_count!=actual.effect_count||memcmp(native.effects,actual.effects,sizeof native.effects)){fprintf(stderr,"Resumed tick mismatch result=%u tick=%u rates=%u/%u effects=%u/%u return=%u/%u\n",result,tick,native.shared.rates.count,be(ram+0x17f0054),native.effect_count,actual.effect_count,native.return_count,actual.return_count);return 33;}
   for(unsigned i=0;i<native.shared.rates.count;++i)for(unsigned j=0;j<13;++j){unsigned v=native.shared.rates.records[i].words[j];if(j==7)v=0x817f0100;if(v!=be(ram+0x17e0000+i*52+j*4))return 34;}
  }
  printf("Actual Options returned word%u plus160 ticks exact:frames=%u return=%x count=%u\n",result,native.coroutines.count,native.return_identity,native.return_count);
 }
 printf("Original Options Back -> owned suspended frame exact:destination=%08x argc=%u PC=%x SP=%u;complete source file/flags,active stack and load request;recipient/card I/O separate\n",frame->request.target,frame->request.argc,frame->vm.pc,frame->vm.sp);
 free(owned);free(ram);FreeLibrary(h);return 0;}
