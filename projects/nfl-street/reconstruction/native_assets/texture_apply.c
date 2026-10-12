#include "texture_apply.h"
#include <stdlib.h>
#include <string.h>
static uint16_t half(const uint8_t *p){return(uint16_t)((unsigned)p[0]<<8|p[1]);}
static uint32_t word(const uint8_t *p){return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static const uint8_t *resolve(const uint8_t *b,size_t n,uint32_t token,uint32_t pointer,size_t length){
 if(pointer<token)return NULL;size_t at=(size_t)(pointer-token);
 return at<=n&&length<=n-at?b+at:NULL;
}
typedef struct {uint8_t *destination;const uint8_t *source;uint32_t token,size;} copy_plan;
int nflstreet_texture_apply(uint8_t *d,size_t dn,uint32_t dt,const uint8_t *s,size_t sn,uint32_t st,
 nflstreet_texture_flush flush,void *opaque){
 if(!d||!s||!flush||dn<40||sn<40||!dt||!st||dn>UINT32_MAX-dt||sn>UINT32_MAX-st||
    memcmp(d,"MMAP",4)||memcmp(s,"MMAP",4)||half(d+4)!=2||half(s+4)!=2||
    !(half(d+6)&1)||!(half(s+6)&1)||!half(d+12)||!half(s+12))return 0;
 const uint8_t *dr=resolve(d,dn,dt,word(d+20),12),*sr=resolve(s,sn,st,word(s+20),12);
 if(!dr||!sr)return 0;unsigned count=half(dr+2),di=half(dr+4),si=half(sr+4);
 copy_plan *plan=calloc((size_t)count+1,sizeof *plan);if(!plan)return 0;
 for(unsigned i=0;i<=count;++i){const uint8_t *dest,*source;unsigned did,sid;uint32_t dp,sp,size;
  if(i<count){did=(di+i)&0xffffu;sid=(si+i)&0xffffu;
   if(did>=half(d+14)||sid>=half(s+14))goto fail;
   const uint8_t *da=resolve(d,dn,dt,word(d+24),(size_t)half(d+14)*16);
   const uint8_t *sa=resolve(s,sn,st,word(s+24),(size_t)half(s+14)*16);if(!da||!sa)goto fail;
   dest=da+did*16;source=sa+sid*16;size=word(dest+8);dp=word(dest+12);sp=word(source+12);
  }else{did=half(dr+8);sid=half(sr+8);if(did>=half(d+16)||sid>=half(s+16))goto fail;
   const uint8_t *da=resolve(d,dn,dt,word(d+28),(size_t)half(d+16)*12);
   const uint8_t *sa=resolve(s,sn,st,word(s+28),(size_t)half(s+16)*12);if(!da||!sa)goto fail;
   dest=da+did*12;source=sa+sid*12;size=word(dest+4);dp=word(dest+8);sp=word(source+8);
  }
  plan[i].destination=(uint8_t*)resolve(d,dn,dt,dp,size);plan[i].source=resolve(s,sn,st,sp,size);
  if(!plan[i].destination||!plan[i].source)goto fail;plan[i].token=dp;plan[i].size=size;
 }
 for(unsigned i=0;i<=count;++i){memmove(plan[i].destination,plan[i].source,plan[i].size);
  if(!flush(opaque,plan[i].token,plan[i].size))goto fail;}
 free(plan);return 1;
fail:free(plan);return 0;
}
void nflstreet_applied_face_texture_close(nflstreet_applied_face_texture *r){if(r){free(r->bytes);memset(r,0,sizeof *r);}}
int nflstreet_face_texture_apply_template(const nflstreet_archive *archive,const nflstreet_face_texture_resource *source,
 uint32_t token,nflstreet_texture_flush flush,void *opaque,nflstreet_applied_face_texture *out){
 if(!archive||!source||!out)return 0;nflstreet_applied_face_texture r={0};nflstreet_archive_entry e;r.token=token;
 if(!nflstreet_archive_find(archive,"PLATEX_TEMPLATE_FACE",&r.template_index)||
    !nflstreet_archive_get(archive,r.template_index,&e)||!nflstreet_archive_decode(&e,&r.bytes,&r.size)||
    !nflstreet_mmap_relocate(r.bytes,r.size,r.token)||
    !nflstreet_texture_apply(r.bytes,r.size,r.token,source->processed,source->size,source->token,flush,opaque)){
  nflstreet_applied_face_texture_close(&r);return 0;}
 *out=r;return 1;
}
