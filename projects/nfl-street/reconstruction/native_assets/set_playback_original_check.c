/* Controlled source-set installation/playback oracle; not full gameplay. */
#include <windows.h>
#include "moderngekko/module_abi.h"
#include "live_locomotion.h"
#include "../actor_target_input/locomotion_selection.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
static unsigned u32(const uint8_t*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static void put(uint8_t*p,unsigned v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static float real(const uint8_t*p){unsigned w=u32(p);float f;memcpy(&f,&w,4);return f;}
static void scalar(uint8_t*p,float f){unsigned w;memcpy(&w,&f,4);put(p,w);}
static uint8_t*read(const char*path,size_t*n){FILE*f=fopen(path,"rb");if(!f)return NULL;fseek(f,0,SEEK_END);long size=ftell(f);rewind(f);if(size<0){fclose(f);return NULL;}uint8_t*b=malloc(size);if(!b||fread(b,1,size,f)!=(size_t)size){free(b);b=NULL;}fclose(f);*n=size;return b;}
static int call(const ModernGekkoModuleDesc*m,CPUState*c,unsigned pc){c->pc=pc;c->lr=0;c->downcount=c->cycle_budget=100000000;c->exception=0;for(unsigned i=0;c->pc&&i<1000000;++i){m->dispatch(c,c->pc);if(c->exception)break;}if(c->pc||c->exception){fprintf(stderr,"Original call %08x failed pc=%08x ex=%08x\n",pc,c->pc,c->exception);return 0;}return 1;}
static int blend_tail(const ModernGekkoModuleDesc*m,CPUState*c){
 c->pc=0x80063dac;c->lr=0;c->downcount=c->cycle_budget=100000000;c->exception=0;
 for(unsigned i=0;c->pc!=0x800627b8&&i<10000;++i){m->dispatch(c,c->pc);if(c->exception||!c->pc)return 0;}
 return c->pc==0x800627b8&&!c->exception;
}
int main(int argc,char**argv){if(argc!=5)return 2;HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;const ModernGekkoModuleDesc*m=get();if(m->cpu_state_size!=sizeof(CPUState))return 4;
 size_t rn,bn,pn;uint8_t*ram=read(argv[2],&rn),*bank=read(argv[3],&bn),*profile=read(argv[4],&pn);if(!ram||rn!=24*1024*1024||!bank||bn>1024*1024||!profile)return 5;
 const uint8_t*initial_profile;size_t initial_size;
 if(!nflstreet_live_locomotion_catalog_get(profile,pn,0,&initial_profile,&initial_size))return 6;
 /* Reject every truncated catalog without altering output arguments. */
 for(size_t truncated=0;truncated<pn;++truncated){const uint8_t*sentinel=profile;size_t bytes=123;
  if(nflstreet_live_locomotion_catalog_get(profile,truncated,0,&sentinel,&bytes)||sentinel!=profile||bytes!=123)return 34;
 }
 uint8_t*duplicate=malloc(pn);if(!duplicate)return 5;memcpy(duplicate,profile,pn);
 size_t second=12+u32(profile+8)+4;if(second+8>pn)return 34;duplicate[second+6]=initial_profile[6];duplicate[second+7]=initial_profile[7];
 const uint8_t*sentinel=profile;size_t bytes=123;
 if(nflstreet_live_locomotion_catalog_get(duplicate,pn,0,&sentinel,&bytes)||sentinel!=profile||bytes!=123)return 34;free(duplicate);
 nflstreet_live_locomotion native;if(!nflstreet_live_locomotion_open(&native,bank,bn,initial_profile,initial_size))return 6;
 memcpy(ram+0x1000000,bank,bn);unsigned table=u32(bank+12),programs=u32(bank+table),registry=u32(ram+0x3a4ad4);
 if(table+4+(size_t)programs*4>bn||registry<0x80000000||registry-0x80000000+3*28>rn)return 7;
 put(ram+0x1000000+12,0x81000000+table);put(ram+0x1000000+16,0x81000000+u32(bank+16));
 for(unsigned i=0;i<programs;++i)put(ram+0x1000000+table+4+i*4,0x81000000+u32(bank+table+4+i*4));
 unsigned original_bank=u32(ram+(registry-0x80000000)+2*28+12);put(ram+(registry-0x80000000)+2*28+12,0x81000000);
 memcpy(ram+0x17f4000,native.descriptor,64);memcpy(ram+0x17f5000,native.channels,496);
 memset(ram+0x17f2000,0,16);ram[0x17f2005]=4;
 memset(ram+0x17f7000,0,404);ram[0x17f7005]=0x4a;ram[0x17f7007]=3;scalar(ram+0x17f7050,1);
 uint8_t*initial=ram+0x17f4000;put(initial+12,native.source[0].identity);initial[0x36]=native.source[0].bank;initial[0x28]=initial[0x29]=255;initial[0x3c]=initial_profile[6];initial[0x3d]=initial_profile[7];
 for(unsigned i=0;i<3;++i){put(initial+16+4*i,native.source[i].identity);initial[0x37+i]=native.source[i].bank;}
 CPUState c={0};c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;c.gpr[1]=0x81700000;c.ram=ram;c.ram_size=rn;c.msr=0x2000;c.hid2=0xe0000000;c.gqr[6]=0x0f070007;
 unsigned angles[]={0,1,0x3c71c6,0x3c71c7,0x3c71c8,0x56c16c,0x56c16d,0x56c16e,0x800000,0xffffff};unsigned installs=0,deferred=0;
 unsigned pair=65535,starts=0,fade_updates=0,completions=0,packed=0,held=0;
 for(unsigned step=0;step<480;++step){double speed=step<120?-0.2+(double)step*0.003:step<240?0.3-(double)(step-120)*0.004:step<360?0:step&1?-0.15:0.2;if(step%120>=40&&step%120<80)speed=0.1;double delta=step%7==0?3.25:step%7==1?0:0.5;
  nflstreet_curve_inputs input={(float)speed,step%120<80?0:-0.1f,0.1f,0.08f,step%120>=40&&step%120<80?10:0};speed=input.speed;
  unsigned first=0x00fffff0,second=first-angles[(step/12)%10];
  put(ram+0x17f61c0,first);put(ram+0x17f61c8,second);scalar(ram+0x17f61c4,input.speed);
  const uint8_t*requested;size_t requested_size;unsigned requested_index=step>=200&&step<240?(step/2)%2:(step/40)%2;
  if(!nflstreet_live_locomotion_catalog_get(profile,pn,requested_index,&requested,&requested_size))return 8;
  int16_t pose[384];unsigned components;if(!nflstreet_live_locomotion_step_set(&native,requested,requested_size,&input,first,second,delta,pose,384,&components))return 8;
  uint8_t*d=ram+0x17f4000;unsigned choice_changed=0,duration=8,set_changed=0;
  if(d[0x3c]!=requested[6]||d[0x3d]!=requested[7]){
   if(d[0x3b])++deferred;
   else {put(ram+(registry-0x80000000)+2*28+12,original_bank);
    c.gpr[3]=1;c.gpr[4]=0x4a;c.gpr[5]=0x817f1000;if(!call(m,&c,0x80175af8))return 31;
    d[0x3c]=requested[6];d[0x3d]=requested[7];c.gpr[3]=0x817f1000;c.gpr[4]=0x817f4000;c.gpr[5]=0x803c19c0;c.gpr[6]=0;
    if(!call(m,&c,0x80062594))return 31;put(ram+(registry-0x80000000)+2*28+12,0x81000000);set_changed=1;++installs;
   }
  }
  if(pair!=65535&&!d[0x3b]){scalar(ram+0x17f61c4,input.speed);scalar(ram+0x17f61dc,input.comparison);
   scalar(ram+0x17f61e8,input.bound_a);scalar(ram+0x17f61ec,input.bound_b);ram[0x17f621e]=input.status;ram[0x17f8000]=8;
   c.gpr[3]=0x817f6000;c.gpr[4]=0x817f4000;c.gpr[5]=0x817f8000;if(!call(m,&c,0x80062624))return 25;
   choice_changed=c.gpr[3];duration=ram[0x17f8000];
  }
  unsigned current=u32(d+12);c.gpr[3]=0x817f6000;c.gpr[4]=current;if(!call(m,&c,0x80062748))return 9;unsigned wanted=c.gpr[3];
  if(pair==65535){pair=wanted;for(unsigned slot=0;slot<2;++slot){unsigned clip=u32(ram+(current-0x80000000)+0x20+(pair+slot)*12),id=u32(bank+table+4+clip*4);uint8_t*channel=ram+0x17f5000+slot*124;
    channel[1]=2;channel[6]=2;channel[8]=0;channel[9]=0x4a;
    c.gpr[3]=0x817f5000+slot*124+0x4c;c.gpr[4]=0x81000000+id;c.gpr[5]=1;if(!call(m,&c,0x8017724c))return 10;
   }++starts;}
  else if(!d[0x3b]&&(wanted!=pair||choice_changed||set_changed)){
   c.gpr[3]=current;c.gpr[4]=0x817f4000;c.gpr[5]=0x803c19c0;c.gpr[6]=2;c.gpr[7]=wanted;c.gpr[8]=0x4a;c.gpr[9]=0x817f2000;c.gpr[10]=0x817f5000;put(ram+0x1700008,duration);
   if(!call(m,&c,0x8006475c))return 11;d[0x3a]=d[0x3b]=duration;pair=wanted;++starts;
  }else if(d[0x3b]&&wanted!=pair)++held;
  d[0x34]=pair>>8;d[0x35]=pair;
  if(d[0x3b]){--d[0x3b];c.gpr[3]=0x817f4000;c.fpr[1]=d[0x3a];c.fpr[2]=d[0x3b];if(!call(m,&c,0x80064af4))return 12;
   c.gpr[3]=0x817f4000;c.gpr[4]=0x817f2000;c.gpr[5]=0x817f5000;if(!call(m,&c,0x80064b04))return 13;++fade_updates;if(!d[0x3b])++completions;
  }else{d[0x3a]=0;scalar(d+4,1);}
  scalar(d+0x2c,input.speed);c.gpr[28]=0x817f6000;c.gpr[31]=0x817f4000;c.gpr[27]=0x817f5000;
  if(!blend_tail(m,&c)||!call(m,&c,0x800627b8))return 14;
  for(unsigned slot=0;slot<4;++slot)if(ram[0x17f5000+slot*124+1]){c.gpr[3]=0x817f5000+slot*124+0x4c;c.fpr[1]=delta;if(!call(m,&c,0x80177374))return 15;}
  uint8_t comparison[64];memcpy(comparison,d,64);memset(comparison+12,0,16);memset(comparison+0x36,0,4);
  if(current!=native.source[native.current_curve].identity||memcmp(comparison,native.descriptor,64)||memcmp(ram+(current-0x80000000),native.curve,native.curve_size)||pair!=native.pair){fprintf(stderr,"Descriptor mismatch step=%u\n",step);return 16;}
  for(unsigned slot=0;slot<4;++slot){const uint8_t*original=ram+0x17f5000+slot*124,*ours=native.channels+slot*124;
   if(original[1]!=ours[1])return 17;uint8_t copy[76];memcpy(copy,original+0x30,76);if(u32(copy+0x1c))put(copy+0x1c,u32(copy+0x1c)-0x81000000);
   if(memcmp(copy,ours+0x30,76)){fprintf(stderr,"Channel mismatch step=%u slot=%u\n",step,slot);for(unsigned i=0;i<76;++i)if(copy[i]!=ours[0x30+i]){fprintf(stderr,"byte=%02x native=%02x original=%02x\n",0x30+i,ours[0x30+i],copy[i]);break;}return 18;}
  }
  /* Exercise actual original list assembly with controlled group weight1
     and source metadata rows. Compare slot/frame order and each normalized
     weight, excluding original frame/decoder pointers from the numeric claim. */
  float w[4]={0},f[4]={0};for(unsigned slot=0;slot<4;++slot)if(native.channels[slot*124+1]){w[slot]=real(native.channels+slot*124+0x30);f[slot]=real(native.channels+slot*124+0x60);}
  uint8_t ids[32],which[32];float weights[32];unsigned entries;if(!nflstreet_locomotion_pose_weights(w,f,4,ids,which,weights,&entries))return 19;
  c.gpr[3]=0x817f2000;c.gpr[4]=0x817f5000;c.gpr[5]=0x817f7000;if(!call(m,&c,0x80176650))return 20;
  unsigned original_entries=(ram[0x32bf28]<<8)|ram[0x32bf29];if(entries!=original_entries){fprintf(stderr,"Entry count mismatch step=%u native=%u original=%u\n",step,entries,original_entries);return 21;}
  for(unsigned i=0;i<entries;++i){uint8_t*row=ram+0x32bf2c+i*44;unsigned slot=row[9];unsigned frame=(row[34]<<8)|row[35];const uint8_t*s=native.channels+ids[i]*124+0x4c;unsigned at=which[i]?0x12:0x10;
   if(slot!=ids[i]||frame!=(unsigned)(s[at]<<8|s[at+1])||real(row+4)!=weights[i]){fprintf(stderr,"Pose weight/order mismatch step=%u entry=%u slot=%u/%u weight=%.9g/%.9g\n",step,i,ids[i],slot,weights[i],real(row+4));return 22;}++packed;
  }
 }
 if(starts!=native.changes||fade_updates!=native.fade_updates||completions!=native.completed_fades||!completions||!native.curve_changes||installs!=native.set_changes||!installs||!deferred)return 23;
 uint8_t descriptor[64],channels[496],before[496];memcpy(descriptor,native.descriptor,64);memcpy(channels,native.channels,496);memcpy(before,channels,496);
 descriptor[0x3a]=descriptor[0x3b]=8;descriptor[2]=4;uint8_t saved[64];memcpy(saved,descriptor,64);
 if(nflstreet_locomotion_fade(descriptor,channels,496)||memcmp(saved,descriptor,64)||memcmp(before,channels,496))return 24;
 float w[4]={1,0,0,0},f[4]={NAN,0,0,0},normalized[32];uint8_t ids[32],which[32];memset(ids,0x5a,32);memset(which,0x5a,32);memset(normalized,0x5a,sizeof normalized);unsigned entries=123;
 if(nflstreet_locomotion_pose_weights(w,f,4,ids,which,normalized,&entries)||entries!=123)return 24;
 for(unsigned i=0;i<32;++i)if(ids[i]!=0x5a||which[i]!=0x5a)return 24;
 for(unsigned i=0;i<sizeof normalized;++i)if(((uint8_t*)normalized)[i]!=0x5a)return 24;
 printf("480 joined source-set/direction/curve/four-channel updates exact; %u starts, %u fade updates, %u completions, %u gated requests; %u original pose-list weights/order entries exact\n",starts,fade_updates,completions,held,packed);
 printf("%u source-set installs, %u deferred set requests; ",installs,deferred);
 printf("%u source-identity curve changes; original duration decisions joined to transitions\n",native.curve_changes);
 nflstreet_live_locomotion saved_native=native;int16_t output[384];memset(output,0x5a,sizeof output);unsigned components=91;
 const uint8_t*bad=profile;size_t bad_size=7;nflstreet_curve_inputs valid={0,0,1,0,0};
 if(nflstreet_live_locomotion_step_set(&native,bad,bad_size,&valid,0,0,1,output,384,&components)||memcmp(&native,&saved_native,sizeof native)||components!=91)return 32;
 for(unsigned i=0;i<sizeof output;++i)if(((uint8_t*)output)[i]!=0x5a)return 32;
 nflstreet_live_locomotion_reset(&native);if(native.initialized||native.set_changes||native.set_keys[0]!=initial_profile[6]||native.set_keys[1]!=initial_profile[7])return 33;
 puts("Invalid catalog/profile rollback and reset-to-initial-set checks pass");
 free(ram);free(bank);free(profile);FreeLibrary(h);return 0;
}
