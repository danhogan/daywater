#include "contact_selection.h"
#include "../kinematics/kinematics.h"
#include <math.h>
#include <string.h>
static uint16_t u16(const uint8_t *p) { return (uint16_t)((unsigned)p[0]<<8|p[1]); }
static uint32_t u32(const uint8_t *p) { return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3]; }
static void put(uint8_t *p,uint32_t w) { p[0]=(uint8_t)(w>>24);p[1]=(uint8_t)(w>>16);p[2]=(uint8_t)(w>>8);p[3]=(uint8_t)w; }
static float real(const uint8_t *p) { uint32_t w=u32(p);float f;memcpy(&f,&w,4);return f; }
static float mul(float a,float b) { volatile float f=a*b;return f; }
static float sub(float a,float b) { volatile float f=a-b;return f; }
static uint32_t dist(uint32_t a,uint32_t b) { uint32_t d=(a-b)&0xffffff;return d<=0x800000?d:0x1000000-d; }
int nflstreet_contact_parameters(uint32_t actor_identity,uint32_t incoming_identity,
                                 const uint8_t a[0x14d4],const uint8_t n[0x14d4],
                                 const uint8_t d[6],const uint8_t nd[6],
                                 const uint8_t manager[256],uint8_t params[60],unsigned *eligible)
{
    uint8_t p[60]={0};float dx,dy,length;uint32_t angle;unsigned ready;
    if(!a||!n||!d||!nd||!manager||!params||!eligible||!actor_identity||!incoming_identity||
       d[0]!=0x1a||nd[0]!=0x1b||u16(a+0x154)!=2||u32(a+0x158)!=4||a[0x156]!=2||
       a[0x157]!=4||manager[1]!=0) return 0;
    dx=sub(real(a+0x1a8),real(n+0x1a8));dy=sub(real(a+0x1ac),real(n+0x1ac));
    length=sqrtf(fmaf(dx,dx,mul(dy,dy)));
    if(!isfinite(dx)||!isfinite(dy)||!isfinite(length)||length==0) return 0;
    angle=nflstreet_angle_from_vector(dy,dx);
    ready=u32(manager+8+0xb4)==u32(a) && length<4.0f &&
          (dist(u32(n+0x1c0),angle)<0x2aaaaa || (n[3]==1 && u32(n+0x408)));
    put(p,actor_identity);put(p+4,incoming_identity);
    put(p+0x20,0x2aaaaa);put(p+0x24,0x155555);put(p+0x28,0x3f59999a);put(p+0x2c,0x40800000);
    p[0x31]=30;p[0x32]=p[0x33]=255;p[0x36]=1;
    memcpy(params,p,60);*eligible=ready;return 1;
}
static int match(const uint8_t *p,const uint8_t *a,const uint8_t *n,const uint8_t *motion,
                 float scale,const uint8_t *current,const uint8_t *other)
{
    uint32_t expected=p[0x35] ? u32(current+4)-u32(other+4) : u32(other+4)-u32(current+4);
    uint32_t h=u32(a+0x1c0), ih=u32(n+0x1c0), hc=h+u32(a+0x12c), ic=ih+u32(n+0x12c);
    float dx=sub(real(n+0x1a8),real(a+0x1a8)),dy=sub(real(n+0x1ac),real(a+0x1ac));
    float s=nflstreet_sine_angle(0x1000000-h),c=nflstreet_sine_angle(0x1400000-h);
    float x=fmaf(c,dx,-mul(s,dy)),y=fmaf(c,dy,mul(s,dx));
    float rx=real(other+8),ry=real(other+12),limit=real(p+0x28);
    uint32_t tolerance=u32(p+0x24); unsigned dynamic=0;
    if(p[0x36]==1)
        dynamic=real(motion+8)<1.0f && sub(real(motion+0x28),real(motion+0x18))>0.5f;
    if((dynamic && other[3]!=2) || (!dynamic && other[3]==2)) return 0;
    if(dist(expected,ih-h)>=tolerance && dist(expected,ic-hc)>=tolerance &&
       dist(expected,ih-hc)>=tolerance && dist(expected,ic-h)>=tolerance) return 0;
    if(p[0x35]) ry=-ry;
    if(!isfinite(x)||!isfinite(y)||!isfinite(rx)||!isfinite(ry)||!isfinite(limit)||limit<=0 ||
       fabsf(sub(x,rx))>=limit) return 0;
    if(fabsf(sub(y,ry))>=limit) return 0; /* Optional distance ordering is unsupported. */
    if((x==0 && y==0)||(rx==0 && ry==0)) return 0;
    return dist(nflstreet_angle_from_vector(y,x),nflstreet_angle_from_vector(ry,rx)) <=
           (uint32_t)mul(scale,46603.37890625f);
}
static int random_index(uint8_t *rng,uint8_t *gen,unsigned count,unsigned *index)
{
    float value;
    if(u32(gen)==0) {
        uint32_t seed=u32(gen+4)*UINT32_C(1664525)+UINT32_C(1013904223);
        put(gen+4,seed);value=mul((float)seed,0x1p-32f);
        if(value>=0.9999998807907104f) value=0.9999998807907104f;
    } else if(u32(gen)==2) {
        uint32_t first=u32(gen+4),second=u32(gen+8);
        int64_t difference;
        if(first>55 || second>55) return 0;
        first=first==55 ? 1 : first+1;second=second==55 ? 1 : second+1;
        difference=(int64_t)(int32_t)u32(gen+16+4*first)-(int32_t)u32(gen+16+4*second);
        if(difference<0) difference+=1000000000;
        if(difference<0 || difference>=1000000000) return 0;
        put(gen+4,first);put(gen+8,second);put(gen+16+4*first,(uint32_t)difference);
        value=mul((float)difference,9.999999717180685e-10f);
        if(value>=0.9999998807907104f) value=0.9999998807907104f;
    } else return 0;
    value=mul(value,(float)count);
    if(!isfinite(value)||value<0 || value>=(float)count) return 0;
    *index=(unsigned)value;
    put(rng+8,u32(rng+8)+1);put(rng+12,(u32(rng+12)^count)+7);return 1;
}
int nflstreet_contact_select(const uint8_t p[60],const uint8_t a[0x14d4],
                            const uint8_t n[0x14d4],const uint8_t motion[44],
                            float scale,const nflstreet_contact_candidate *candidates,
                            size_t count,uint8_t rng[24],uint8_t generator[256],
                            uint32_t *identity,uint16_t *choice,unsigned *accepted)
{
    size_t eligible[12];unsigned found=0,index=0;uint8_t r[24],g[256];
    if(!p||!a||!n||!motion||!candidates||!rng||!generator||!identity||!choice||!accepted||
       count>64 || p[0x32]!=255 || p[0x34] || p[0x37] || !isfinite(scale)||scale<0 || scale>180) return 0;
    for(unsigned i=8;i<32;++i) if(p[i]) return 0; /* One source actor only. */
    memcpy(r,rng,24);memcpy(g,generator,256);
    for(size_t i=0;i<count;++i) {
        const uint8_t *data=candidates[i].asset,*rows[2]={0,0};unsigned types[2]={0,0};
        if(!data || candidates[i].size<8) return 0;
        unsigned row_count=u16(data+4);
        if(row_count>256 || 8+(size_t)row_count*20>candidates[i].size) return 0;
        if(p[0x33]!=255 && p[0x33]!=i) continue;
        if(p[0x32]!=255 && p[0x32]!=data[4]) continue;
        for(unsigned j=0;j<row_count;++j) {
            const uint8_t *row=data+8+j*20;unsigned type=row[2]!=0;
            if(!rows[type]) rows[type]=row;
            ++types[type];
        }
        if(types[0]!=1 || !types[1]) return 0;
        if(match(p,a,n,motion,scale,rows[1],rows[0]) && found<12) eligible[found++]=i;
    }
    if(found && !random_index(r,g,found,&index)) return 0;
    *identity=found ? candidates[eligible[index]].identity : 0;
    *choice=found ? candidates[eligible[index]].choice : 0;
    *accepted=found;memcpy(rng,r,24);memcpy(generator,g,256);return 1;
}
static void link(uint8_t *a,const uint8_t *b)
{
    uint32_t handle=u32(b);
    for(unsigned i=0;i<2;++i) {
        uint32_t current=u32(a+0x274+4*i);
        if(!current) { put(a+0x274+4*i,handle);return; }
        if(current==handle) return;
    }
}
static void request(uint8_t *a,const uint8_t *r)
{ a[0x3f0]=r[11];a[0x3f1]=r[10];a[0x3f2]=r[13];a[0x3f3]=r[12]; }
int nflstreet_contact_complete(uint8_t a[0x14d4],uint8_t n[0x14d4],uint8_t manager[256],
                              const uint8_t current[16],const uint8_t incoming[16])
{
    if(!a||!n||!manager||!current||!incoming||u16(a+0x154)!=2||u32(a+0x158)!=4 ||
       u16(current+8)!=30 || u16(incoming+8)!=30 || manager[1]!=0) return 0;
    link(a,n);link(n,a);request(a,current);request(n,incoming);
    /* Group 30's observed animation callback sets bit 0x200 before the
       contact service clears advance/ready bits on the current actor. */
    a[0x20e]=n[0x20e]=0;put(a+0xc,(u32(a+0xc)|UINT32_C(0x200))&~UINT32_C(5));
    put(n+0xc,u32(n+0xc)|UINT32_C(0x200));
    a[0x154]=0;a[0x155]=3;put(manager+8+0xc8,0);return 1;
}
