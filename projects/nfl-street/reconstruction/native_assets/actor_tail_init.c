#include "actor_tail_init.h"
static void word(uint8_t*p,uint32_t v) {
    p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);
    p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;
}
int nflstreet_actor_tail_initialize(uint8_t *a,size_t size,
    const nflstreet_actor_tail_defaults *d) {
    if(!a || size<NFLSTREET_ACTOR_TAIL_STORAGE || !d) return 0;
    word(a+0x720,0);word(a+0x724,0);a[0x6f8]=0;a[0x6e4]=0;
    word(a+0x230,d->default_float_bits);word(a+0x234,0);
    word(a+0x238,UINT32_MAX);a[0x23c]=0;
    for(unsigned i=0;i<4;++i) word(a+0x240+4*i,d->defaults[i]);
    word(a+0x6d0,2);word(a+0x6d4,0);a[0x6dc]=0;
    static const uint32_t pairs[4][2]={{0x12,0x10},{3,2},{0x18,0x16},{7,6}};
    for(unsigned i=0;i<4;++i) {
        uint8_t *row=a+i*0x8c;
        word(row+0x78,7);word(row+0x7c,0);word(row+0x80,0);
        word(row+0x84,pairs[i][0]);word(row+0x88,pairs[i][1]);
    }
    word(a+0x71c,0);
    static const unsigned repeated[]={0x710,0x6fc,0x704,0x700,0x70c,0x714};
    for(unsigned i=0;i<sizeof repeated/sizeof repeated[0];++i)
        word(a+repeated[i],d->repeated_float_bits);
    return 1;
}
