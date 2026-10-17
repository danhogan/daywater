#include "venue_setup.h"
#include <string.h>
#define BASE 0x80000000u
#define CONFIG 0x802a301cu
static uint32_t rd(const uint8_t *p){return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void wr(uint8_t *p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
static void half(uint8_t *p,uint32_t v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static float real(const uint8_t *p){uint32_t b=rd(p);float f;memcpy(&f,&b,4);return f;}
static void store_float(uint8_t *p,float f){uint32_t b;memcpy(&b,&f,4);wr(p,b);}
/* Gekko fctiwz: avoid undefined host float-to-int conversion at the limits. */
static uint32_t truncate_word(float f){
 if(f!=f||f<=-2147483648.0f)return 0x80000000u;
 if(f>=2147483648.0f)return 0x7fffffffu;
 return(uint32_t)(int32_t)f;}
uint32_t nflstreet_venue_archive_base(uint32_t raw_key){
 switch(raw_key){
 case 0:return 0x4d;case 1:return 0x2c;case 2:return 0x16;case 3:return 0x0b;
 case 4:return 0x00;case 5:return 0x21;case 6:return 0x37;case 7:return 0x42;
 default:return 0x0b;}}
int nflstreet_venue_setup(uint8_t *mem1,size_t bytes,const nflstreet_venue_setup_services *s){
 if(!mem1||bytes<0x2a391eu||!s||!s->word||!s->real)return 0;
 uint8_t *p=mem1+(CONFIG-BASE);uint32_t base=nflstreet_venue_archive_base(rd(p));
 /* sth destinations and store order from 80026D00..80026D18. */
 half(p+6,base+1);half(p+12,base+4);half(p+14,base+9);half(p+16,base+10);
 half(p+8,base+2);half(p+10,base+3);half(p+4,base);
 wr(p+0x40,s->word(s->user,0x8014b588u));
 store_float(p+0x4c,(float)s->real(s->user,0x8014b5b8u));
 store_float(p+0x50,(float)s->real(s->user,0x8014b5e8u));
 /* fmuls rounds BEFORE fctiwz; do not multiply in double then truncate. */
 double value=s->real(s->user,0x8014b618u);
 float scaled=(float)(value*(double)real(mem1+(0x80245820u-BASE)));
 wr(p+0x54,truncate_word(scaled));
 store_float(p+0x58,(float)s->real(s->user,0x8014b648u));
 store_float(p+0x64,(float)s->real(s->user,0x8014b678u));
 wr(p+0x68,s->word(s->user,0x8014b6a8u));
 wr(p+0x6c,s->word(s->user,0x8014b6d8u));
 store_float(p+0x70,(float)s->real(s->user,0x8014b708u));
 switch(rd(p+0x40)){
 case 0:p[0x44]=0x1f;break;case 1:p[0x44]=0x2d;break;
 case 2:p[0x44]=0x44;break;case 3:p[0x44]=0x5f;break;
 default:break; /* original retains this byte for other raw values */}
 uint32_t overlay=0;
 /* fcmpu + cror SO,EQ,LT + bso: unordered follows the greater branch. */
 if(!(real(p+0x4c)<=real(mem1+(0x80245824u-BASE))))overlay=rd(p+0x40)==0?1u:2u;
 wr(p+0x48,overlay);
 p[0x901]=0;
 wr(p+0x5c,rd(p+0x54));store_float(p+0x60,real(p+0x58));
 p[0x900]=0;
 return 1;}
