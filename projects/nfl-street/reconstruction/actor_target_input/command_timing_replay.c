#include "command_timing.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int bytes(const char *text, uint8_t *out, size_t size)
{
    size_t i;
    if (strlen(text)!=size*2) return 0;
    for (i=0;i<size;++i) {
        char pair[3]={text[2*i],text[2*i+1],0};
        if (!strchr("0123456789abcdefABCDEF",pair[0]) || !strchr("0123456789abcdefABCDEF",pair[1])) return 0;
        out[i]=(uint8_t)strtoul(pair,NULL,16);
    }
    return 1;
}
int main(int argc,char **argv)
{
    uint8_t state[64],program[256]; uint16_t commands[64]; size_t count,i;
    unsigned complete; uint32_t flags; char *end; double delta; unsigned long long parsed;
    if (argc!=5 || !bytes(argv[1],state,64) || !bytes(argv[2],program,256)) return 2;
    errno=0; delta=strtod(argv[3],&end); if (errno || end==argv[3] || *end) return 2;
    errno=0; parsed=strtoull(argv[4],&end,0);
    if (errno || end==argv[4] || *end || parsed>UINT32_MAX) return 2;
    flags=(uint32_t)parsed;
    if (!nflstreet_command_timing(state,program,256,delta,&flags,commands,&count,&complete)) return 3;
    for(i=0;i<64;++i) printf("%02x",state[i]);
    printf(" %u %08x",complete,(unsigned)flags);
    for(i=0;i<count;++i) printf(" %04x",commands[i]);
    putchar('\n'); return 0;
}
