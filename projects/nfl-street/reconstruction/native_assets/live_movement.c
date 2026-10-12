#include "live_movement.h"
#include <math.h>
#include <string.h>
static uint32_t be32(const uint8_t *p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static int16_t be16(const uint8_t *p){return (int16_t)((unsigned)p[0]<<8|p[1]);}
static float scalar(uint32_t w){float v;memcpy(&v,&w,4);return v;}
int nflstreet_live_movement_open(const uint8_t *b,size_t n,nflstreet_live_movement *out){
    if(!b||!out||n<152||(memcmp(b,"NMS1",4)&&memcmp(b,"NBS1",4)))return 0;
    unsigned count=be32(b+4);if(!count||count>1000||n!=152+(size_t)count*(!memcmp(b,"NBS1",4)?132:164))return 0;
    uint32_t actor[12],state[21];for(unsigned i=0;i<12;++i)actor[i]=be32(b+20+i*4);for(unsigned i=0;i<21;++i)state[i]=be32(b+68+i*4);
    if(!isfinite(scalar(state[0]))||!isfinite(scalar(state[1])))return 0;
    nflstreet_live_movement m={0};nflstreet_actor_runtime_init(&m.seed,be32(b+8),be16(b+12),be16(b+14),be32(b+16),actor,state);
    m.origin[0]=scalar(state[0]);m.origin[1]=scalar(state[1]);m.runtime=m.seed;*out=m;return 1;
}
void nflstreet_live_movement_reset(nflstreet_live_movement *m){if(m)m->runtime=m->seed;}
void nflstreet_live_movement_observe(const nflstreet_live_movement *m,nflstreet_live_motion *out){
    out->x=scalar(m->runtime.state[0])-m->origin[0];out->y=scalar(m->runtime.state[1])-m->origin[1];
    /* Command result heading from measured handoff; renderer orientation is
       chosen to align model -Z with field +Y at the original quarter turn. */
    out->facing=(float)((m->runtime.state[6]&0xffffff)*(6.283185307179586/16777216.0)-1.5707963267948966);
    out->speed_per_update=scalar(m->runtime.state[7]);
}
int nflstreet_live_movement_step(nflstreet_live_movement *m,int dx,int dy,nflstreet_live_motion *out){
    if(!m||!out)return -1;
    if(!dx&&!dy){if(!nflstreet_actor_runtime_brake(&m->runtime))return -1;
        nflstreet_live_movement_observe(m,out);return out->speed_per_update>0?1:0;}
    if(dx<-1||dx>1||dy<-1||dy>1)return -1;
    uint8_t vector[32]={0};float scale=dx&&dy?0.7071067811865475244f:1.0f;
    float vx=dx*scale,vy=dy*scale;uint32_t xb,yb;memcpy(&xb,&vx,4);memcpy(&yb,&vy,4);
    for(unsigned i=0;i<4;++i){vector[4+i]=(uint8_t)(xb>>(24-i*8));vector[8+i]=(uint8_t)(yb>>(24-i*8));}
    if(!nflstreet_actor_runtime_step_vector(&m->runtime,vector))return -1;
    nflstreet_live_movement_observe(m,out);return 1;
}
int nflstreet_live_movement_check(const uint8_t *b,size_t n,unsigned *updates){
    nflstreet_live_movement m;if(!b||n<152||!updates||memcmp(b,"NMS1",4)||!nflstreet_live_movement_open(b,n,&m))return 0;
    unsigned count=be32(b+4);for(unsigned i=0;i<count;++i){const uint8_t *r=b+152+(size_t)i*164;
        /* Exercise the same semantic input bridge used by the window. */
        uint32_t x=be32(r+4),y=be32(r+8);int dx=x==0x3f800000?1:x==0xbf800000?-1:0;int dy=y==0x3f800000?1:0;
        uint8_t expected[32]={0};if(dx){expected[4]=(uint8_t)(dx>0?0x3f:0xbf);expected[5]=0x80;}else if(dy){expected[8]=0x3f;expected[9]=0x80;}
        if(memcmp(expected,r,32))return 0;
        nflstreet_live_motion observed;if(nflstreet_live_movement_step(&m,dx,dy,&observed)!=1)return 0;
        for(unsigned j=0;j<12;++j)if(m.runtime.actor[j]!=be32(r+32+j*4))return 0;
        for(unsigned j=0;j<21;++j)if(m.runtime.state[j]!=be32(r+80+j*4))return 0;
    }
    /* Unsupported requests preserve state; neutral now advances braking. */
    nflstreet_actor_runtime before=m.runtime;nflstreet_live_motion observed;
    if(nflstreet_live_movement_step(&m,2,1,&observed)!=-1||memcmp(&before,&m.runtime,sizeof before)||
       nflstreet_live_movement_step(&m,0,-2,&observed)!=-1||memcmp(&before,&m.runtime,sizeof before))return 0;
    nflstreet_live_movement_reset(&m);if(memcmp(&m.seed,&m.runtime,sizeof m.seed))return 0;
    *updates=count;return 1;
}

int nflstreet_live_braking_check(const uint8_t *b,size_t n,unsigned *updates){
    nflstreet_live_movement m;if(!b||n<152||memcmp(b,"NBS1",4)||!updates||!nflstreet_live_movement_open(b,n,&m))return 0;
    unsigned count=be32(b+4);for(unsigned i=0;i<count;++i){const uint8_t *r=b+152+(size_t)i*132;nflstreet_live_motion observed;
        if(nflstreet_live_movement_step(&m,0,0,&observed)<0)return 0;
        for(unsigned j=0;j<12;++j)if(m.runtime.actor[j]!=be32(r+j*4))return 0;
        for(unsigned j=0;j<21;++j)if(m.runtime.state[j]!=be32(r+48+j*4))return 0;
    }
    if(m.runtime.state[7]||m.runtime.state[10]||m.runtime.state[11])return 0;
    *updates=count;return 1;
}
