#include "pose.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned u16(const uint8_t *p){return p[0]*256u+p[1];}
static unsigned u32(const uint8_t *p){return p[0]*16777216u+p[1]*65536u+p[2]*256u+p[3];}
int main(int argc,char **argv){
    if(argc!=2)return 2;FILE *f=fopen(argv[1],"rb");assert(f);assert(!fseek(f,0,SEEK_END));long size=ftell(f);assert(size>32);rewind(f);
    uint8_t *bank=malloc((size_t)size);assert(bank&&fread(bank,1,(size_t)size,f)==(size_t)size);fclose(f);
    int16_t output[384],before[384];unsigned count=999;memset(output,0xa5,sizeof output);memcpy(before,output,sizeof output);
    assert(!nflstreet_pose_frame(bank,31,0,output,384,&count));assert(count==999&&!memcmp(output,before,sizeof output));
    assert(!nflstreet_pose_frame(bank,size,u16(bank+4),output,384,&count));assert(count==999&&!memcmp(output,before,sizeof output));
    assert(!nflstreet_pose_frame(bank,size,0,output,1,&count));assert(count==999&&!memcmp(output,before,sizeof output));
    unsigned descriptor=u32(bank+20),width=u32(bank+descriptor+20);uint8_t saved=bank[width+1];bank[width+1]=17;
    assert(!nflstreet_pose_frame(bank,size,0,output,384,&count));assert(count==999&&!memcmp(output,before,sizeof output));bank[width+1]=saved;
    assert(nflstreet_pose_frame(bank,size,0,output,384,&count));assert(count==u32(bank+descriptor));
    printf("Pose checks: bounded frame/table/capacity/bit widths, transactional rejection, valid frame (%u components)\n",count);free(bank);return 0;
}
