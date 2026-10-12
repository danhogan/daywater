#include "animation_channels.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void p16(unsigned char *p,unsigned v){p[0]=(unsigned char)(v>>8);p[1]=(unsigned char)v;}
int main(void)
{
    unsigned char pool[3*124]={0},before[3*124],manager[404]={0},saved[404];int32_t index=99;
    pool[1]=2;pool[125]=1;pool[249]=0;
    assert(nflstreet_animation_allocate(pool,sizeof pool,3,&index)&&index==2);
    pool[249]=2;assert(nflstreet_animation_allocate(pool,sizeof pool,3,&index)&&index==1);
    pool[125]=2;pool[8]=pool[132]=0x80;
    assert(nflstreet_animation_allocate(pool,sizeof pool,3,&index)&&index==1&&pool[125]==0);
    memcpy(before,pool,sizeof pool);
    assert(!nflstreet_animation_allocate(pool,123,3,&index)&&!memcmp(before,pool,sizeof pool));
    for(unsigned i=0;i<4;++i)p16(manager+i*100+6,3);
    memcpy(saved,manager,404);
    assert(!nflstreet_animation_manager_allocate(manager,&index)&&!memcmp(saved,manager,404));
    /* Two slots needed when a transition placeholder is created. */
    p16(manager+306,0);p16(manager+4,77);p16(manager+104,74);p16(manager+204,73);
    unsigned char transitions[16]={0};p16(transitions,1);p16(transitions+4,77);
    memcpy(saved,manager,404);
    assert(!nflstreet_animation_group_prepare(manager,30,1,transitions,16,&index));
    assert(!memcmp(saved,manager,404));
    /* A bad clip program must leave the chosen channel and flags unchanged. */
    unsigned char header[16]={0},bank[32]={0},groups[4]={0},bad[6]={0x20,0,0,0,0x80,0x7f};
    uint32_t flags=0x1234;p16(header+4,3);memcpy(before,pool,sizeof pool);
    assert(!nflstreet_animation_start(pool,sizeof pool,header,1,bank,groups,4,585,30,65535,
                                     0x80800012,bad,6,1,&flags,&index));
    assert(!memcmp(before,pool,sizeof pool)&&flags==0x1234);
    puts("animation bounds and transactional rejection checks passed");return 0;
}
