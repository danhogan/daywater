#include "contact_selection.h"
#include "pair_requests.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int read(FILE *f,void *p,size_t n) { return fread(p,1,n,f)==n; }
static int word(FILE *f,uint32_t *w) {
    uint8_t b[4];if(!read(f,b,4))return 0;
    *w=(uint32_t)b[0]<<24|(uint32_t)b[1]<<16|(uint32_t)b[2]<<8|b[3];return 1;
}
static void hex(const uint8_t *p,size_t n) { for(size_t i=0;i<n;++i)printf("%02x",p[i]);putchar('\n'); }
int main(int argc,char **argv)
{
    uint8_t params[60],actor[0x14d4],incoming[0x14d4],motion[44],rng[24],generator[256];
    uint8_t manager[256],requests[2][16],descriptor[6],incoming_descriptor[6],native_params[60];
    nflstreet_contact_candidate candidates[64];
    uint8_t assets[64][5128];uint32_t count,scale_bits,identity;uint16_t choice;unsigned accepted;
    float scale;FILE *f;
    if(argc!=2 || !(f=fopen(argv[1],"rb"))) return 2;
    if(!read(f,params,60)||!read(f,actor,sizeof(actor))||!read(f,incoming,sizeof(incoming))||
       !read(f,motion,44)||!word(f,&scale_bits)||!read(f,rng,24)||!read(f,generator,256)||
       !read(f,manager,256)||!read(f,descriptor,6)||!read(f,incoming_descriptor,6)||!word(f,&count)||count>64) { fclose(f);return 2; }
    memcpy(&scale,&scale_bits,4);
    for(unsigned i=0;i<count;++i) {
        uint32_t value,size;
        if(!word(f,&candidates[i].identity)||!word(f,&value)||value>65535) { fclose(f);return 2; }
        candidates[i].choice=(uint16_t)value;
        if(!word(f,&value)||value>65535||!word(f,&size)||size>sizeof(assets[i])||!read(f,assets[i],size)) {
            fclose(f);return 2;
        }
        candidates[i].key=(uint16_t)value;candidates[i].asset=assets[i];candidates[i].size=size;
    }
    if(fgetc(f)!=EOF) { fclose(f);return 2; } fclose(f);
    uint32_t actor_identity=(uint32_t)params[0]<<24|(uint32_t)params[1]<<16|(uint32_t)params[2]<<8|params[3];
    uint32_t incoming_identity=(uint32_t)params[4]<<24|(uint32_t)params[5]<<16|(uint32_t)params[6]<<8|params[7];
    unsigned eligible;
    if(!nflstreet_contact_parameters(actor_identity,incoming_identity,actor,incoming,descriptor,
                                     incoming_descriptor,manager,native_params,&eligible)||!eligible||
       memcmp(params,native_params,60))return 3;
    /* Selection uses native parameters, not the expected parameter bytes. */
    if(!nflstreet_contact_select(native_params,actor,incoming,motion,scale,candidates,count,rng,generator,
                                 &identity,&choice,&accepted)) return 3;
    printf("%08x %u %u\n",identity,choice,accepted);hex(rng,24);hex(generator,256);
    if(identity) {
        unsigned i=0;while(i<count && candidates[i].identity!=identity)++i;
        if(i==count||!nflstreet_pair_requests(candidates[i].asset,candidates[i].size,native_params,choice,
                                           requests[0],requests[1])) return 3;
        hex(requests[0],16);hex(requests[1],16);
        if(!nflstreet_contact_complete(actor,incoming,manager,requests[0],requests[1]))return 3;
        hex(actor,sizeof(actor));hex(incoming,sizeof(incoming));hex(manager,256);
    }
    return 0;
}
