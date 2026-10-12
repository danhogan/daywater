#include "animation_callbacks.h"
#include "asset_loader.h"
#include "command_timing.h"
#include "contact_selection.h"
#include "../kinematics/kinematics.h"
#include <math.h>
#include <string.h>
static uint16_t u16(const uint8_t *p){return (uint16_t)((unsigned)p[0]<<8|p[1]);}
static uint32_t u32(const uint8_t *p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void p16(uint8_t *p,uint16_t v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static void p32(uint8_t *p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
static float real(const uint8_t *p){uint32_t v=u32(p);float f;memcpy(&f,&v,4);return f;}
static void pf(uint8_t *p,float f){uint32_t v;memcpy(&v,&f,4);p32(p,v);}
static float mul(float x,float y){volatile float z=x*y;return z;}
static float add(float x,float y){volatile float z=x+y;return z;}
static int layout(const uint8_t *a){return u32(a+0x318)-u32(a+0x31c)==0x4e4&&
    u32(a+0x320)-u32(a+0x31c)==12&&u16(a+0x4dc)==10;}
static int active_group(const uint8_t *a){
    for(unsigned i=0;i<4;++i){
        if(u16(a+0x9bc+i*100+6)==3)return (int)u16(a+0x9bc+i*100+4);
    }
    return -1;
}
static uint8_t *group_record(uint8_t *a,uint16_t group){for(unsigned i=0;i<4;++i){uint8_t *r=a+0x9bc+i*100;
    if(u16(r+6)&&u16(r+4)==group)return r;}return NULL;}
int nflstreet_animation_transition_select(const nflstreet_animation_blob *assets,size_t count,
                                         uint16_t clip,uint16_t tag,uint16_t group,
                                         uint16_t target_clip,uint16_t bank,uint32_t *identity)
{
    uint32_t result=0;uint16_t destination=target_clip==65535?group:target_clip;
    uint16_t selector=target_clip==65535?65535:(uint16_t)(bank|0x8000);
    if(!identity||count>64||(!assets&&count))return 0;
    for(size_t i=0;i<count;++i){const nflstreet_animation_blob *b=assets+i;
        if(!b->data||b->size<4)return 0;
        uint32_t rows=u32(b->data);if(rows>256||b->size<4+(size_t)rows*16)return 0;
        for(uint32_t j=0;j<rows;++j){const uint8_t *r=b->data+4+j*16;
            if(u16(r)!=selector&&(selector!=65535||(u16(r)&0x8000)))continue;
            if(u16(r+2)!=destination||u16(r+4)!=clip)continue;
            if(clip!=65535&&(tag<u16(r+6)||tag>u16(r+8)))continue;
            result=u32(r+12); /* Original continues scanning: last match wins. */
        }
    }
    *identity=result;return 1;
}
static int anonymous_bank(const nflstreet_animation_sources *s,uint16_t type,uint16_t *key)
{
    if(s->registry_count>4096||(!s->registry&&s->registry_count))return 0;
    for(size_t i=0;i<s->registry_count;++i){const nflstreet_animation_registry_entry *r=s->registry+i;
        if(!r->registry||r->registry_size!=28)return 0;
        if(r->registry[18]!=1||u16(r->registry+20)!=type)continue;
        if(!r->bank||!r->groups||r->groups_size<4)return 0;
        unsigned count=u16(r->groups+2);if(count>512||r->groups_size<4+(size_t)count*16)return 0;
        for(unsigned j=0;j<count;++j){uint16_t value=u16(r->groups+4+j*16);
            if((value&0x4000)&&(u16(r->bank+6)&8)){*key=value;return 1;}}
    }
    *key=65535;return 1;
}
static int strongest(const uint8_t *a,uint16_t group)
{
    float best=0;int index=-1;
    for(unsigned i=0;i<10;++i){const uint8_t *c=a+0x4e4+i*124;
        if(c[1]==2&&u16(c+8)==group){float weight=real(c+0x30);
            if(!isfinite(weight))return -2;
            if(weight>best){best=weight;index=(int)i;}}}
    return index;
}
static int spawn_transition(uint8_t *a,uint8_t *r,uint16_t new_group,const uint8_t *desc,
                            const nflstreet_animation_sources *s)
{
    const nflstreet_asset_store *store=s->store;const uint8_t *bank,*groups,*table,*program;
    uint16_t anonymous;uint32_t key,identity,flags=u32(a+12);int32_t index;
    if(!store||!anonymous_bank(s,u16(a+0x4da),&anonymous))return 0;
    p16(r+10,anonymous);
    if(!nflstreet_asset_bank_find(store,u16(a+0x4da),u16(desc+12),&key))return 0;
    if(key==0xffffff)return nflstreet_animation_blend(r+0x50,-1,0,10);
    if(key>255||!nflstreet_asset_bank(store,key,&bank)||!bank)return 0;
    groups=nflstreet_asset_read(store,u32(bank+8),4);if(!groups||u32(groups)>512)return 0;
    size_t group_size=4+(size_t)u32(groups)*16;groups=nflstreet_asset_read(store,u32(bank+8),group_size);
    table=nflstreet_asset_read(store,u32(bank+12),4);if(!groups||!table||u32(table)>4096||u16(desc+14)>=u32(table))return 0;
    size_t table_size=4+(size_t)u32(table)*4;table=nflstreet_asset_read(store,u32(bank+12),table_size);
    if(!table||!nflstreet_stream_program_lookup(table,table_size,u16(desc+14),&identity)||
       !(program=nflstreet_asset_read(store,identity,4096)))return 0;
    if(!nflstreet_animation_start(a+0x4e4,1240,a+0x4d8,(uint16_t)key,bank,groups,group_size,
                                 u16(desc+14),65535,anonymous,identity,program,4096,1,&flags,&index))return 0;
    if(index<0)return nflstreet_animation_blend(r+0x50,-1,0,10);
    p32(a+12,flags);
    float duration,factor=real(desc+8);if(!isfinite(factor)||factor<0||factor>1||!nflstreet_animation_program_duration(program,4096,&duration))return 0;
    float blend=mul(mul(duration,add(1,-factor)),0.5f);
    if(!isfinite(blend)||blend<0||blend>=4294967296.0f||duration<0||duration>=4294967296.0f)return 0;
    uint32_t ticks=(uint32_t)blend,remaining=(uint32_t)duration-2*ticks;
    if(!nflstreet_animation_blend(r+0x50,-1,0,ticks)||!nflstreet_animation_blend(r+0x118,0,real(r+0x11c),ticks))return 0;
    /* Original 0x80179310's mode-three parameter block: initial value,
       ramp duration, u8 ramp phase, zero cursor, u16 hold phase. */
    uint8_t *envelope=r+0xb4;float current=real(envelope);
    if(!isfinite(current))return 0;
    pf(envelope+4,1);p32(envelope+8,3);pf(envelope+12,current);pf(envelope+16,(float)(uint8_t)ticks);
    envelope[20]=(uint8_t)ticks;envelope[21]=0;p16(envelope+22,(uint8_t)remaining);
    float hold=add((float)remaining,(float)ticks);
    for(unsigned i=0;i<4;++i){uint8_t *g=a+0x9bc+i*100;if(u16(g+6)&&u16(g+4)==new_group)pf(g+12,hold);}
    for(unsigned i=0;i<10;++i){uint8_t *c=a+0x4e4+i*124;if(c[1]==2&&u16(c+8)==new_group)pf(c+0x48,hold);}
    return 1;
}
int nflstreet_animation_transition(uint8_t actor[0x14d4],uint16_t new_group,uint16_t old_group,
                                  const nflstreet_animation_sources *s)
{
    uint8_t a[0x14d4];uint16_t bank=65535;int bank_found=0;
    if(!actor||!s||!layout(actor)||!s->transition_table||s->transition_table_size<8||
       u16(s->transition_table+4)!=old_group||u16(s->transition_table)!=s->transition_count)return 0;
    memcpy(a,actor,sizeof a);
    for(unsigned i=0;i<4;++i){uint8_t *r=a+0x9bc+i*100;
        if(u16(r+4)==65535){
            if(!bank_found){if(!anonymous_bank(s,u16(a+0x4da),&bank))return 0;bank_found=1;}
            p16(r+10,bank);continue;
        }
        if(u16(r+6)!=2)continue;
        p16(r+6,1);uint32_t identity=0;int old=strongest(a,u16(r+4)),fresh=strongest(a,new_group);
        if(old==-2||fresh==-2)return 0;
        if(s->transition_count&&old>=0&&fresh>=0){
            const uint8_t *c=a+0x4e4+(unsigned)old*124,*n=a+0x4e4+(unsigned)fresh*124;
            uint16_t source_clip=u16(c+2),tag=u16(c+0x64),target_clip=u16(n+2),target_bank=u16(n+4);
            if(!nflstreet_animation_transition_select(s->transition_assets,s->transition_count,source_clip,tag,new_group,target_clip,target_bank,&identity))return 0;
            if(!identity&&!nflstreet_animation_transition_select(s->transition_assets,s->transition_count,source_clip,tag,new_group,65535,0,&identity))return 0;
            if(!identity&&!nflstreet_animation_transition_select(s->transition_assets,s->transition_count,65535,0,new_group,target_clip,target_bank,&identity))return 0;
            if(!identity&&!nflstreet_animation_transition_select(s->transition_assets,s->transition_count,65535,0,new_group,65535,0,&identity))return 0;
            if(identity){const nflstreet_animation_blob *desc=NULL;
                for(size_t j=0;j<s->target_count;++j)if(s->transition_targets[j].identity==identity)desc=s->transition_targets+j;
                if(!desc||!desc->data||desc->size<16)return 0;
                unsigned mode=u16(desc->data+4);
                if(mode==0||mode==2){pf(r+0x50,0);pf(r+0x54,0);p32(r+0x58,0);continue;}
                if(mode==1){float factor=real(desc->data+8),scale=(float)u16(desc->data+6);
                    float ticks=fmaf(mul(scale,real(c+0x50)),add(1,-factor),mul(mul(scale,real(n+0x50)),factor));
                    if(i>1||!isfinite(ticks)||ticks<0||ticks>=4294967296.0)return 0;
                    uint32_t duration=(uint32_t)ticks;
                    if(!nflstreet_animation_blend(r+0x50,-1,0,duration)||
                       !nflstreet_animation_blend(r+0x118,0,real(r+0x11c),duration))return 0;
                    continue;
                }
                if(mode==3){if(!spawn_transition(a,r,new_group,desc->data,s))return 0;continue;}
                /* Unknown original modes fall through to default retirement. */
            }
        }
        if(!nflstreet_animation_blend(r+0x50,-1,0,10))return 0;
    }
    memcpy(actor,a,sizeof a);return 1;
}
static int metadata(uint8_t *data,const uint8_t *partner,uint8_t descriptor,
                    uint16_t clip,const nflstreet_animation_sources *s)
{
    const uint8_t *primary=NULL;
    if(s->metadata_count>64||(!s->metadata_assets&&s->metadata_count))return 0;
    data[0x3e]=data[0x3f]=255;
    for(size_t i=0;i<s->metadata_count;++i){const nflstreet_animation_blob *asset=s->metadata_assets+i;
        if(!asset->data||asset->size<8)return 0;
        unsigned rows=u16(asset->data+4);
        if(rows>127||asset->size<8+(size_t)rows*20)return 0;
        for(unsigned j=0;j<rows;++j){const uint8_t *row=asset->data+8+j*20;
            if(row[2]==0){primary=row;if(u16(row)==clip)break;}}
        if(primary&&u16(primary)==clip)break;
        primary=NULL;
    }
    if(!primary){data[0x1c]=0;return 1;} /* Group 30 fallback helper has no stores. */
    if(descriptor!=0x1b||!partner||data[0x1f])return 0;
    float x=real(primary+8),y=real(primary+12);
    uint32_t heading=u32(partner+0x1c0);
    float sn=nflstreet_sine_angle(heading),cs=nflstreet_sine_angle(heading+0x400000);
    float xx=fmaf(cs,x,-mul(sn,y)),yy=fmaf(cs,y,mul(sn,x));
    xx=add(xx,real(partner+0x1a8));yy=add(yy,real(partner+0x1ac));
    if(!isfinite(xx)||!isfinite(yy))return 0;
    data[0x1c]=1;pf(data,0);pf(data+4,xx);pf(data+8,yy);pf(data+12,0);pf(data+16,0);
    p32(data+0x14,(u32(primary+4)+heading)&0xffffff);return 1;
}
int nflstreet_animation_callback(uint8_t actor[0x14d4],const uint8_t partner[0x14d4],
                                uint8_t descriptor,const nflstreet_animation_sources *s,uint8_t globals[8])
{
    uint8_t a[0x14d4],g[8];uint8_t *r;int32_t index;uint32_t flags,clip,key,program_identity;float speed=0;
    if(!actor||!s||!globals||!layout(actor)||(descriptor!=0x1a&&descriptor!=0x1b)||
       active_group(actor)!=30||s->side_source<-1||s->side_source>255)return 0;
    memcpy(a,actor,sizeof a);memcpy(g,globals,8);r=group_record(a,30);if(!r)return 0;
    key=(uint32_t)a[0x3f2]|(uint32_t)a[0x3f3]<<8;clip=(uint32_t)a[0x3f0]|(uint32_t)a[0x3f1]<<8;
    if(key!=s->bank_key||r[0x2f])return 0;
    if(!nflstreet_stream_program_lookup(s->program_table,s->program_table_size,clip,&program_identity)||
       program_identity!=s->program_identity)return 0;
    p16(r+8,(uint16_t)(u16(r+8)|1));flags=u32(a+12);
    if(!nflstreet_animation_start(a+0x4e4,1240,a+0x4d8,(uint16_t)key,s->bank,s->groups,s->groups_size,
                                 (uint16_t)clip,30,65535,program_identity,s->program,s->program_size,1,&flags,&index)||index<0)return 0;
    uint8_t *d=r+16;d[0x1d]=(uint8_t)index;p32(d+0x18,30);flags|=0x200;p32(a+12,flags);
    if(!metadata(d,partner,descriptor,(uint16_t)clip,s))return 0;
    if(d[0x1f])return 0; /* Mirror toggle paths await their own evidence. */
    if(u32(a+0x30c)){
        if(!s->profile||!s->speed_table)return 0;
        uint32_t category=u32(s->profile+8);
        if(category>=16||s->speed_table_size<4*((size_t)category+1))return 0;
        speed=real(s->speed_table+category*4);
    }
    if(!isfinite(speed))return 0;
    d[0x36]=d[0x34]=0;pf(d+0x30,speed>=0?speed:0);d[0x37]=speed>=0?(uint8_t)index:255;
    if(speed>=0){pf(g,mul(speed,35));pf(g+4,mul(speed,50));}
    d[0x1e]=(uint8_t)(s->side_source==1);
    memcpy(actor,a,sizeof a);memcpy(globals,g,8);return 1;
}
int nflstreet_animation_group_update(uint8_t actor[0x14d4],const uint8_t partner[0x14d4],
                                    uint8_t descriptor,const nflstreet_animation_sources *s,uint8_t globals[8])
{
    uint8_t a[0x14d4],g[8];int32_t index;int old;
    if(!actor||!s||!globals||!layout(actor)||(old=active_group(actor))<0)return 0;
    memcpy(a,actor,sizeof a);memcpy(g,globals,8);
    if(!nflstreet_animation_group_prepare(a+0x9bc,30,1,s->transition_table,s->transition_table_size,&index)||
       !nflstreet_animation_callback(a,partner,descriptor,s,g)||
       !nflstreet_animation_transition(a,30,(uint16_t)old,s))return 0;
    memcpy(actor,a,sizeof a);memcpy(globals,g,8);return 1;
}
static void contact_link(uint8_t *a,const uint8_t *b){uint32_t handle=u32(b);
    for(unsigned i=0;i<2;++i){uint8_t *p=a+0x274+4*i;
        if(!u32(p)){p32(p,handle);return;}if(u32(p)==handle)return;}}
static void contact_request(uint8_t *a,const uint8_t *r){
    a[0x3f0]=r[11];a[0x3f1]=r[10];a[0x3f2]=r[13];a[0x3f3]=r[12];}
int nflstreet_contact_animation_complete(uint8_t actor[0x14d4],uint8_t incoming[0x14d4],
                                        uint8_t manager[256],const uint8_t requests[2][16],
                                        const uint8_t descriptors[2],
                                        const nflstreet_animation_sources sources[2],uint8_t globals[8])
{
    uint8_t a[0x14d4],n[0x14d4],m[256],g[8];const uint8_t *partner;
    if(!actor||!incoming||!manager||!requests||!descriptors||!sources||!globals||manager[1]!=0||
       u16(requests[0]+8)!=30||u16(requests[1]+8)!=30)return 0;
    memcpy(a,actor,sizeof a);memcpy(n,incoming,sizeof n);memcpy(m,manager,256);memcpy(g,globals,8);
    uint32_t handle=u32(m+8+0xb4);
    if(handle==u32(a))partner=a;else if(handle==u32(n))partner=n;else return 0;
    contact_link(a,n);contact_link(n,a);contact_request(a,requests[0]);
    if(!nflstreet_animation_group_update(a,partner,descriptors[0],sources,g))return 0;
    contact_request(n,requests[1]);
    if(!nflstreet_animation_group_update(n,partner,descriptors[1],sources+1,g)||
       !nflstreet_contact_complete(a,n,m,requests[0],requests[1]))return 0;
    memcpy(actor,a,sizeof a);memcpy(incoming,n,sizeof n);memcpy(manager,m,256);memcpy(globals,g,8);return 1;
}
