#include "early_event.h"
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
static void print(const uint8_t *p,size_t count)
{ size_t i; for(i=0;i<count;++i) printf("%02x",p[i]); }
int main(int argc,char **argv)
{
    uint8_t e[12],r[3],c[16],d[6],id[6],g[48],ig[48],t[12],m[2],f[12];
    uint32_t flags,incoming_408; unsigned long long parsed; char *end; int owns,enabled,tag;
    if(argc!=16 || !hex(argv[1],e,12) || !hex(argv[3],r,3) || !hex(argv[4],c,16) ||
        !hex(argv[6],d,6) || !hex(argv[7],id,6) || !hex(argv[8],g,48) || !hex(argv[9],ig,48) ||
        !hex(argv[10],t,12) || !hex(argv[11],m,2) || !hex(argv[13],f,12)) return 2;
    errno=0; parsed=strtoull(argv[2],&end,0);
    if(errno || end==argv[2] || *end || parsed>UINT32_MAX) return 2;
    flags=(uint32_t)parsed;
    if((strcmp(argv[5],"0") && strcmp(argv[5],"1")) || (strcmp(argv[12],"0") && strcmp(argv[12],"1"))) return 2;
    owns=argv[5][0]-'0'; enabled=argv[12][0]-'0';
    errno=0; parsed=strtoull(argv[15],&end,0);
    if(errno || end==argv[15] || *end || parsed>UINT32_MAX ||
        (strcmp(argv[14],"0") && strcmp(argv[14],"1"))) return 2;
    incoming_408=(uint32_t)parsed; tag=argv[14][0]-'0';
    if(!nflstreet_early_event(e,&flags,r,c,owns,d,id,g,ig,t,m,(uint32_t)enabled,f,(uint8_t)tag,incoming_408)) return 3;
    print(e,12); printf(" %08x ",flags); print(r,3); putchar(' '); print(c,16); putchar('\n');
    return 0;
}
