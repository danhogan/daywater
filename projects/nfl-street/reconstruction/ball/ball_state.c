#include "ball_state.h"
#include <math.h>
#include <string.h>
static uint32_t be(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
int nflstreet_ball_observe(const uint8_t *pool,size_t n,nflstreet_ball_state*out){
 if(!pool||!out||n<8||!pool[0]||pool[1]>=pool[0]||n<8+(size_t)pool[0]*0xf4)return 0;
 const uint8_t *r=pool+8+(size_t)pool[1]*0xf4;nflstreet_ball_state s={0};s.active_index=pool[1];
 for(unsigned i=0;i<3;++i){s.position_words[i]=be(r+12+i*4);float f;memcpy(&f,s.position_words+i,4);if(!isfinite(f))return 0;}
 s.associated_actor_handle=be(r+0xb4);s.flags=be(r+8);s.opaque_state=be(r+0xc4);*out=s;return 1;
}
int nflstreet_ball_find_actor(const uint8_t*pool,size_t n,uint32_t actor,unsigned*found,unsigned*index){
 if(!pool||!found||!index||n<8||(actor&255)!=1||n<8+(size_t)pool[0]*0xf4)return 0;
 unsigned match=0,selected=0;for(unsigned i=0;i<pool[0];++i){uint32_t handle=be(pool+8+(size_t)i*0xf4+0xb4);
  if((handle&0xffffff)==(actor&0xffffff)){match=1;selected=i;break;}}
 *found=match;*index=selected;return 1;
}
int nflstreet_ball_anchor(const float current[3],const float first[3],const float second[3],float out[3]){
 if(!current||!first||!second||!out)return 0;
 for(unsigned i=0;i<3;++i)if(!isfinite(current[i])||!isfinite(first[i])||!isfinite(second[i]))return 0;
 float selected[3];memcpy(selected,current,sizeof selected);
 if(selected[1]<first[1])memcpy(selected,first,sizeof selected);
 if(selected[1]<second[1])memcpy(selected,second,sizeof selected);
 /* DOL 0x80251FB4: 0x3dc730f6, single-precision addition. */
 selected[1]+=0.09726135432720184f;
 if(!isfinite(selected[1]))return 0;memcpy(out,selected,sizeof selected);return 1;
}

int nflstreet_ball_sample_plane(const nflstreet_ball_trajectory *in,double plane,float *time,float xy[2]){
 if(!in||!xy||!isfinite(plane))return 0;
 for(unsigned i=0;i<3;++i)if(!isfinite(in->position[i])||!isfinite(in->velocity[i]))return 0;
 for(unsigned i=0;i<2;++i)if(!isfinite(in->acceleration_xy[i]))return 0;
 float squared=in->velocity[2]*in->velocity[2];
 float delta=(float)((double)in->position[2]-plane);
 float discriminant=fmaf(delta,0.005962963216006756f,squared)*112495.6484375f;
 if(!isfinite(discriminant))return 0;if(discriminant<0)discriminant=0;
 float t=fmaf(in->velocity[2],335.4037170410156f,sqrtf(discriminant));
 if(!isfinite(t))return 0;
 float selected[2];if(t<=0){t=0;memcpy(selected,in->position,sizeof selected);}
 else {float half=t*0.5f,scale=t*half;
  for(unsigned i=0;i<2;++i){float term=in->acceleration_xy[i]*scale;
   selected[i]=fmaf(in->velocity[i],t,term)+in->position[i];if(!isfinite(selected[i]))return 0;}}
 if(time)*time=t;memcpy(xy,selected,sizeof selected);return 1;
}

int nflstreet_ball_read_trajectory(const uint8_t*r,size_t n,nflstreet_ball_trajectory*out){
 if(!r||!out||n<0xf4)return 0;
 nflstreet_ball_trajectory t;
 for(unsigned i=0;i<8;++i){unsigned offset=i<3?12+i*4:i<6?0x54+(i-3)*4:0x70+(i-6)*4;
  uint32_t word=be(r+offset);float f;memcpy(&f,&word,4);if(!isfinite(f))return 0;
  if(i<3)t.position[i]=f;else if(i<6)t.velocity[i-3]=f;else t.acceleration_xy[i-6]=f;
 }
 *out=t;return 1;
}
