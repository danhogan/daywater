#include "command_timing.h"
#include <assert.h>
#include <string.h>

static void seed(uint8_t state[64])
{
    memset(state,0,64);
    state[4]=0x40; /* interval = 2 */
    state[0x1a]=state[0x1b]=0xff;
}
int main(void)
{
    uint8_t state[64],before[64],program[256]={0};
    uint16_t commands[64]; size_t count=99; unsigned done=99;
    uint32_t flags=0x10000000;
    /* A callback followed by an unsupported callback must roll back every
       output, including effects produced before the rejection. */
    seed(state); memcpy(before,state,64);
    program[4]=0xc0; program[5]=0x0c; program[6]=0xc0; program[7]=0x10;
    memset(commands,0xa5,sizeof(commands));
    assert(!nflstreet_command_timing(state,program,256,1,&flags,commands,&count,&done));
    assert(!memcmp(state,before,64) && flags==0x10000000 && count==99 && done==99 && commands[0]==0xa5a5);
    /* Crossing multiple intervals reads three tags and emits one command.
       The first advancement uses zero, later ones use the interval. */
    seed(state); memset(program,0,256);flags=0x10040000;
    program[5]=1; program[7]=2; program[8]=0xc0; program[9]=6; program[11]=3;
    assert(nflstreet_command_timing(state,program,256,5,&flags,commands,&count,&done));
    assert(count==1 && commands[0]==0xc006 && flags==0x10040001 && !done);
    assert(state[0x19]==4 && state[0x1d]==3 && state[8]==0x40 && state[9]==0xa0);
    /* Equality waits. There is no early emission at countdown == zero. */
    seed(state); state[12]=0x3f; state[13]=0x80;
    program[4]=0xc0; program[5]=6; flags=0;
    assert(nflstreet_command_timing(state,program,6,1,&flags,commands,&count,&done));
    assert(!count && !flags && !state[0x19] && !state[12] && !state[13]);
    /* Non-advancing control cycles cannot hang the host. */
    seed(state); memcpy(before,state,64); program[4]=0x80; program[5]=1;
    assert(!nflstreet_command_timing(state,program,6,1,&flags,commands,&count,&done));
    assert(!memcmp(state,before,64));
    return 0;
}
