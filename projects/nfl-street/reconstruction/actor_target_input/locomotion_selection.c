#include "locomotion_selection.h"
#include <math.h>
#include <string.h>
static unsigned u16(const uint8_t*p){return (unsigned)p[0]<<8|p[1];}
static uint32_t u32(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
int nflstreet_locomotion_select(const nflstreet_asset_store *store,const uint8_t *rows,
                               size_t n,const uint8_t keys[4],nflstreet_locomotion_choice *out){
 if(!rows||n<4||!keys||!out)return 0;unsigned count=u16(rows);
 if(count>64||n<4+(size_t)count*8)return 0;
 nflstreet_locomotion_choice choice={-1,0,65535,65535,0};int best=-1;
 for(unsigned i=0;i<count;++i){const uint8_t*r=rows+4+i*8;uint32_t id=u32(r+4);
  const uint8_t*metadata=nflstreet_asset_read(store,id,8);if(!metadata)return 0;
  unsigned score=0;for(unsigned j=0;j<4;++j)if(keys[j]==metadata[4+j])score+=10-j;
  if((int)score>best){best=(int)score;choice.row=(int32_t)i;choice.score=(uint16_t)score;
   choice.bank_key=(uint16_t)u16(r);choice.clip_key=(uint16_t)u16(r+2);choice.metadata_identity=id;}
 }
 *out=choice;return 1;
}

static void p32(uint8_t*p,uint32_t v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static float scalar(const uint8_t*p){uint32_t v=u32(p);float f;memcpy(&f,&v,4);return f;}
int nflstreet_locomotion_resolve(const nflstreet_asset_store *store,uint16_t context,
                                const uint8_t keys[2],uint8_t data[64]){
 if(!keys||!data)return 0;uint8_t rows[520],variant[4]={keys[0],255,255,255};nflstreet_locomotion_choice choice;
 if(!nflstreet_asset_group_rows(store,context,0x4a,rows)||
    !nflstreet_locomotion_select(store,rows,sizeof rows,variant,&choice)||choice.row<0)return 0;
 if(!nflstreet_asset_group_rows(store,context,choice.clip_key&0x7fff,rows))return 0;
 variant[0]=keys[1];if(!nflstreet_locomotion_select(store,rows,sizeof rows,variant,&choice)||choice.row<0)return 0;
 if(!nflstreet_asset_group_rows(store,context,choice.clip_key&0x7fff,rows))return 0;
 uint8_t next[64];memcpy(next,data,64);next[0x38]=next[0x39]=0;p32(next+0x14,0);p32(next+0x18,0);int primary=0;
 for(unsigned i=0;i<u16(rows);++i){const uint8_t*r=rows+4+i*8;
  const uint8_t*m=nflstreet_asset_read(store,u32(r+4),8);if(!m)return 0;
  unsigned kind=m[4];if(kind<1||kind>3)continue;uint32_t identity;
  if(!nflstreet_asset_metadata(store,context,(uint16_t)(u16(r+2)&0x7fff),&identity)||!identity)return 0;
  unsigned at=kind==1?0x10:kind==2?0x18:0x14,bank=kind==1?0x37:kind==2?0x39:0x38;
  p32(next+at,identity);next[bank]=(uint8_t)u16(r);if(kind==1)primary=1;
 }
 if(!primary)return 0;
 if(!u32(next+0x14)){p32(next+0x14,u32(next+0x10));next[0x38]=next[0x37];}
 if(!u32(next+0x18)){p32(next+0x18,u32(next+0x10));next[0x39]=next[0x37];}
 p32(next+0xc,u32(next+0x10));next[0x36]=next[0x37];memcpy(data,next,64);return 1;
}
int nflstreet_locomotion_pair(const uint8_t*curve,size_t n,double speed,uint16_t*index,uint32_t clips[2]){
 if(!curve||n<8||!index||!clips||!isfinite(speed))return 0;
 unsigned count=u32(curve+4);if(count<2||count>128||n<0x18+(size_t)count*12)return 0;
 unsigned i=0;while(i<count-2){float threshold=scalar(curve+0x1c+(i+1)*12);
  if(!isfinite(threshold))return 0;if(fabsf(threshold)<0.000001f)threshold=0;
  if((double)threshold>speed)break;++i;
 }
 uint32_t first=u32(curve+0x20+i*12),second=u32(curve+0x20+(i+1)*12);
 *index=(uint16_t)i;clips[0]=first;clips[1]=second;return 1;
}

static void putscalar(uint8_t*p,float f){uint32_t v;memcpy(&v,&f,4);p32(p,v);}
int nflstreet_locomotion_blend(uint8_t*curve,size_t n,uint16_t index,double speed,
                              const uint8_t descriptor[8],uint8_t*channels,size_t size,float*out){
 if(!curve||n<8||!descriptor||!channels||!out||!isfinite(speed)||size>256*124)return 0;
 unsigned rows=u32(curve+4);if(rows<2||rows>128||index>=rows-1||n<0x18+(size_t)rows*12)return 0;
 unsigned a=descriptor[0],b=descriptor[1];
 if(a==255||b==255||((size_t)a+1)*124>size||((size_t)b+1)*124>size)return 0;
 float lo=scalar(curve+0x1c+index*12),hi=scalar(curve+0x28+index*12);
 float first=fabsf(scalar(curve+0x18+index*12)),second=fabsf(scalar(curve+0x24+index*12));
 float scale=scalar(descriptor+4),old_a=scalar(channels+a*124+0x30),old_b=scalar(channels+b*124+0x30);
 if(!isfinite(lo)||!isfinite(hi)||hi==lo||!isfinite(first)||!isfinite(second)||
    !isfinite(scale)||!isfinite(old_a)||!isfinite(old_b))return 0;
 float mix=(float)(speed-(double)lo)/(hi-lo);if(mix<0)mix=0;if(mix>1)mix=1;
 float rate=fmaf(second-first,mix,first),wa=(1.0f-mix)*scale,wb=mix*scale;
 if(old_a+old_b==1.0f&&(fabsf(old_a-wa)>0.33000001311302185f||fabsf(old_b-wb)>0.33000001311302185f)){
  wa=fmaf(wa-old_a,0.2199999988079071f,old_a);wb=old_b+(old_a-wa);
 }
 if(!isfinite(rate)||!isfinite(wa)||!isfinite(wb))return 0;
 uint8_t next[256*124];memcpy(next,channels,size);
 for(unsigned j=0;j<4;++j){unsigned slot=descriptor[j];if(j>=2&&slot==255)continue;
  if(((size_t)slot+1)*124>size)return 0;uint8_t*s=next+slot*124+0x4c;
  unsigned tags=u16(s+0x1a);float period=scalar(s+4),time=scalar(s+8),remaining=scalar(s+12);
  if(tags==65535||!isfinite(period)||!isfinite(time)||!isfinite(remaining))return 0;
  float new_period=tags?(float)((double)rate*60.0f)/(float)tags:0;
  if(period==0){time=0;remaining=0;}
  else{time=(float)((double)time*new_period)/period;remaining=(float)((double)remaining*new_period)/period;}
  if(!isfinite(new_period)||!isfinite(time)||!isfinite(remaining))return 0;
  putscalar(s+4,new_period);putscalar(s+8,time);putscalar(s+12,remaining);
 }
 putscalar(next+a*124+0x30,wa);putscalar(next+a*124+0x34,wa);p32(next+a*124+0x38,0);
 putscalar(next+b*124+0x30,wb);putscalar(next+b*124+0x34,wb);p32(next+b*124+0x38,0);
 putscalar(curve+0x18+index*12,first);putscalar(curve+0x24+index*12,second);
 memcpy(channels,next,size);*out=rate;return 1;
}

int nflstreet_locomotion_fade(uint8_t data[64],uint8_t*channels,size_t size){
 if(!data||!channels||size>256*124)return 0;
 uint8_t d[64],c[256*124];memcpy(d,data,64);memcpy(c,channels,size);
 unsigned left=d[0x3b],duration=d[0x3a];
 if(left>127||duration>127||left>duration)return 0;
 if(!left){d[0x3a]=0;putscalar(d+4,1);memcpy(data,d,64);return 1;}
 --left;d[0x3b]=(uint8_t)left;float scale=((float)duration-(float)left)/(float)duration;
 putscalar(d+4,scale);float old=scalar(d+8);if(!isfinite(old)||old<0||old>1)return 0;
 for(unsigned j=2;j<4;++j){unsigned slot=d[j];if(slot==255)continue;if(((size_t)slot+1)*124>size)return 0;
  if(scale==1){c[slot*124+1]=0;d[j]=255;}
  else{float weight=(1.0f-scale)*(j==2?old:1.0f-old);
   putscalar(c+slot*124+0x30,weight);putscalar(c+slot*124+0x34,weight);p32(c+slot*124+0x38,0);}
 }
 memcpy(data,d,64);memcpy(channels,c,size);return 1;
}
int nflstreet_locomotion_pose_weights(const float*weights,const float*fractions,unsigned count,
                                     uint8_t slots[32],uint8_t next[32],float normalized[32],unsigned*out){
 if(!weights||!fractions||!slots||!next||!normalized||!out||count>16)return 0;
 uint8_t ids[32],which[32];float raw[32];unsigned n=0;
 for(unsigned i=0;i<count&&n<24;++i){float w=weights[i],fraction=fractions[i];
  if(!isfinite(w)||w<0||!isfinite(fraction)||fraction<0||fraction>1)return 0;
  float a=w*fraction,b=w*(1.0f-fraction);
  if(a!=0){ids[n]=(uint8_t)i;which[n]=1;raw[n++]=a;}
  if(b!=0&&n<24){ids[n]=(uint8_t)i;which[n]=0;raw[n++]=b;}
 }
 if(!n)return 0;float sum=raw[0];raw[0]=1;
 for(unsigned i=1;i<n;++i){sum+=raw[i];if(!isfinite(sum))return 0;raw[i]=sum==0?0:raw[i]/sum;}
 memcpy(slots,ids,n);memcpy(next,which,n);memcpy(normalized,raw,n*sizeof *raw);*out=n;return 1;
}

int nflstreet_locomotion_curve_select(const nflstreet_curve_inputs *in,
 const nflstreet_curve_source curves[3],const nflstreet_curve_decision *current,nflstreet_curve_decision *out){
 if(!in||!curves||!current||!out||!isfinite(in->speed)||!isfinite(in->comparison)||
    !isfinite(in->bound_a)||!isfinite(in->bound_b))return 0;
 for(unsigned i=0;i<3;++i)if(!curves[i].identity||!isfinite(curves[i].minimum)||
    !isfinite(curves[i].maximum)||curves[i].minimum>curves[i].maximum)return 0;
 nflstreet_curve_decision next=*current;next.changed=0;int selected=-1;unsigned duration=6;
 if(in->speed==0)selected=0;
 else if(in->comparison>=0){
  if(in->comparison!=0||in->status!=10){
   if(in->bound_b<=in->bound_a){if(in->speed>in->bound_b){selected=0;duration=8;}}
   else selected=0;
  }else if(in->speed>=curves[2].minimum&&in->speed<=curves[2].maximum)selected=2;
 }else if(in->speed>=curves[1].minimum&&in->speed<=curves[1].maximum)selected=1;
 if(selected>=0){next.duration=(uint8_t)duration;
  if(curves[selected].identity!=current->identity){
   if(current->identity==curves[2].identity)next.duration*=2;
   next.identity=curves[selected].identity;next.bank=curves[selected].bank;next.changed=1;
  }
 }
 *out=next;return 1;
}

int nflstreet_locomotion_direction(float speed,uint32_t first,uint32_t second,nflstreet_directional_speed*out){
 if(!out||!isfinite(speed))return 0;uint32_t distance=(first-second)&0xffffff;
 if(distance>0x1000000-distance)distance=0x1000000-distance;
 nflstreet_directional_speed result={distance,(double)speed,(double)speed};
 if(speed!=0&&distance>0x3c71c7)result.pair_speed=-result.pair_speed;
 if(distance>=0x56c16d)result.blend_speed=-result.blend_speed;
 *out=result;return 1;
}
