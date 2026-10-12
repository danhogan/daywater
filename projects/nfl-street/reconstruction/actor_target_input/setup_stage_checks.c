#include "command_timing.h"
#include "early_event.h"
#include "pair_requests.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    uint8_t s[64], saved[64], program[8]={0,0,0,0,0,7,0x80,0};
    uint8_t asset[48]={0}, params[60]={0}, current[16], incoming[16];
    uint8_t e[12]={0}, request[3]={7,8,9}, control[16]={0};
    uint8_t desc[6]={0}, other[6]={0}, g[48]={0}, ig[48]={0};
    uint8_t target[12]={0}, mode[2]={0}, field[12]={0};
    uint16_t commands[64]; size_t count=77; unsigned done=77;
    uint32_t flags=0x15, identity=0x12345678;
    const uint8_t table[8]={0,0,0,0,0x80,0,0,4};
    assert(nflstreet_stream_program_lookup(table,8,0,&identity) && identity==0x80000004);
    assert(!nflstreet_stream_program_lookup(table,8,1,&identity) && identity==0x80000004);
    memset(s,0x5a,sizeof(s)); memcpy(saved,s,64);
    program[6]=0x80; program[7]=7; /* Unported loop-stack operation. */
    assert(!nflstreet_stream_setup(s,identity,program,8,&flags,commands,&count,&done));
    assert(!memcmp(s,saved,64) && flags==0x15 && count==77 && done==77);
    program[7]=0;
    assert(nflstreet_stream_setup(s,identity,program,8,&flags,commands,&count,&done));
    assert(s[4]==0x3f && s[5]==0x80 && s[6]==0 && s[7]==0 && done==1);
    assert(!memcmp(s+32,saved+32,32)); /* Preserved interpreter tail. */
    asset[5]=2; asset[8]=2; asset[9]=0x49; asset[10]=1;
    asset[28]=2; asset[29]=0x4a; params[0x31]=0x1e;
    assert(nflstreet_pair_requests(asset,48,params,1,current,incoming));
    assert(current[10]==2 && current[11]==0x49 && incoming[11]==0x4a);
    memset(current,0x5a,16); memset(incoming,0x5a,16);
    asset[30]=1; /* Missing the incoming type: reject both outputs intact. */
    assert(!nflstreet_pair_requests(asset,48,params,1,current,incoming));
    for(unsigned i=0;i<16;++i) assert(current[i]==0x5a && incoming[i]==0x5a);
    e[11]=4; e[6]=2; flags=0x15;
    assert(nflstreet_early_event(e,&flags,request,control,1,desc,other,g,ig,target,mode,0,field,1,0));
    assert(e[5]==1 && e[7]==4 && flags==0x11 && request[0]==4 && request[1]==1 && request[2]==1);
    memcpy(saved,e,12); memcpy(saved+12,request,3); memcpy(saved+15,control,16);
    assert(nflstreet_early_event(e,&flags,request,control,1,desc,other,g,ig,target,mode,0,field,1,0));
    assert(!memcmp(saved,e,12) && !memcmp(saved+12,request,3) && !memcmp(saved+15,control,16));
    e[5]=2; other[0]=0x1b; memcpy(saved,e,12);
    assert(!nflstreet_early_event(e,&flags,request,control,1,desc,other,g,ig,target,mode,0,field,1,0));
    assert(!memcmp(saved,e,12) && flags==0x11 && !memcmp(saved+12,request,3) && !memcmp(saved+15,control,16));
    puts("setup/stage guards passed");
    return 0;
}
