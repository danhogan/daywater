#include "command_timing.h"
#include "event_signals.h"
#include <math.h>
#include <string.h>

static uint16_t u16(const uint8_t *p) { return (uint16_t)((unsigned)p[0]<<8 | p[1]); }
static uint32_t u32(const uint8_t *p)
{ return (uint32_t)p[0]<<24 | (uint32_t)p[1]<<16 | (uint32_t)p[2]<<8 | p[3]; }
static void put16(uint8_t *p, uint16_t v) { p[0]=(uint8_t)(v>>8); p[1]=(uint8_t)v; }
static float real(const uint8_t *p) { uint32_t v=u32(p); float f; memcpy(&f,&v,4); return f; }
static void putreal(uint8_t *p, float f)
{ uint32_t v; memcpy(&v,&f,4); p[0]=(uint8_t)(v>>24); p[1]=(uint8_t)(v>>16); p[2]=(uint8_t)(v>>8); p[3]=(uint8_t)v; }
static int word(uint8_t *s, const uint8_t *p, size_t size, uint16_t *out)
{
    size_t cursor=u16(s+0x18), offset=4+cursor*2;
    if (offset+2>size || cursor==UINT16_MAX) return 0;
    *out=u16(p+offset); put16(s+0x18,(uint16_t)(cursor+1)); return 1;
}
static int argument(uint8_t *s, const uint8_t *p, size_t size, uint32_t *out)
{
    uint16_t w, next;
    if (!word(s,p,size,&w)) return 0;
    *out=w&0xfff;
    if (w&0x800) *out|=0xf000;
    if (w&0x1000) {
        if (!word(s,p,size,&next)) return 0;
        *out=(*out<<16)+next;
    }
    return 1;
}
static int advance(uint8_t *s, const uint8_t *p, size_t size, float delta,
                   uint32_t *flags, uint16_t *commands, size_t *count, uint16_t *tag)
{
    unsigned steps;
    *tag=UINT16_MAX;
    for (steps=0; steps<512; ++steps) {
        uint16_t w;
        if (!word(s,p,size,&w)) return 0;
        if (!(w&0x8000)) {
            *tag=w&0x7fff;
            put16(s+0x1c,(uint16_t)(u16(s+0x1c)+1));
            break;
        }
        if (w&0x4000) {
            if (*count==64 || !nflstreet_event_flag_command(w,flags)) return 0;
            commands[(*count)++]=w;
        } else if (w==0x8000 || w==0x8001) {
            if (u16(s+0x1a)==UINT16_MAX) put16(s+0x1a,u16(s+0x1c));
            if (w==0x8000) break;
            put16(s+0x18,0);
        } else if (w==0x8004) {
            uint32_t arg; int64_t signed_arg;
            if (!argument(s,p,size,&arg)) return 0;
            signed_arg=arg>=UINT32_C(0x80000000) ? (int64_t)arg-INT64_C(0x100000000) : arg;
            putreal(s+0xc,fmaf((float)signed_arg,0.0625f,real(s+0xc)));
        } else if (w==0x8005) s[0x1e]|=9;
        else if (w==0x8006) s[0x1e]|=0x24;
        else return 0; /* Loop stack and other controls are not verified. */
    }
    if (steps==512) return 0;
    putreal(s+8,real(s+8)+delta);
    return 1;
}
int nflstreet_command_timing(uint8_t state[64], const uint8_t *program,
                            size_t program_size, double delta,
                            uint32_t *actor_flags, uint16_t commands[64],
                            size_t *command_count, unsigned *complete)
{
    uint8_t s[64]; uint16_t emitted[64]; size_t count=0;
    uint32_t flags; unsigned done=0, iterations=0;
    float period, remaining, time;
    if (!state || !program || !actor_flags || !commands || !command_count || !complete ||
        program_size<6 || !isfinite(delta) || delta<0 || delta>65536) return 0;
    memcpy(s,state,64); flags=*actor_flags;
    period=real(s+4); remaining=real(s+0xc); time=real(s+8);
    if (!isfinite(period) || period<=0 || !isfinite(remaining) || !isfinite(time) || s[0x1f]) return 0;
    remaining=(float)((double)remaining-delta); putreal(s+0xc,remaining);
    if (remaining<0) {
        /* The original keeps the pre-subtraction countdown in f1 for the
           first interpreter call, then uses the interval for catch-up calls. */
        float step=real(state+0xc);
        do {
            uint16_t tag;
            put16(s+0x10,u16(s+0x12));
            if (++iterations>512 || !advance(s,program,program_size,step,&flags,emitted,&count,&tag)) return 0;
            put16(s+0x12,tag);
            if (tag==UINT16_MAX) { done=1; put16(s+0x12,u16(s+0x10)); }
            remaining=real(s+0xc)+period; putreal(s+0xc,remaining);
            step=period;
        } while (remaining<0 && !done);
        putreal(s+8,real(s+8)+(period-remaining));
    } else putreal(s+8,(float)((double)real(s+8)+delta));
    putreal(s+0x14,done ? 1.0f : (period-remaining)/period);
    if (!isfinite(real(s+8)) || !isfinite(real(s+0xc)) || !isfinite(real(s+0x14))) return 0;
    memcpy(state,s,64); *actor_flags=flags; memcpy(commands,emitted,count*sizeof(*commands));
    *command_count=count; *complete=done; return 1;
}

