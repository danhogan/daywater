#include "animation_callbacks.h"
#include "asset_loader.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void p16(uint8_t *p,unsigned v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static void p32(uint8_t *p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
int main(void)
{
    uint8_t rows[36]={0};p32(rows,2);
    for(unsigned i=0;i<2;++i){uint8_t *r=rows+4+i*16;p16(r,0x8000);p16(r+2,6);p16(r+4,5);p16(r+6,3);p16(r+8,8);p32(r+12,0xabc+i);}
    nflstreet_animation_blob b={0x123,rows,sizeof rows};uint32_t identity=999;
    assert(nflstreet_animation_transition_select(&b,1,5,3,30,6,0,&identity)&&identity==0xabd);
    assert(nflstreet_animation_transition_select(&b,1,5,8,30,6,0,&identity)&&identity==0xabd);
    assert(nflstreet_animation_transition_select(&b,1,5,9,30,6,0,&identity)&&identity==0);
    assert(nflstreet_animation_transition_select(&b,1,5,3,30,65535,0,&identity)&&identity==0);
    b.size=35;identity=999;assert(!nflstreet_animation_transition_select(&b,1,5,3,30,6,0,&identity)&&identity==999);b.size=36;
    uint8_t actor[0x14d4]={0},before[0x14d4],globals[8]={0},saved[8];
    /* Artificial opaque identities for offline tests, not game RAM addresses. */
    p32(actor+0x318,0x100009bc);p32(actor+0x31c,0x100004d8);p32(actor+0x320,0x100004e4);
    p16(actor+0x4dc,10);p16(actor+0x4da,1);p16(actor+0x9c0,77);p16(actor+0x9c2,3);actor[0x3f2]=1;
    uint8_t bank[32]={0},groups[20]={0},program_table[8]={0},program[8]={0x20,0,0,0,0x3e,0x3d,0x3e,0x3e},table[8]={0};
    p32(groups,1);p16(groups+4,30);p32(program_table,1);p32(program_table+4,0x1234);p16(table+4,77);
    nflstreet_animation_sources s={0};s.bank_key=1;s.bank=bank;s.groups=groups;s.groups_size=20;
    s.program_table=program_table;s.program_table_size=8;s.program=program;s.program_size=8;s.program_identity=0x1234;
    s.side_source=-1;s.transition_table=table;s.transition_table_size=8;
    memcpy(before,actor,sizeof actor);memcpy(saved,globals,8);
    program[4]=0x80;program[5]=0x7f;
    assert(!nflstreet_animation_group_update(actor,actor,0x1a,&s,globals));
    assert(!memcmp(actor,before,sizeof actor)&&!memcmp(globals,saved,8));
    program[4]=0x3e;program[5]=0x3d;
    assert(nflstreet_animation_group_update(actor,actor,0x1a,&s,globals));
    /* Spawning without a source store must reject atomically. */
    memcpy(actor,before,sizeof actor);p16(actor+0x9c2,2);
    for(unsigned i=0;i<2;++i){uint8_t *c=actor+0x4e4+i*124;c[1]=2;p16(c+8,i?30:77);p16(c+2,i?6:5);p32(c+0x30,0x3f800000);p16(c+0x64,3);}
    uint8_t desc[16]={0};p16(desc+4,3);nflstreet_animation_blob target={0xabd,desc,16};
    s.transition_assets=&b;s.transition_count=1;s.transition_targets=&target;s.target_count=1;
    p16(table,1);uint8_t expanded[16]={0};memcpy(expanded,table,8);s.transition_table=expanded;s.transition_table_size=16;
    memcpy(before,actor,sizeof actor);
    assert(!nflstreet_animation_transition(actor,30,77,&s));assert(!memcmp(actor,before,sizeof actor));
    /* Failure in the second actor must roll back the entire paired update. */
    uint8_t incoming[0x14d4],saved_incoming[0x14d4],manager[256]={0},saved_manager[256],requests[2][16]={{0},{0}};
    uint8_t descriptors[2]={0x1a,0x1b},bad[8]={0x20,0,0,0,0x80,0x7f,0x3e,0x3e};
    p16(actor+0x9c2,3);p32(actor,0x101);p16(actor+0x154,2);p32(actor+0x158,4);
    memcpy(incoming,actor,sizeof incoming);p32(incoming,0x10101);
    manager[0]=1;p32(manager+8+0xb4,0x101);
    for(unsigned i=0;i<2;++i){p16(requests[i]+8,30);p16(requests[i]+12,1);}
    p16(table,0);s.transition_table=table;s.transition_table_size=8;s.transition_count=s.target_count=0;
    nflstreet_animation_sources pair[2]={s,s};pair[1].program=bad;
    memcpy(before,actor,sizeof actor);memcpy(saved_incoming,incoming,sizeof incoming);memcpy(saved_manager,manager,256);memcpy(saved,globals,8);
    assert(!nflstreet_contact_animation_complete(actor,incoming,manager,requests,descriptors,pair,globals));
    assert(!memcmp(actor,before,sizeof actor)&&!memcmp(incoming,saved_incoming,sizeof incoming)&&
           !memcmp(manager,saved_manager,256)&&!memcmp(globals,saved,8));
    /* Offset relocation rejects truncated records without partial writes. */
    uint8_t raw[512]={0},raw_saved[512];p16(raw+6,8);p16(raw+4,1);
    p32(raw+8,32);p32(raw+12,224);p32(raw+16,300);p32(raw+32,1);
    p16(raw+36,30);p32(raw+40,80);p32(raw+44,112);p32(raw+48,128);
    p16(raw+80,1);p16(raw+84,22);p32(raw+88,112);p32(raw+128,1);p32(raw+144,192);
    p32(raw+224,1);p32(raw+228,256);p32(raw+300,400);
    memcpy(raw_saved,raw,512);p32(raw+144,512);memcpy(raw_saved,raw,512);
    assert(!nflstreet_asset_relocate(raw,512,0x10000000)&&!memcmp(raw,raw_saved,512));
    p32(raw+144,192);assert(nflstreet_asset_relocate(raw,512,0x10000000));
    memcpy(raw_saved,raw,512);assert(nflstreet_asset_relocate(raw,512,0x10000000)&&!memcmp(raw,raw_saved,512));
    uint8_t registry[28]={0};registry[18]=1;p16(registry+20,1);p32(registry+12,0x10000000);
    nflstreet_asset_store store={0x10000000,raw,registry,512,28};const uint8_t *found=NULL;uint32_t key=7;
    assert(!nflstreet_asset_read(&store,0x0fffffff,1));assert(!nflstreet_asset_read(&store,0x100001ff,2));
    assert(nflstreet_asset_bank(&store,1,&found)&&found==NULL);
    assert(nflstreet_asset_bank_find(&store,2,0,&key)&&key==0xffffff);
    assert(nflstreet_asset_metadata(&store,1,30,&key)&&key==0x10000070);
    uint8_t timed[12]={0x20,0,0,0,0,1,0,2,0x80,0,0,0};float duration=999;
    assert(nflstreet_animation_program_duration(timed,12,&duration)&&duration==2);
    duration=999;assert(!nflstreet_animation_program_duration(timed,8,&duration)&&duration==999);
    puts("callback, transition, asset bounds and atomic rollback checks passed");return 0;
}
