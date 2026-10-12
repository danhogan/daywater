#include "contact_selection.h"
#include "pair_requests.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void put(uint8_t *p,uint32_t w) { p[0]=(uint8_t)(w>>24);p[1]=(uint8_t)(w>>16);p[2]=(uint8_t)(w>>8);p[3]=(uint8_t)w; }
int main(void)
{
    uint8_t p[60]={0},a[0x14d4]={0},n[0x14d4]={0},motion[44]={0};
    uint8_t rng[24]={0},generator[256]={0},manager[256]={0},asset[48]={0};
    uint8_t request[16],other[16],saved_rng[24],saved_generator[256],saved_actor[0x14d4];
    uint32_t identity=77;uint16_t choice=77;unsigned accepted=77,eligible;
    uint8_t descriptor[6]={0x1a},incoming_descriptor[6]={0x1b},native_params[60];
    nflstreet_contact_candidate candidate={1,1,1,asset,48};
    p[0x32]=p[0x33]=255;put(p+0x24,0x155555);put(p+0x28,0x3f59999a);p[0x31]=30;
    put(n+0x1a8,0x3f800000);asset[5]=2;asset[8]=2;asset[9]=0x49;asset[10]=1;
    asset[28]=2;asset[29]=0x4a;put(asset+36,0x3f800000);
    assert(nflstreet_contact_select(p,a,n,motion,115,&candidate,1,rng,generator,&identity,&choice,&accepted));
    assert(identity==1 && choice==1 && accepted==1 && rng[11]==1);
    memcpy(saved_rng,rng,24);memcpy(saved_generator,generator,256);
    put(generator,1);memcpy(saved_generator,generator,256);identity=choice=accepted=77;
    assert(!nflstreet_contact_select(p,a,n,motion,115,&candidate,1,rng,generator,&identity,&choice,&accepted));
    assert(!memcmp(rng,saved_rng,24) && !memcmp(generator,saved_generator,256));
    assert(identity==77 && choice==77 && accepted==77);
    put(generator,0);asset[5]=255;
    assert(!nflstreet_contact_select(p,a,n,motion,115,&candidate,1,rng,generator,&identity,&choice,&accepted));
    asset[5]=2;p[0x37]=1;
    assert(!nflstreet_contact_select(p,a,n,motion,115,&candidate,1,rng,generator,&identity,&choice,&accepted));
    p[0x37]=0;
    assert(nflstreet_pair_requests(asset,48,p,1,request,other));
    put(a,257);put(n,0x10101);put(a+12,5);a[0x155]=2;a[0x15b]=4;manager[0xd3]=9;
    a[0x156]=2;a[0x157]=4;put(manager+8+0xb4,257);put(n+0x408,1);
    assert(nflstreet_contact_parameters(1,2,a,n,descriptor,incoming_descriptor,manager,native_params,&eligible));
    assert(eligible && native_params[0x31]==30 && native_params[0x36]==1);
    put(n+0x1a8,0x40a00000);
    assert(nflstreet_contact_parameters(1,2,a,n,descriptor,incoming_descriptor,manager,native_params,&eligible));
    assert(!eligible);put(n+0x1a8,0x3f800000);
    assert(nflstreet_contact_complete(a,n,manager,request,other));
    assert(a[0x155]==3 && a[14]==2 && a[15]==0 && n[14]==2 && manager[0xd3]==0);
    assert(a[0x3f0]==0x49 && n[0x3f0]==0x4a && a[0x275]==1 && a[0x277]==1 && n[0x277]==1);
    memcpy(saved_actor,a,sizeof(a));
    assert(!nflstreet_contact_complete(a,n,manager,request,other));
    assert(!memcmp(saved_actor,a,sizeof(a)));
    puts("contact rollback and completion guards passed");return 0;
}