int nflstreet_stream_program_lookup(const uint8_t *table, size_t table_size,
                                    uint32_t index, uint32_t *program_identity)
{
    size_t offset;
    if (!table || !program_identity || index>4095) return 0;
    offset=4+(size_t)index*4;
    if (offset+4>table_size || !u32(table+offset)) return 0;
    *program_identity=u32(table+offset); return 1;
}
int nflstreet_stream_setup(uint8_t state[64], uint32_t program_identity,
                           const uint8_t *program, size_t program_size,
                           uint32_t *flags, uint16_t commands[64],
                           size_t *command_count, unsigned *complete)
{
    uint8_t s[64]; uint16_t emitted[64],previous,next; size_t count=0;
    uint32_t f; float interval; unsigned done=0;
    if (!state || !program || program_size<6 || !flags || !commands ||
        !command_count || !complete || !program_identity) return 0;
    memcpy(s,state,64); f=*flags;
    s[0]=(uint8_t)(program_identity>>24); s[1]=(uint8_t)(program_identity>>16);
    s[2]=(uint8_t)(program_identity>>8); s[3]=(uint8_t)program_identity;
    interval=(float)program[0]*0.0625f;
    if (interval<=0) interval=1.0f;
    putreal(s+4,interval); putreal(s+8,0); putreal(s+12,interval);
    put16(s+0x18,0); put16(s+0x1a,UINT16_MAX); put16(s+0x1c,0);
    s[0x1e]=s[0x1f]=0;
    if (!advance(s,program,program_size,0,&f,emitted,&count,&previous)) return 0;
    put16(s+0x10,previous);
    if (!advance(s,program,program_size,0,&f,emitted,&count,&next)) return 0;
    if (next==UINT16_MAX) { next=previous; done=1; }
    put16(s+0x12,next); putreal(s+0x14,0);
    memcpy(state,s,64); *flags=f; memcpy(commands,emitted,count*sizeof(*commands));
    *command_count=count; *complete=done; return 1;
}

int nflstreet_stream_rebind_phase(uint8_t state[64],uint32_t identity,
                                 const uint8_t *program,size_t size,
                                 uint16_t tags,uint32_t *flags){
 if(!state||!program||!flags||!tags||tags==UINT16_MAX)return 0;
 if(size!=4+((size_t)tags+1)*2||u16(program+size-2)!=0x8001)return 0;
 for(unsigned i=0;i<tags;++i)if(u16(program+4+i*2)&0x8000)return 0;
 uint16_t old_tags=u16(state+0x1a);float old_period=real(state+4),elapsed=real(state+8);
 if(!old_tags||old_tags==UINT16_MAX||!isfinite(old_period)||old_period<=0||!isfinite(elapsed)||elapsed<0)return 0;
 float cycle=old_period*(float)old_tags;if(!isfinite(cycle)||cycle<=0)return 0;
 /* Original 0x80215CC0 takes float operands and returns float remainder. */
 float phase=fmodf(elapsed,cycle),period=(float)((double)cycle/(double)(float)tags);
 if(!isfinite(phase)||phase<0||!isfinite(period)||period<=0)return 0;
 uint8_t next[64];memcpy(next,state,64);uint32_t f=*flags;uint16_t commands[64];size_t emitted;unsigned done;
 if(!nflstreet_stream_setup(next,identity,program,size,&f,commands,&emitted,&done)||emitted||done)return 0;
 float initial=real(next+4),time=real(next+8),remaining=real(next+12);
 if(initial==0){time=remaining=0;}else{time=(float)((double)time*period)/initial;remaining=(float)((double)remaining*period)/initial;}
 putreal(next+4,period);putreal(next+8,time);putreal(next+12,remaining);put16(next+0x1a,tags);
 if(!nflstreet_command_timing(next,program,size,phase,&f,commands,&emitted,&done)||emitted||done)return 0;
 memcpy(state,next,64);*flags=f;return 1;
}
