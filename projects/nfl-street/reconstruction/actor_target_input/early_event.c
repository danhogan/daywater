#include "early_event.h"
#include "../kinematics/kinematics.h"
#include <math.h>
#include <string.h>

static uint32_t word(const uint8_t *p)
{ return (uint32_t)p[0]<<24 | (uint32_t)p[1]<<16 | (uint32_t)p[2]<<8 | p[3]; }
static void store(uint8_t *p,uint32_t w)
{ p[0]=(uint8_t)(w>>24); p[1]=(uint8_t)(w>>16); p[2]=(uint8_t)(w>>8); p[3]=(uint8_t)w; }
static float real(const uint8_t *p) { uint32_t w=word(p); float f; memcpy(&f,&w,4); return f; }
static uint32_t bits(float f) { uint32_t w; memcpy(&w,&f,4); return w; }
static uint32_t distance(uint32_t a,uint32_t b)
{ uint32_t d=(a-b)&0xffffff; return d<=0x800000 ? d : 0x1000000-d; }
static float add(float a,float b) { volatile float f=a+b; return f; }
static float sub(float a,float b) { volatile float f=a-b; return f; }
static float mul(float a,float b) { volatile float f=a*b; return f; }

static int steer(uint8_t *control,const uint8_t *desc,const uint8_t *g,
                 const uint8_t *incoming,const uint8_t *target,const uint8_t *mode,
                 uint32_t enabled,const uint8_t *field)
{
    float dx=sub(real(incoming),real(target)),dy=sub(real(incoming+4),real(target+4));
    float length=sqrtf(fmaf(dx,dx,mul(dy,dy)));
    uint32_t angle, heading=word(g+24);
    float speed=enabled ? 1.0f : 0.75f;
    if (!isfinite(dx) || !isfinite(dy) || !isfinite(length) || length<1.5f) return 0;
    angle=nflstreet_angle_from_vector(dy,dx);
    if (enabled && !mode[1]) {
        uint32_t goal,a,b; int adjust=1;
        float x=real(g),ix=real(incoming),vx=real(incoming+40);
        float goal_x=real(field+8),goal_y=real(field+4);
        if (desc[2]!=4 || !isfinite(x) || !isfinite(ix) || !isfinite(vx) ||
            !isfinite(goal_x) || !isfinite(goal_y)) return 0;
        {
            float gx=sub(goal_x,x),gy=sub(goal_y,real(g+4));
            if (!isfinite(gx) || !isfinite(gy) || (gx==0 && gy==0)) return 0;
            goal=nflstreet_angle_from_vector(gy,gx);
        }
        a=(angle-goal)&0xffffff; b=(heading-goal)&0xffffff;
        if ((int64_t)heading-(int64_t)angle>0x38e38 ||
            (!(ix<sub(x,0.2f)) && vx>0) ||
            (!(ix>add(x,0.2f)) && vx<0)) {
            if (a<=0x755555 && b>a) { angle=goal+(a>>1); adjust=0; }
            else if (a>0x8aaaaa && b<a) { angle=goal+a+((0x1000000-a)>>1); adjust=0; }
        }
        if (adjust) {
            uint32_t difference=distance(angle,goal);
            int32_t correction=(int32_t)mul((float)difference,
                nflstreet_sine_angle(difference/2+0x400000));
            if (correction>0x38e38) {
                if ((int64_t)heading-(int64_t)angle<=0x38e37 &&
                    ((!(ix>add(x,0.2f)) && vx>0) || (!(ix<sub(x,0.2f)) && vx<0)))
                    correction=(int32_t)mul((float)correction,0.3333333432674408f);
                if (((goal-heading)&0xffffff)<=0x7fffff) angle+=(uint32_t)correction;
                else angle-=(uint32_t)correction;
            }
        }
    }
    store(control,bits(speed)); store(control+4,angle&0xffffff);
    store(control+8,angle&0xffffff); control[14]=4; return 1;
}
static int prepare(uint8_t *request,uint8_t *event,const uint8_t *mode,unsigned stage)
{
    uint32_t code=word(event+8);
    if (code==14) { request[0]=1; return 1; }
    if (code!=4 || event[6]!=2) return 0;
    event[7]=4;
    request[0]=stage==1 ? 4 : 3;
    request[1]=(uint8_t)stage;
    if (mode[0]<=1) request[2]=(uint8_t)(mode[0]+1);
    return 1;
}
int nflstreet_early_event(uint8_t event[12], uint32_t *flags,
                          uint8_t request[3], uint8_t control[16], int owns_record,
                          const uint8_t descriptor[6],const uint8_t incoming_descriptor[6],
                          const uint8_t geometry[48],const uint8_t incoming_geometry[48],
                          const uint8_t target[12],const uint8_t actor_mode[2],
                          uint32_t tracking_enabled,const uint8_t field[12],
                          uint8_t incoming_handle_tag,uint32_t incoming_408)
{
    uint8_t e[12],r[3],c[16]; uint32_t f; unsigned stage;
    if (!event || !flags || !request || !control || !descriptor || !incoming_descriptor ||
        !geometry || !incoming_geometry || !target || !actor_mode || !field || tracking_enabled>1) return 0;
    memcpy(e,event,12); memcpy(r,request,3); memcpy(c,control,16); f=*flags;
    stage=(unsigned)e[4]*256+e[5];
    if (stage>2 || (word(e+8)!=4 && word(e+8)!=14)) return 0;
    if (!owns_record) {
        if (stage || word(e+8)==14) return 0;
        return 1; /* Stage-zero record lookup returns null: no mutation. */
    }
    if (!stage) {
        stage=word(e+8)==14 ? 3 : 1;
        if (!prepare(r,e,actor_mode,stage)) return 0;
        f&=~UINT32_C(4);
    } else if (stage==1) {
        if (!(f&4) && e[6]!=3) return 1;
        if (word(e+8)!=4 || e[6]!=2 || !steer(c,descriptor,geometry,incoming_geometry,target,
                                             actor_mode,tracking_enabled,field)) return 0;
        stage=2; f&=~UINT32_C(4);
        if (!prepare(r,e,actor_mode,stage)) return 0;
    } else {
        /* A pairing descriptor enters 0x8007B73C's contact service. Its
           eligibility/metadata/position side effects remain separate. */
        if (incoming_descriptor[0]==0x1b) {
            float x=sub(real(geometry),real(incoming_geometry));
            float y=sub(real(geometry+4),real(incoming_geometry+4));
            float length=sqrtf(fmaf(x,x,mul(y,y)));
            uint32_t angle;
            if (!isfinite(length) || !isfinite(x) || !isfinite(y) || length==0) return 0;
            angle=nflstreet_angle_from_vector(y,x);
            if (length<4.0f && (distance(word(incoming_geometry+24),angle)<0x2aaaaa ||
                                 (incoming_handle_tag==1 && incoming_408))) return 0;
        }
        if (word(e+8)!=4 ||
            real(geometry+4)<real(incoming_geometry+4) ||
            !steer(c,descriptor,geometry,incoming_geometry,target,actor_mode,tracking_enabled,field)) return 0;
    }
    e[4]=(uint8_t)(stage>>8); e[5]=(uint8_t)stage;
    memcpy(event,e,12); memcpy(request,r,3); memcpy(control,c,16); *flags=f; return 1;
}
