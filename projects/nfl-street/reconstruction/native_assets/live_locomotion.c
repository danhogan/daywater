#include "live_locomotion.h"
#include "pose.h"
#include "../actor_target_input/locomotion_selection.h"
#include "../actor_target_input/command_timing.h"
#include "../actor_target_input/animation_channels.h"
#include <math.h>
#include <string.h>
static unsigned u16(const uint8_t*p){return (unsigned)p[0]<<8|p[1];}
static uint32_t u32(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static int span(size_t n,size_t at,size_t size){return at<=n&&size<=n-at;}
static float real(const uint8_t*p){uint32_t w=u32(p);float f;memcpy(&f,&w,4);return f;}
static void scalar(uint8_t*p,float f){uint32_t w;memcpy(&w,&f,4);p[0]=w>>24;p[1]=w>>16;p[2]=w>>8;p[3]=w;}
void nflstreet_live_locomotion_reset(nflstreet_live_locomotion*m){
 if(!m)return;if(m->initial_profile&&(m->set_keys[0]!=m->initial_profile[6]||m->set_keys[1]!=m->initial_profile[7])){
  nflstreet_live_locomotion fresh;if(nflstreet_live_locomotion_open(&fresh,m->bank,m->bank_size,m->initial_profile,m->initial_profile_size))*m=fresh;return;}
 m->set_changes=m->pending_set=0;m->current_curve=m->curve_changes=m->direction_updates=m->negative_pair_updates=m->negative_blend_updates=0;m->curve_size=m->source_sizes[0];memcpy(m->curve,m->seed_curve,m->curve_size);memset(m->channels,0,sizeof m->channels);
 m->descriptor[0]=0;m->descriptor[1]=1;m->descriptor[2]=m->descriptor[3]=255;scalar(m->descriptor+4,1);
 scalar(m->channels+0x30,1);scalar(m->channels+0x34,1);
 m->initialized=m->changes=m->updates=m->flags=m->fade_updates=m->completed_fades=0;m->rate=0;
 memset(m->descriptor+8,0,sizeof m->descriptor-8);
}
int nflstreet_live_locomotion_open(nflstreet_live_locomotion*out,const uint8_t*bank,size_t n,const uint8_t*profile,size_t pn){
 if(!out||!bank||n<24||!profile||pn<20||(memcmp(profile,"NLC1",4)&&memcmp(profile,"NLC2",4))||profile[11]!=3)return 0;
 /* This verified candidate bank must match the exported registry key's
    measured bank type/tag/frame signature. Complete resident mapping is open. */
 if(profile[12]!=2||u16(bank)!=1||u16(bank+2)!=7||u16(bank+4)!=4641)return 0;
 nflstreet_live_locomotion m={0};m.bank=bank;m.bank_size=n;m.initial_profile=profile;m.initial_profile_size=pn;m.set_keys[0]=profile[6];m.set_keys[1]=profile[7];m.source_decisions=!memcmp(profile,"NLC2",4);
 size_t at=12;for(unsigned i=0;i<3;++i){size_t header=m.source_decisions?12:8;
  if(!span(pn,at,header)||profile[at]!=2)return 0;unsigned size=u32(profile+at+4);
  uint16_t index;uint32_t clips[2];if(size>1600||!span(pn,at+header,size)||
   !nflstreet_locomotion_pair(profile+at+header,size,0,&index,clips))return 0;
  m.source_sizes[i]=size;memcpy(m.source_curves[i],profile+at+header,size);
  m.source[i]=(nflstreet_curve_source){m.source_decisions?u32(profile+at+8):i+1,profile[at],real(profile+at+header+12),real(profile+at+header+16)};
  if(m.source_decisions&&!m.source[i].identity)return 0;at+=header+size;
 }if(at!=pn)return 0;
 for(unsigned i=0;i<3;++i)for(unsigned j=0;j<i;++j)if(m.source[i].identity==m.source[j].identity&&
  (m.source[i].bank!=m.source[j].bank||m.source_sizes[i]!=m.source_sizes[j]||memcmp(m.source_curves[i],m.source_curves[j],m.source_sizes[i])))return 0;
 m.curve_size=m.source_sizes[0];memcpy(m.seed_curve,m.source_curves[0],m.curve_size);
 m.transition_duration=8;nflstreet_live_locomotion_reset(&m);*out=m;return 1;
}
static int setup(nflstreet_live_locomotion*m,unsigned slot,unsigned clip){
 size_t table=u32(m->bank+12);uint32_t id;
 if(!span(m->bank_size,table,4)||clip>=u32(m->bank+table)||
    !nflstreet_stream_program_lookup(m->bank+table,m->bank_size-table,clip,&id)||!span(m->bank_size,id,6))return 0;
 size_t at=id+4;unsigned tags=0;for(;span(m->bank_size,at,2)&&tags<1024;at+=2){unsigned w=u16(m->bank+at);
  if(w==0x8001)break;if(w&0x8000||w>=u16(m->bank+4))return 0;++tags;}
 if(!span(m->bank_size,at,2)||u16(m->bank+at)!=0x8001||tags<2)return 0;
 uint8_t*s=m->channels+slot*124+0x4c;uint16_t commands[64];size_t emitted;unsigned complete;
 if(m->initialized){if(!nflstreet_stream_rebind_phase(s,id,m->bank+id,at+2-id,(uint16_t)tags,&m->flags))return 0;}
 else if(!nflstreet_stream_setup(s,id,m->bank+id,at+2-id,&m->flags,commands,&emitted,&complete)||complete||emitted)return 0;
 /* Loop-only source programs: the original tag-count scanner reaches the
    same first terminal word; no callbacks or arguments are bypassed. */
 s[0x1a]=tags>>8;s[0x1b]=tags;m->channels[slot*124+1]=2;m->program[slot]=m->bank+id;m->program_size[slot]=at+2-id;return 1;
}
static int step(nflstreet_live_locomotion*m,double speed,double blend_speed,unsigned directional,const nflstreet_curve_inputs*input,double delta,int16_t*out,size_t capacity,unsigned*count){
 if(!m||!out||!count||capacity<384||!isfinite(speed)||!isfinite(blend_speed)||!isfinite(delta)||delta<0||delta>65536)return 0;
 nflstreet_live_locomotion next=*m;uint16_t index;uint32_t clips[2];unsigned curve_changed=0,duration=next.transition_duration;
 if(input&&next.source_decisions&&next.initialized&&!next.descriptor[0x3b]){
  nflstreet_curve_decision before={next.source[next.current_curve].identity,next.source[next.current_curve].bank,8,0},decision;
  if(!nflstreet_locomotion_curve_select(input,next.source,&before,&decision))return 0;
  duration=decision.duration;curve_changed=decision.changed;
  if(curve_changed){unsigned selected=3;for(unsigned i=0;i<3;++i)if(next.source[i].identity==decision.identity){selected=i;break;}
   if(selected==3)return 0;next.current_curve=selected;next.curve_size=next.source_sizes[selected];
   memcpy(next.curve,next.source_curves[selected],next.curve_size);++next.curve_changes;
  }
 }

 if(directional){next.descriptor[0x28]=next.descriptor[0x29]=255;scalar(next.descriptor+0x2c,input->speed);}
 if(!nflstreet_locomotion_pair(next.curve,next.curve_size,speed,&index,clips))return 0;
 if(duration>127)return 0;
 if(!next.initialized||(!next.descriptor[0x3b]&&(index!=next.pair||curve_changed||next.pending_set))){
  if(next.initialized&&duration){unsigned old[2]={next.descriptor[0],next.descriptor[1]};
   float old_weight=real(next.channels+old[0]*124+0x30);scalar(next.descriptor+8,old_weight);
   for(unsigned i=0;i<2;++i){int32_t slot;if(!nflstreet_animation_allocate(next.channels,sizeof next.channels,4,&slot)||slot<0)return 0;
    memcpy(next.channels+(unsigned)slot*124,next.channels+old[i]*124,124);next.descriptor[i]=(uint8_t)slot;next.descriptor[i+2]=(uint8_t)old[i];
    scalar(next.channels+(unsigned)slot*124+0x30,0);scalar(next.channels+(unsigned)slot*124+0x34,0);
    if(!nflstreet_animation_blend(next.channels+(unsigned)slot*124+0x30,-1,1,duration))return 0;
   }
   next.descriptor[0x3a]=next.descriptor[0x3b]=(uint8_t)duration;
  }
  for(unsigned i=0;i<2;++i)if(!setup(&next,next.descriptor[i],clips[i]))return 0;
  next.pending_set=0;next.pair=index;next.descriptor[0x34]=index>>8;next.descriptor[0x35]=index;memcpy(next.clips,clips,sizeof clips);next.initialized=1;++next.changes;}
 unsigned fading=next.descriptor[0x3b];if(!nflstreet_locomotion_fade(next.descriptor,next.channels,sizeof next.channels))return 0;
 if(fading){++next.fade_updates;if(!next.descriptor[0x3b])++next.completed_fades;}
 if(!nflstreet_locomotion_blend(next.curve,next.curve_size,next.pair,blend_speed,next.descriptor,next.channels,sizeof next.channels,&next.rate)||next.rate<=0)return 0;
 if(directional)scalar(next.descriptor+0x30,next.rate);
 float weights[4]={0},fractions[4]={0};unsigned components=0;
 for(unsigned slot=0;slot<4;++slot){if(!next.channels[slot*124+1])continue;uint8_t*s=next.channels+slot*124+0x4c;uint16_t commands[64];size_t emitted;unsigned complete;
  if(!nflstreet_command_timing(s,next.program[slot],next.program_size[slot],delta,&next.flags,commands,&emitted,&complete)||emitted||complete)return 0;
  weights[slot]=real(next.channels+slot*124+0x30);fractions[slot]=real(s+0x14);
 }
 uint8_t slots[32],which[32];float normalized[32];unsigned entries;
 if(!nflstreet_locomotion_pose_weights(weights,fractions,4,slots,which,normalized,&entries))return 0;
 int16_t pose[384];for(unsigned i=0;i<entries;++i){const uint8_t*s=next.channels+slots[i]*124+0x4c;int16_t decoded[384];unsigned n;
  if(!nflstreet_pose_frame(next.bank,next.bank_size,u16(s+(which[i]?0x12:0x10)),decoded,384,&n)||(i&&n!=components))return 0;
  if(!i)memcpy(pose,decoded,n*sizeof *pose);else if(!nflstreet_pose_mix(pose,pose,decoded,n,(unsigned)(normalized[i]*4096)))return 0;
  components=n;
 }
 ++next.updates;memcpy(out,pose,components*sizeof *out);*count=components;*m=next;return 1;
}

int nflstreet_live_locomotion_step(nflstreet_live_locomotion*m,double speed,double delta,int16_t*out,size_t n,unsigned*count){
 return step(m,speed,speed,0,NULL,delta,out,n,count);
}
int nflstreet_live_locomotion_step_actor(nflstreet_live_locomotion*m,const nflstreet_curve_inputs*in,double delta,int16_t*out,size_t n,unsigned*count){
 if(!in)return 0;return step(m,in->speed,in->speed,0,in,delta,out,n,count);
}

int nflstreet_live_locomotion_step_direction(nflstreet_live_locomotion*m,const nflstreet_curve_inputs*in,uint32_t first,uint32_t second,double delta,int16_t*out,size_t n,unsigned*count){
 if(!m||!in)return 0;if(!m->source_decisions)return nflstreet_live_locomotion_step_actor(m,in,delta,out,n,count);
 nflstreet_directional_speed speeds;if(!nflstreet_locomotion_direction(in->speed,first,second,&speeds))return 0;
 if(!step(m,speeds.pair_speed,speeds.blend_speed,1,in,delta,out,n,count))return 0;
 ++m->direction_updates;if(speeds.pair_speed<0)++m->negative_pair_updates;if(speeds.blend_speed<0)++m->negative_blend_updates;return 1;
}

int nflstreet_live_locomotion_catalog_get(const uint8_t*data,size_t n,unsigned index,const uint8_t**profile,size_t*size){
 if(!data||!profile||!size||n<8||memcmp(data,"NSC1",4))return 0;unsigned entries=u32(data+4);
 if(!entries||entries>32||index>=entries)return 0;size_t at=8,chosen=0,length=0;unsigned keys[32];
 for(unsigned i=0;i<entries;++i){if(!span(n,at,4))return 0;unsigned bytes=u32(data+at);at+=4;
  if(bytes<12||!span(n,at,bytes)||memcmp(data+at,"NLC2",4))return 0;
  keys[i]=(unsigned)data[at+6]<<8|data[at+7];for(unsigned j=0;j<i;++j)if(keys[i]==keys[j])return 0;
  if(i==index){chosen=at;length=bytes;}at+=bytes;
 }if(at!=n)return 0;*profile=data+chosen;*size=length;return 1;
}
int nflstreet_live_locomotion_step_set(nflstreet_live_locomotion*m,const uint8_t*profile,size_t bytes,
 const nflstreet_curve_inputs*input,uint32_t first,uint32_t second,double delta,int16_t*out,size_t n,unsigned*count){
 if(!m||!input)return 0;nflstreet_live_locomotion selected,next=*m;
 if(!nflstreet_live_locomotion_open(&selected,m->bank,m->bank_size,profile,bytes)||!selected.source_decisions)return 0;
 if(!next.descriptor[0x3b]&&(memcmp(next.set_keys,selected.set_keys,2))){
  memcpy(next.source_curves,selected.source_curves,sizeof next.source_curves);memcpy(next.source_sizes,selected.source_sizes,sizeof next.source_sizes);memcpy(next.source,selected.source,sizeof next.source);
  next.curve_size=selected.curve_size;memcpy(next.curve,selected.curve,next.curve_size);next.current_curve=0;
  memcpy(next.set_keys,selected.set_keys,2);next.pending_set=1;++next.set_changes;
 }
 next.descriptor[0x3c]=next.set_keys[0];next.descriptor[0x3d]=next.set_keys[1];
 if(!nflstreet_live_locomotion_step_direction(&next,input,first,second,delta,out,n,count))return 0;
 *m=next;return 1;
}
