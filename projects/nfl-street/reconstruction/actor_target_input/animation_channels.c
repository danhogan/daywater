#include "animation_channels.h"
#include "command_timing.h"
#include <math.h>
#include <string.h>
static uint16_t u16(const uint8_t *p){return (uint16_t)((unsigned)p[0]<<8|p[1]);}
static uint32_t u32(const uint8_t *p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void p16(uint8_t *p,uint16_t v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static void p32(uint8_t *p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
static float real(const uint8_t *p){uint32_t v=u32(p);float f;memcpy(&f,&v,4);return f;}
static void pf(uint8_t *p,float f){uint32_t v;memcpy(&v,&f,4);p32(p,v);}
int nflstreet_animation_allocate(uint8_t *channels,size_t size,unsigned count,int32_t *index)
{
    unsigned i,state;int32_t selected=-1;float least=1.0f;
    if(!channels||!index||count>16||size<(size_t)count*124)return 0;
    for(state=0;state<2;++state)for(i=0;i<count;++i)
        if(channels[i*124+1]==state){*index=(int32_t)i;return 1;}
    for(i=0;i<count;++i){const uint8_t *c=channels+i*124;float weight=real(c+0x30);
        if(u16(c+8)&0x8000){
            if(!isfinite(weight))return 0;
            if(weight<=least){least=weight;selected=(int32_t)i;}}}
    if(selected>=0)channels[(unsigned)selected*124+1]=0;
    *index=selected;return 1;
}
int nflstreet_animation_manager_allocate(uint8_t manager[404],int32_t *index)
{
    uint8_t m[404];int high=3,i,j;
    if(!manager||!index)return 0;
    memcpy(m,manager,404);
    while(high>=0&&!u16(m+high*100+6))--high;
    for(i=0;i<high;){
        if(u16(m+i*100+6)){++i;continue;}
        for(j=i;j<high;++j){memmove(m+j*100+4,m+(j+1)*100+4,100);
            p16(m+(j+1)*100+6,0);pf(m+(j+1)*100+12,0);}
        --high;
    }
    for(i=0;i<4;++i)if(!u16(m+i*100+6)){
        memcpy(manager,m,404);*index=i;return 1;}
    return 0; /* Full manager invokes callbacks/eviction not reconstructed. */
}
int nflstreet_animation_blend(uint8_t blend[20],float current,float target,uint32_t duration)
{
    float initial;
    if(!blend||!isfinite(current)||!isfinite(target))return 0;
    initial=current==-1.0f?real(blend):current;
    if(!isfinite(initial))return 0;
    if(current!=-1.0f)pf(blend,current);
    if(target!=-1.0f)pf(blend+4,target);
    p32(blend+8,1);pf(blend+12,initial);p32(blend+16,duration);return 1;
}
int nflstreet_animation_start(uint8_t *channels,size_t size,const uint8_t header[16],
                             uint16_t bank_key,const uint8_t bank[32],const uint8_t *groups,
                             size_t groups_size,uint16_t clip,uint16_t group,uint16_t row_override,
                             uint32_t identity,const uint8_t *program,size_t program_size,
                             float weight,uint32_t *actor_flags,int32_t *index)
{
    uint8_t copy[16*124],state[64];uint8_t *c;unsigned count,i,done;int32_t slot;
    uint32_t flags,rows;uint16_t row=UINT16_MAX,commands[64];size_t emitted;
    if(!channels||!header||!bank||!groups||!program||!actor_flags||!index||
       bank_key>255||!isfinite(weight)||groups_size<4||program_size<6)return 0;
    count=u16(header+4);if(count>16||size<count*124u)return 0;
    rows=u32(groups);if(rows>512||groups_size<4+(size_t)rows*16)return 0;
    memcpy(copy,channels,count*124u);
    if(!nflstreet_animation_allocate(copy,count*124u,count,&slot))return 0;
    if(slot<0){*index=-1;return 1;}
    if(row_override!=UINT16_MAX||!(group&0x8000)){
        uint16_t key=row_override==UINT16_MAX?group:row_override;
        for(i=0;i<rows;++i)if(u16(groups+4+i*16)==key){row=(uint16_t)i;break;}
    }
    c=copy+(unsigned)slot*124;
    pf(c+0x30,weight);pf(c+0x34,weight);p32(c+0x38,0);
    p16(c+4,u16(bank+2));c[6]=(uint8_t)bank_key;p16(c+2,clip);
    p16(c+8,group);p16(c+10,row);p32(c+12,u32(header+8));
    p32(c+0x1c,0);p32(c+0x20,0x01000000);pf(c+0x2c,1);
    pf(c+0x48,0);pf(c+0x28,0);pf(c+0x24,0);
    memset(state,0,sizeof state);memcpy(state,c+0x4c,48);flags=*actor_flags;
    if(!nflstreet_stream_setup(state,identity,program,program_size,&flags,commands,&emitted,&done))return 0;
    memcpy(c+0x4c,state,48);c[7]=program[2];if(!done)c[1]=2;
    memcpy(channels,copy,count*124u);*actor_flags=flags;*index=slot;return 1;
}
int nflstreet_animation_group_prepare(uint8_t manager[404],uint16_t group,float target,
                                     const uint8_t *transitions,size_t size,int32_t *index)
{
    uint8_t m[404];unsigned i,count;int32_t slot;uint8_t *r;int active=-1;
    if(!manager||!transitions||!index||size<8||!isfinite(target)||group==UINT16_MAX)return 0;
    count=u16(transitions);if(count>64||size<8+(size_t)count*8)return 0;
    memcpy(m,manager,404);
    for(i=0;i<4;++i)if(u16(m+i*100+6)==3){active=(int)i;break;}
    if(active<0||u16(transitions+4)!=u16(m+(unsigned)active*100+4))return 0;
    for(i=0;i<4;++i){r=m+i*100;
        if(u16(r+6)&&(u16(r+4)==group||u16(r+4)==UINT16_MAX))return 0;
        if(u16(r+6)==3){if(real(r+12)!=0)return 0;p16(r+6,2);}}
    if(count){
        if(!nflstreet_animation_manager_allocate(m,&slot))return 0;
        r=m+(unsigned)slot*100;memset(r+4,0,100);p16(r+4,UINT16_MAX);p16(r+6,1);
        pf(r+0x50,0);pf(r+0x54,0);p32(r+0x58,0);
    }
    if(!nflstreet_animation_manager_allocate(m,&slot))return 0;
    r=m+(unsigned)slot*100;memset(r+4,0,100);p16(r+4,group);p16(r+6,3);
    if(!nflstreet_animation_blend(r+0x50,0,target,10))return 0;
    memcpy(manager,m,404);*index=slot;return 1;
}
