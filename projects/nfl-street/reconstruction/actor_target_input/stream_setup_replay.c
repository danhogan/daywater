#include "command_timing.h"
#include "pair_requests.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int hex(const char *s,uint8_t *out,size_t count)
{
    size_t i;
    if(strlen(s)!=count*2) return 0;
    for(i=0;i<count;++i) {
        char p[3]={s[2*i],s[2*i+1],0};
        if(!strchr("0123456789abcdefABCDEF",p[0]) || !strchr("0123456789abcdefABCDEF",p[1])) return 0;
        out[i]=(uint8_t)strtoul(p,NULL,16);
    }
    return 1;
}
static int number(const char *s,uint32_t *out)
{
    char *end; unsigned long long n; errno=0; n=strtoull(s,&end,0);
    if(errno || end==s || *end || n>UINT32_MAX) return 0;
    *out=(uint32_t)n; return 1;
}
int main(int argc,char **argv)
{
    uint8_t state[64],program[256],table[16388],params[60],current[16],incoming[16]; uint32_t identity,flags,index;
    uint16_t commands[64]; unsigned done; size_t count,i,size;
    if(argc==5 && !strcmp(argv[1],"P")) {
        size=strlen(argv[2])/2;
        if(size>sizeof(table) || !hex(argv[2],table,size) || !hex(argv[3],params,60) ||
            !number(argv[4],&index) || index>UINT16_MAX) return 2;
        if(!nflstreet_pair_requests(table,size,params,(uint16_t)index,current,incoming)) return 3;
        for(i=0;i<16;++i) printf("%02x",current[i]); putchar(' ');
        for(i=0;i<16;++i) printf("%02x",incoming[i]); putchar('\n'); return 0;
    }
    if(argc==4 && !strcmp(argv[1],"L")) {
        size=strlen(argv[2])/2;
        if(size>sizeof(table) || !hex(argv[2],table,size) || !number(argv[3],&index)) return 2;
        if(!nflstreet_stream_program_lookup(table,size,index,&identity)) return 3;
        printf("%08x\n",identity); return 0;
    }
    if(argc!=6 || strcmp(argv[1],"S") || !hex(argv[2],state,64) || !hex(argv[3],program,256) ||
        !number(argv[4],&identity) || !number(argv[5],&flags)) return 2;
    if(!nflstreet_stream_setup(state,identity,program,256,&flags,commands,&count,&done)) return 3;
    for(i=0;i<64;++i) printf("%02x",state[i]);
    printf(" %u %08x",done,flags);
    for(i=0;i<count;++i) printf(" %04x",commands[i]);
    putchar('\n'); return 0;
}
