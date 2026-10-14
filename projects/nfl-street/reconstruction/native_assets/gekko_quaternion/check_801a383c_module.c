#include "quaternion_kernels.h"
#include "moderngekko/module_abi.h"
#include <dlfcn.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define BASE 0x80000000u
#define BYTES (24u*1024u*1024u)
#define STACK 0x80f47000u
#define A 0x80f37f00u
#define B 0x80f37f10u
#define OUT 0x80f37f20u
static uint32_t be(const uint8_t*p){return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void put(uint8_t*p,uint32_t w){p[0]=w>>24;p[1]=w>>16;p[2]=w>>8;p[3]=w;}
static uint32_t bits(float f){uint32_t w;memcpy(&w,&f,4);return w;}
static uint32_t rnd(uint32_t*s){*s^=*s<<13;*s^=*s>>17;*s^=*s<<5;return*s;}
static int call(const ModernGekkoModuleDesc*m,uint8_t*ram,float t){const uint32_t ret=0x81234560;CPUState c;memset(&c,0,sizeof c);c.ram=ram;c.ram_size=BYTES;c.gpr[1]=STACK;c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;c.gpr[3]=OUT;c.gpr[4]=A;c.gpr[5]=B;c.gpr[6]=B;c.fpr[1]=t;c.ps1[1]=t;c.lr=ret;c.pc=0x801a383c;c.msr=0x2000;c.hid2=0xe0000000;c.gqr[6]=0x0f070007;c.downcount=-1000000000;for(unsigned i=0;c.pc!=ret&&i<10000;++i)if(m->dispatch(&c,c.pc)!=1||c.exception)return 0;return c.pc==ret;}
static uint32_t unary(const ModernGekkoModuleDesc*m,uint8_t*ram,uint32_t pc,float x){const uint32_t ret=0x81234560;CPUState c;memset(&c,0,sizeof c);c.ram=ram;c.ram_size=BYTES;c.gpr[1]=STACK;c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;c.fpr[1]=x;c.ps1[1]=x;c.lr=ret;c.pc=pc;c.msr=0x2000;c.hid2=0xe0000000;c.gqr[6]=0x0f070007;c.downcount=-1000000000;for(unsigned i=0;c.pc!=ret&&i<10000;++i)if(m->dispatch(&c,c.pc)!=1||c.exception)return 0xffffffffu;return bits((float)c.fpr[1]);}
static uint32_t binary(const ModernGekkoModuleDesc*m,uint8_t*ram,uint32_t pc,float x,float y){const uint32_t ret=0x81234560;CPUState c;memset(&c,0,sizeof c);c.ram=ram;c.ram_size=BYTES;c.gpr[1]=STACK;c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;c.fpr[1]=x;c.ps1[1]=x;c.fpr[2]=y;c.ps1[2]=y;c.lr=ret;c.pc=pc;c.msr=0x2000;c.hid2=0xe0000000;c.gqr[6]=0x0f070007;c.downcount=-1000000000;for(unsigned i=0;c.pc!=ret&&i<10000;++i)if(m->dispatch(&c,c.pc)!=1||c.exception)return 0xffffffffu;return bits((float)c.fpr[1]);}
static uint32_t normdot(const ModernGekkoModuleDesc*m,uint8_t*ram){const uint32_t ret=0x81234560;CPUState c;memset(&c,0,sizeof c);c.ram=ram;c.ram_size=BYTES;c.gpr[1]=STACK;c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;c.gpr[3]=B;c.gpr[4]=A;c.lr=ret;c.pc=0x801a2fa8;c.msr=0x2000;c.hid2=0xe0000000;c.gqr[6]=0x0f070007;c.downcount=-1000000000;for(unsigned i=0;c.pc!=ret&&i<10000;++i)if(m->dispatch(&c,c.pc)!=1||c.exception)return 0xffffffffu;return bits((float)c.fpr[1]);}
static int reduce(const ModernGekkoModuleDesc*m,uint8_t*ram,float x,uint32_t*y0,uint32_t*y1,uint32_t*n){const uint32_t ret=0x81234560,at=0x80f37f40;CPUState c;memset(&c,0,sizeof c);c.ram=ram;c.ram_size=BYTES;c.gpr[1]=STACK;c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;c.gpr[3]=at;c.fpr[1]=x;c.ps1[1]=x;c.lr=ret;c.pc=0x802171c4;c.msr=0x2000;c.hid2=0xe0000000;c.gqr[6]=0x0f070007;c.downcount=-1000000000;for(unsigned i=0;c.pc!=ret&&i<10000;++i)if(m->dispatch(&c,c.pc)!=1||c.exception)return 0;*n=c.gpr[3];*y0=be(ram+at-BASE);*y1=be(ram+at-BASE+4);return 1;}
int main(int argc, char **argv)
{
    if (argc != 3) return 2;
    FILE *f = fopen(argv[1], "rb");
    uint8_t *ram = malloc(BYTES);
    if (!f || !ram || fread(ram, 1, BYTES, f) != BYTES) return 2;
    fclose(f);
    void *l = dlopen(argv[2], RTLD_NOW | RTLD_LOCAL);
    ModernGekkoGetModuleFn get = l ? (ModernGekkoGetModuleFn)dlsym(l, MODERNGEKKO_GET_MODULE_SYMBOL) : 0;
    const ModernGekkoModuleDesc *m = get ? get() : 0;
    if (!m) return 2;
    for (unsigned i = 0, q = 0x6d2b79f5u; i < 500000; ++i) {
        q = rnd(&q);
        const uint32_t word = 0x3f490fd8u + q % (0x3fc90fdbu - 0x3f490fd8u + 1u);
        float x;
        memcpy(&x, &word, sizeof x);
        const uint32_t original = unary(m, ram, 0x80216b94u, x);
        const uint32_t native = bits(nflstreet_80216b94_sine(x));
        if (original != native) {
            fprintf(stderr, "80216B94 mismatch input=%08x original=%08x native=%08x\\n", word, original, native);
            return 1;
        }
    }
    for (unsigned i = 0, q = 0x9e3779b9u; i < 500000; ++i) {
        q = rnd(&q);
        const uint32_t word = 0x00800000u + q % (0x3e800001u - 0x00800000u);
        float x;
        memcpy(&x, &word, sizeof x);
        const uint32_t original = unary(m, ram, 0x802165c8u, x);
        const uint32_t native = bits(nflstreet_802165c8_sqrt(x));
        if (original != native) {
            fprintf(stderr, "802165C8 mismatch input=%08x original=%08x native=%08x\\n", word, original, native);
            return 1;
        }
    }
    for (unsigned i = 0, q = 0x12345678u; i < 500000; ++i) {
        q = rnd(&q);
        const uint32_t word = q % 0x3f800001u;
        float x;
        memcpy(&x, &word, sizeof x);
        const uint32_t original = unary(m, ram, 0x80215844u, x);
        const uint32_t native = bits(nflstreet_80215844_acos(x));
        if (original != native) {
            const float z=(1.0f-x)*0.5f;
            fprintf(stderr, "80215844 mismatch input=%08x original=%08x native=%08x sqrt=%08x/%08x\\n", word, original, native,unary(m,ram,0x802165c8u,z),bits(nflstreet_802165c8_sqrt(z)));
            return 1;
        }
    }
    uint32_t s = 0x801a383c;
    for (unsigned n = 0; n < 10000; ++n) {
        float a[4], b[4], out[4], t = (float)(rnd(&s) & 0xffff) / 65535.0f;
        for (unsigned i = 0; i < 4; ++i) {
            a[i] = (float)(int16_t)rnd(&s) * 0x1p-14f;
            b[i] = (float)(int16_t)rnd(&s) * 0x1p-14f;
            put(ram + A - BASE + 4 * i, bits(a[i]));
            put(ram + B - BASE + 4 * i, bits(b[i]));
        }
        nflstreet_801a383c_slerp(t, a, b, out);
        if (!call(m, ram, t)) return 3;
        for (unsigned i = 0; i < 4; ++i) if (be(ram + OUT - BASE + 4 * i) != bits(out[i])) {
            const float dot = fmaf(a[3], b[3], fmaf(a[2], b[2], fmaf(a[0], b[0], a[1] * b[1])));
            const float al = fmaf(a[3], a[3], fmaf(a[2], a[2], fmaf(a[0], a[0], a[1] * a[1])));
            const float bl = fmaf(b[3], b[3], fmaf(b[2], b[2], fmaf(b[0], b[0], b[1] * b[1])));
            float co = dot * ((1.0f / sqrtf(al)) * (1.0f / sqrtf(bl)));
            if (!(co >= 0.0f)) co = -co;
            const float theta = acosf(co);
            uint32_t y0 = 0, y1 = 0, qn = 0;
            reduce(m, ram, theta, &y0, &y1, &qn);
            const float mtheta=nflstreet_80215844_acos(co),inv=1.0f/nflstreet_80216b94_sine(mtheta),nwa0=mtheta*t,nwb0=mtheta*(1.0f-t),wa=nflstreet_80216b94_sine(nwa0)*inv,wb=nflstreet_80216b94_sine(nwb0)*inv;uint32_t oth=unary(m,ram,0x80188d08u,co);float otheta,osin;memcpy(&otheta,&oth,4);uint32_t osi=unary(m,ram,0x80188accu,otheta);memcpy(&osin,&osi,4);const float oinv=1.0f/osin;const float owa0=otheta*t,owb0=otheta*(1.0f-t);float ow0s,ow1s;uint32_t ow0b=unary(m,ram,0x80188accu,owa0),ow1b=unary(m,ram,0x80188accu,owb0);memcpy(&ow0s,&ow0b,4);memcpy(&ow1s,&ow1b,4);const float owa=ow0s*oinv,owb=ow1s*oinv;float fy0,fy1;memcpy(&fy0,&y0,4);memcpy(&fy1,&y1,4);fprintf(stderr, "case=%u lane=%u t=%08x a=%08x,%08x,%08x,%08x b=%08x,%08x,%08x,%08x co native/original=%08x/%08x acos original/host/msl=%08x/%08x/%08x args native/manual=%08x,%08x/%08x,%08x weights native/manual=%08x,%08x/%08x,%08x original=%08x native=%08x sin original/host/msl=%08x/%08x/%08x reduce=%u,%08x,%08x cos original=%08x\\n",
                n,i,bits(t),bits(a[0]),bits(a[1]),bits(a[2]),bits(a[3]),bits(b[0]),bits(b[1]),bits(b[2]),bits(b[3]),bits(co),normdot(m,ram),oth,bits(theta),bits(mtheta),bits(nwa0),bits(nwb0),bits(owa0),bits(owb0),bits(wa),bits(wb),bits(owa),bits(owb),be(ram + OUT - BASE + 4 * i), bits(out[i]),
                unary(m, ram, 0x80188accu, theta), bits(sinf(theta)),
                bits(nflstreet_80216b94_sine(theta)), qn, y0, y1,binary(m,ram,0x80216e0cu,fy0,fy1));
            return 1;
        }
    }
    puts("801A383C host candidate exact.");
    return 0;
}
