/* 800C0684, bounded no-wind/no-spin branch. The existing ball_flight
   record API is retained, with 801DE278's double delta rounded as Gekko's
   frC multiplier BEFORE its single fused operation. */
#include "../ball/ball_flight.h"
#include "../ball/ball_orientation.h"
#include "../ball/ball_state.h"
#include "gekko_float.h"
#include <math.h>
#include <string.h>
static uint32_t be(const uint8_t*p){return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static float get(const uint8_t*p){uint32_t w=be(p);float f;memcpy(&f,&w,4);return f;}
static float word(uint32_t w){float f;memcpy(&f,&w,4);return f;}
static void put(uint8_t*p,float f){uint32_t w;memcpy(&w,&f,4);p[0]=w>>24;p[1]=w>>16;p[2]=w>>8;p[3]=w;}
int nflstreet_ball_update_airborne(uint8_t*r,size_t n,double dt,double plane){
 if(!r||n<244||!isfinite(dt)||dt<0||dt>=28||!isfinite(plane)||plane!=(double)(float)plane||be(r+0xb4)||be(r+0xd0)||get(r+0xe0)!=0||!(get(r+0x9c)>0)||!isfinite(get(r+0x9c)))return 0;
 for(unsigned i=0;i<3;++i)if(be(r+0x88+4*i)||be(r+0x60+4*i))return 0;if(get(r+0x6c)!=1)return 0;
 uint8_t next[244];memcpy(next,r,244);float q[4],delta[4];for(unsigned i=0;i<4;++i)q[i]=get(r+0x18+4*i);
 for(unsigned i=0;i<3;++i){float p=get(r+12+4*i),v=get(r+0x54+4*i),force=get(r+0x7c+4*i);if(!isfinite(p)||!isfinite(v)||!isfinite(force))return 0;
  float a=nflstreet_gk_muls(force,word(0x3b5f4ed0));v=nflstreet_gk_adds(v,a);p=nflstreet_gk_fmadds(v,dt,p);if(!isfinite(a)||!isfinite(v)||!isfinite(p))return 0;
  put(next+0x70+4*i,a);put(next+0x54+4*i,v);put(next+12+4*i,p);}
 const float hi[3]={42.0f,75.0f,30.0f},lo[2]={-42.0f,-75.0f};
 for(unsigned i=0;i<2;++i){float p=get(next+12+4*i);if(fabsf(p)>hi[i])put(next+0x54+4*i,0);if(p<lo[i])p=lo[i];if(p>hi[i])p=hi[i];put(next+12+4*i,p);}
 if(get(next+20)>hi[2])put(next+20,hi[2]);if(!nflstreet_ball_no_spin(q,delta))return 0;
 for(unsigned i=0;i<4;++i){put(next+0x18+4*i,q[i]);put(next+0x60+4*i,delta[i]);}
 nflstreet_ball_trajectory trajectory;float time,xy[2];if(!nflstreet_ball_read_trajectory(next,244,&trajectory)||!nflstreet_ball_sample_plane(&trajectory,plane,&time,xy))return 0;
 put(next+0x44,time);put(next+0x48,xy[0]);put(next+0x4c,xy[1]);put(next+0x50,(float)plane);memcpy(r,next,244);return 1;}
