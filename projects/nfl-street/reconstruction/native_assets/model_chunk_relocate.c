#include "model_chunk_relocate.h"
#include <string.h>

/* Sources: original GNNE69-r0 801C5EAC (nonzero x -> x - old + new),
   registry 801C6904, handlers 801CA1D8, 801CD108, 801C63A0, 801CCAB0,
   801CD34C, 801CA334, 801CCAE4, 801CCEC0, 801CCF5C, 801CCE28 and
   Geom/Swap helpers 801C6200, 801C6150, 801CD418, 801CD43C. The original
   model caller supplies old=0 and new=asset base; this implementation uses a
   caller-owned token instead of a host pointer. All words are big endian. */
typedef struct { uint8_t *b; size_t n, s, z; uint32_t token; int write; } C;
static uint32_t w(const uint8_t *p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static unsigned h(const uint8_t *p){return (unsigned)p[0]<<8|p[1];}
static void put(uint8_t *p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
static int span(size_t n,size_t o,size_t z){return o<=n&&z<=n-o;}
static int table(size_t n,size_t o,size_t count,size_t stride){return o<=n&&count<=(n-o)/stride;}
static int field(C *c,size_t at,size_t *raw){
 uint32_t v;if(!span(c->n,at,4))return 0;v=w(c->b+at);
 if(v&&(v>=c->n||v>UINT32_MAX-c->token))return 0;
 if(raw)*raw=v;
 if(c->write&&v)put(c->b+at,v+c->token);
 return 1;
}
static int matl(C*c){
 size_t a;uint32_t count; if(!span(c->z,0,8))return 0;
 count=w(c->b+c->s);if(!field(c,c->s+4,&a)||!table(c->n,a,count,0x30))return 0;
 for(uint32_t i=0;i<count;i++){size_t r=a+(size_t)i*0x30;int16_t names=(int16_t)h(c->b+r+8);
  if(!field(c,r,0)||!field(c,r+4,0)||!field(c,r+0x2c,0))return 0;
  if(names>0){if(names>8)return 0;for(int j=0;j<names;j++)if(!field(c,r+0xc+(size_t)j*4,0))return 0;}
 }return 1;
}
static int skel(C*c,size_t s){
 if(!span(c->n,s,0x14)||!field(c,s+4,0)||!field(c,s+8,0)||!field(c,s+0xc,0))return 0;
 return !w(c->b+s+0x10)||field(c,s+0x10,0);
}
static int equal_geom(C*c,size_t a,size_t b,size_t attrs){
 unsigned n=c->b[a+0x10];if(n!=c->b[b+0x10]||((c->b[a+0x11]^c->b[b+0x11])&1))return 0;
 size_t aa=attrs+(size_t)h(c->b+a+0xe)*0xc,bb=attrs+(size_t)h(c->b+b+0xe)*0xc;
 for(unsigned i=0;i<n;i++){size_t x=aa+i*0xc,y=bb+i*0xc;
  if(c->b[x+6]!=c->b[y+6]||(w(c->b+x+8)&0xffffff00u)!=(w(c->b+y+8)&0xffffff00u)||
     (w(c->b+x+8)&0xf)!=(w(c->b+y+8)&0xf))return 0;
 }return 1;
}
static int geom(C*c){
 size_t s=c->s,draws,attrs;uint32_t nd,na,flags;if(!span(c->z,0,0x18))return 0;
 nd=w(c->b+s+4);na=w(c->b+s+0xc);flags=w(c->b+s+0x10);
 if(!field(c,s,&draws)||!field(c,s+8,&attrs)||!table(c->n,draws,nd,0x18)||!table(c->n,attrs,na,0xc))return 0;
 for(uint32_t i=0;i<nd;i++)if(!field(c,draws+(size_t)i*0x18,0))return 0;
 for(uint32_t i=0;i<na;i++)if(!field(c,attrs+(size_t)i*0xc,0))return 0;
 if(!field(c,s+0x14,0))return 0;
 if(!(flags&0x10)){
  size_t unique[32];unsigned used=0;
  for(uint32_t i=0;i<nd;i++){size_t r=draws+(size_t)i*0x18;unsigned cnt=c->b[r+0x10],index=0;
   size_t start=h(c->b+r+0xe);if(start>na||cnt>na-start)return 0;
   for(;index<used;index++)if(equal_geom(c,unique[index],r,attrs))break;
   if(index==used){if(used<32)unique[used++]=r;else index=(unsigned)-1;}
   if(c->write)c->b[r+0x12]=(uint8_t)(index+1);
  }
  if(c->write)put(c->b+s+0x10,flags|0x10);
 }return 1;
}
static int swap(C*c){
 size_t s=c->s,a,b;uint32_t count;if(!span(c->z,0,0x10))return 0;
 count=w(c->b+s);if(count>0xffff)return 0;
 if(!field(c,s+4,&a)||!field(c,s+8,&b)||!field(c,s+0xc,0)||!table(c->n,a,count,0x14))return 0;
 for(uint32_t i=0;i<count;i++){size_t r=a+(size_t)i*0x14;size_t start=h(c->b+r+0x10),n=c->b[r+0x12];
  if(start>(c->n-b)/0x14||n>(c->n-b)/0x14-start)return 0;
  for(size_t j=0;j<n;j++){size_t m=b+(start+j)*0x14;
   if(c->write)put(c->b+m+0xc,h(c->b+m+0x12));
  }
 }return 1;
}
static int txfm(C*c){
 size_t s=c->s;uint32_t count;if(!span(c->z,0,4))return 0;count=w(c->b+s);
 if(!table(c->z,4,count,0x18))return 0;
 for(uint32_t i=0;i<count;i++)if(!field(c,s+4+(size_t)i*0x18,0))return 0;
 return 1;
}
static int inst(C*c){
 size_t s=c->s,p,sk;if(!span(c->z,0,0x14))return 0;
 if(!field(c,s+8,0)||!field(c,s+4,0)||!field(c,s+0xc,&p)||!p||!span(c->n,p,8)||!field(c,p+4,0)||!field(c,s+0x10,&sk)||!sk)return 0;
 return skel(c,sk);
}
static int relocate_rint(C*c){
 size_t s=c->s,a;uint32_t count,tail;if(!span(c->z,0,0xc))return 0;
 count=w(c->b+s);tail=w(c->b+s+8);
 if(!field(c,s+4,&a)||!table(c->n,a,count,0x40))return 0;
 for(uint32_t i=0;i<count;i++){size_t r=a+(size_t)i*0x40;
  for(unsigned j=0;j<4;j++){size_t at=r+0x3c-j*4;
   if(w(c->b+at-0x10)&&!field(c,at,0))return 0;
  }
 }
 if(c->z/4<3||(size_t)tail>c->z/4-3)return 0;
 for(uint32_t i=0;i<tail;i++)if(!field(c,s+(size_t)(tail+2-i)*4,0))return 0;
 return 1;
}
static int dinf(C*c){
 size_t s=c->s;uint32_t a,b,na,nb;if(!span(c->z,0,0x10))return 0;
 a=w(c->b+s);b=w(c->b+s+8);
 if(!w(c->b+s+4)||!a){if(c->write)put(c->b+s,0),put(c->b+s+4,0);}
 else {if(s>UINT32_MAX-c->token||a>UINT32_MAX-c->token-(uint32_t)s)return 0;
  na=c->token+(uint32_t)s+a;if(c->write)put(c->b+s,na);}
 if(!w(c->b+s+0xc)||!b){if(c->write)put(c->b+s+8,0),put(c->b+s+0xc,0);}
 else {if(s>UINT32_MAX-c->token||b>UINT32_MAX-c->token-(uint32_t)s)return 0;
  nb=c->token+(uint32_t)s+b;if(c->write)put(c->b+s+8,nb);}
 return 1;
}
static int run(C*c,uint32_t tag){
 switch(tag){
 case 0x496e666fu:case 0x46616465u:return 1; /* Info, Fade: null callbacks */
 case 0x4d61746cu:return matl(c);case 0x536b656cu:return skel(c,c->s);
 case 0x47656f6du:return geom(c);case 0x4578746eu:return span(c->z,0,8)&&field(c,c->s+4,0);
 case 0x53776170u:return swap(c);case 0x436c7374u:return span(c->z,0,4)&&field(c,c->s,0);
 case 0x5478666du:return txfm(c);case 0x496e7374u:return inst(c);
 case 0x52696e74u:return relocate_rint(c);case 0x44496e66u:return dinf(c);
 default:return 0;
 }
}
int nflstreet_model_chunk_relocate(uint8_t *asset,size_t size,uint32_t token,uint32_t tag,size_t section_offset,size_t section_size){
 C c;if(!asset||!token||!span(size,section_offset,section_size)||size>UINT32_MAX-token)return 0;
 c.b=asset;c.n=size;c.s=section_offset;c.z=section_size;c.token=token;c.write=0;
 if(!run(&c,tag))return 0;
 c.write=1;
 return run(&c,tag);
}
