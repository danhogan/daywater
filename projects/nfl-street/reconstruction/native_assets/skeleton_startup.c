#include "skeleton_startup.h"
#include <string.h>
static uint16_t half(const uint8_t *p){return(uint16_t)((unsigned)p[0]<<8|p[1]);}
static uint32_t word(const uint8_t *p){return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void put_half(uint8_t *p,uint16_t v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static void put(uint8_t *p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
int nflstreet_skeleton_startup_catalog_open(const uint8_t *bytes,size_t size,nflstreet_skeleton_startup_catalog *out){
 if(!bytes||!out||size!=530||memcmp(bytes,"NSC1",4)||word(bytes+4)!=1||word(bytes+8)!=384||word(bytes+12)!=114)return 0;
 nflstreet_skeleton_startup_catalog c={bytes+16,bytes+400,bytes+412,word(bytes+526)};*out=c;return 1;
}
void nflstreet_skeleton_startup_close(nflstreet_skeleton_startup *bundle){
 if(bundle){for(unsigned i=0;i<6;++i)nflstreet_skeleton_resource_close(bundle->resources+i);memset(bundle,0,sizeof *bundle);}
}
static int positions(nflstreet_skeleton_resource *r,const uint8_t *bytes,unsigned maximum){
 unsigned count=half(r->bytes+6);if(count>maximum||r->size<0x410u+(size_t)count*6)return 0;
 memcpy(r->bytes+0x410,bytes,(size_t)count*6);return 1;
}
int nflstreet_skeleton_startup_open(const nflstreet_archive *archive,const nflstreet_skeleton_startup_catalog *catalog,
 uint32_t identity,nflstreet_skeleton_startup *out){
 if(!archive||!catalog||!out||!identity||!catalog->base_positions||!catalog->hand_positions||!catalog->hair_positions)return 0;
 nflstreet_skeleton_startup bundle={0};const unsigned indices[]={95,96,97,99,100,101};uint64_t cursor=identity;
 for(unsigned i=0;i<6;++i){
  if(cursor>UINT32_MAX||!nflstreet_skeleton_resource_open(archive,indices[i],(uint32_t)cursor,bundle.resources+i))goto fail;
  cursor+=((uint64_t)bundle.resources[i].size+31u)&~(uint64_t)31u;
 }
 nflstreet_skeleton_resource *r=bundle.resources;
 if(!positions(r,catalog->base_positions,64)||r[0].size<0x494)goto fail;
 const unsigned offsets[]={0x46a,0x46c,0x46e,0x48e,0x490,0x492};
 for(unsigned i=0;i<6;++i)bundle.base_vectors[i]=(uint32_t)((int32_t)(int16_t)half(r[0].bytes+offsets[i])*16);
 for(unsigned i=1;i<6;++i){if(r[i].size<28)goto fail;
  for(unsigned j=0;j<3;++j)put(r[i].bytes+0x10+4*j,catalog->scale_word);}
 for(unsigned i=0;i<2;++i){if(r[3+i].size<0x41c)goto fail;
  memcpy(r[3+i].bytes+0x416,catalog->hand_positions+i*6,6);}
 unsigned count=half(r[1].bytes+6);
 if(r[1].size<0x410u+(size_t)count*6||r[2].size<0x410u+(size_t)count*6)goto fail;
 for(unsigned i=0;i<count;++i){uint8_t *dst=r[1].bytes+0x410+i*6;const uint8_t *src=r[2].bytes+0x410+i*6;
  memcpy(dst,src,2);put_half(dst+2,(uint16_t)(0u-half(src+2)));put_half(dst+4,(uint16_t)(0u-half(src+4)));}
 if(!positions(r+5,catalog->hair_positions,19))goto fail;
 *out=bundle;return 1;
fail:nflstreet_skeleton_startup_close(&bundle);return 0;
}
