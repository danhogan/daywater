#include "environment.h"
#include <math.h>
#include <string.h>
static unsigned u16(const uint8_t *p){return (unsigned)p[0]<<8|p[1];}
static uint32_t u32(const uint8_t *p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static float f32(const uint8_t *p){uint32_t bits=u32(p);float f;memcpy(&f,&bits,4);return f;}
int nflstreet_environment_open(const uint8_t *b,size_t n,nflstreet_environment *out){
    if(!b||!out||n<156||u32(b)!=105)return 0;
    const unsigned strides[17]={8,36,24,24,24,24,24,24,56,36,36,36,36,48,48,60,4};
    nflstreet_environment e={0};e.bytes=b;e.size=n;size_t at=156;
    for(unsigned i=0;i<17;++i){e.count[i]=u32(b+16+i*8);e.offset[i]=at;
        if(e.count[i]>(n-at)/strides[i])return 0;
        at+=(size_t)e.count[i]*strides[i];
    }
    *out=e;return 1;
}
int nflstreet_environment_instance(const nflstreet_environment *e,unsigned group,unsigned index,nflstreet_scene_instance *out){
    if(!e||!out||group>2)return 0;
    const unsigned records[3]={9,11,12},names[3]={3,5,6};unsigned array=records[group],table=names[group];
    if(index>=e->count[array])return 0;
    const uint8_t *r=e->bytes+e->offset[array]+index*36;unsigned key=u16(r+24);
    if(key>=e->count[table])return 0;
    const char *s=(const char *)e->bytes+e->offset[table]+key*24;const char *end=memchr(s,0,24);
    if(!end||end==s)return 0;
    nflstreet_scene_instance v={0};memcpy(v.name,s,(size_t)(end-s));v.group=group;v.index=index;v.name_index=key;
    for(unsigned i=0;i<3;++i){v.position[i]=f32(r+i*4);v.angles[i]=f32(r+12+i*4);
        if(!isfinite(v.position[i])||!isfinite(v.angles[i])||fabsf(v.angles[i])>36000)return 0;
    }
    *out=v;return 1;
}
int nflstreet_environment_rotation(const nflstreet_scene_instance *v,float m[9]){
    if(!v||!m)return 0;
    /* 0x800D3A70 -> 0x801A3794: fixed-angle conversion factor from
       original DOL 0x80252810, then arithmetic shift by one. The original
       sine lookup remains approximated by host sin/cos for inspection. */
    double angle[3];
    for(unsigned i=0;i<3;++i){if(!isfinite(v->angles[i])||fabsf(v->angles[i])>36000)return 0;
        int32_t raw=(int32_t)((double)v->angles[i]*46603.37890625);int32_t half=raw/2;
        if(raw<0&&raw%2)--half;
        angle[i]=(double)half*(6.2831853071795864769/16777216.0);
    }
    double sx=sin(angle[0]),sy=sin(angle[1]),sz=sin(angle[2]),cx=cos(angle[0]),cy=cos(angle[1]),cz=cos(angle[2]);
    double x=sx*cy*cz+cx*sy*sz,y=cx*cy*sz+sx*sy*cz,z=cx*sy*cz-sx*cy*sz,w=cx*cy*cz-sx*sy*sz;
    m[0]=(float)(1-2*(y*y+z*z));m[1]=(float)(2*(x*y-z*w));m[2]=(float)(2*(x*z+y*w));
    m[3]=(float)(2*(x*y+z*w));m[4]=(float)(1-2*(x*x+z*z));m[5]=(float)(2*(y*z-x*w));
    m[6]=(float)(2*(x*z-y*w));m[7]=(float)(2*(y*z+x*w));m[8]=(float)(1-2*(x*x+y*y));return 1;
}
