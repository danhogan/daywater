#include "animation_channels.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
static int readn(FILE *f,void *p,size_t n){return fread(p,1,n,f)==n;}
static int word(FILE *f,uint32_t *v){uint8_t b[4];if(!readn(f,b,4))return 0;
    *v=(uint32_t)b[0]<<24|(uint32_t)b[1]<<16|(uint32_t)b[2]<<8|b[3];return 1;}
static int floatn(FILE *f,float *v){uint32_t w;if(!word(f,&w))return 0;memcpy(v,&w,4);return 1;}
static void hex(const uint8_t *p,size_t n){for(size_t i=0;i<n;++i)printf("%02x",p[i]);putchar('\n');}
int main(int argc,char **argv)
{
    FILE *f;uint8_t data[1984],header[16],bank[32],groups[8196],program[4096],transitions[520];
    uint32_t count,n,flags=0,key,clip,group,override,identity,gs,ps,duration;float current,target;
    int32_t index=-1;int ok=0,mode;size_t size=0;
    if(argc!=2||!(f=fopen(argv[1],"rb")))return 2;
    mode=fgetc(f);
    if(mode=='A'){
        if(!word(f,&count)||count>16||!readn(f,data,count*124))goto invalid;
        size=count*124;ok=nflstreet_animation_allocate(data,size,count,&index);
    }else if(mode=='M'){
        if(!readn(f,data,404))goto invalid;
        size=404;ok=nflstreet_animation_manager_allocate(data,&index);
    }else if(mode=='B'){
        if(!readn(f,data,20)||!floatn(f,&current)||!floatn(f,&target)||!word(f,&duration))goto invalid;
        size=20;ok=nflstreet_animation_blend(data,current,target,duration);
    }else if(mode=='G'){
        if(!readn(f,data,404)||!word(f,&group)||group>65535||!floatn(f,&target)||
           !word(f,&n)||n>520||!readn(f,transitions,n))goto invalid;
        size=404;ok=nflstreet_animation_group_prepare(data,(uint16_t)group,target,transitions,n,&index);
    }else if(mode=='C'){
        if(!readn(f,header,16)||!word(f,&count)||count>16||!readn(f,data,count*124)||
           !word(f,&key)||key>65535||!readn(f,bank,32)||!word(f,&gs)||gs>sizeof(groups)||
           !readn(f,groups,gs)||!word(f,&clip)||clip>65535||!word(f,&group)||group>65535||
           !word(f,&override)||override>65535||!word(f,&identity)||!word(f,&ps)||ps>sizeof(program)||
           !readn(f,program,ps)||!floatn(f,&current)||!word(f,&flags))goto invalid;
        size=count*124;ok=nflstreet_animation_start(data,size,header,(uint16_t)key,bank,groups,gs,
            (uint16_t)clip,(uint16_t)group,(uint16_t)override,identity,program,ps,current,&flags,&index);
    }else goto invalid;
    if(fgetc(f)!=EOF)goto invalid;
    fclose(f);if(!ok)return 3;
    printf("%d %08x\n",index,flags);hex(data,size);return 0;
invalid:fclose(f);return 2;
}
