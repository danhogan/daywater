#ifndef NFLSTREET_GEKKO_FLOAT_H
#define NFLSTREET_GEKKO_FLOAT_H
/* Gekko floating-point operations as the original module executes them
   (DolRecomp cpu.c, FPSCR rounding to nearest, NI clear): single-precision
   results are the double result rounded to single; the multiplier operand
   (frC) of fmuls/fmadds/fmsubs is first rounded to 25 bits; the fused forms
   take one double fma with the tie correction for the single rounding.
   Operands are doubles so that double-valued registers (results of
   fadd/fmul/fsub or a double load) keep their extra bits. Compile with
   -ffp-contract=off. NaN payloads follow the host for the rare NaN
   operand; callers keep inputs finite. */
#include <math.h>
#include <stdint.h>
#include <string.h>
static inline uint64_t nflstreet_gk_bits(double v){uint64_t b;memcpy(&b,&v,8);return b;}
static inline double nflstreet_gk_value(uint64_t b){double v;memcpy(&v,&b,8);return v;}
static inline double nflstreet_gk_f25(double value){
 uint64_t bits=nflstreet_gk_bits(value),fraction=bits&0x000fffffffffffffull,keep=0xfffffffff8000000ull,round=0x0000000008000000ull;
 if((bits&0x7ff0000000000000ull)==0&&fraction){unsigned shift=0;uint64_t f=fraction;while(!(f&0x8000000000000000ull)){f<<=1;++shift;}shift-=11;
  if(shift<28){keep=~((1ull<<(27-shift))-1);round>>=shift;}else{keep=~0ull;round=0;}}
 return nflstreet_gk_value((bits&keep)+(bits&round));}
static inline float nflstreet_gk_adds(double a,double b){return (float)(a+b);}
static inline float nflstreet_gk_subs(double a,double b){return (float)(a-b);}
static inline float nflstreet_gk_muls(double a,double c){return (float)(a*nflstreet_gk_f25(c));}
static inline float nflstreet_gk_divs(double a,double b){return (float)(a/b);}
static inline float nflstreet_gk_fmadds(double a,double c,double b){
 const double rc=nflstreet_gk_f25(c),result=fma(a,rc,b);const uint64_t bits=nflstreet_gk_bits(result);
 if((bits&0x1fffffffull)!=0x10000000ull)return (float)result;
 const double ap=b-result,bp=result+ap,error=fma(a,rc,ap)+(b-bp);if(error==0.0)return (float)result;
 return (float)nflstreet_gk_value((error>0.0)==(result>0.0)?bits+1:bits-1);}
static inline float nflstreet_gk_fmsubs(double a,double c,double b){return nflstreet_gk_fmadds(a,c,-b);}
/* The 0x43300000 conversion: (hi:lo as a double) - k, as fsub then frsp. */
static inline float nflstreet_gk_convert(uint32_t lo,double k){return (float)(nflstreet_gk_value(0x4330000000000000ull|lo)-k);}
/* fctiwz: truncation with the PowerPC saturation and NaN result. */
static inline int32_t nflstreet_gk_fctiwz(double v){
 if(v!=v||v<-2147483648.0)return INT32_MIN;if(v>=2147483648.0)return INT32_MAX;return (int32_t)v;}
#endif
