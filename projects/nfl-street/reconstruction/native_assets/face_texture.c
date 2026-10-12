#include "face_texture.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint16_t half(const uint8_t *p){return(uint16_t)((unsigned)p[0]<<8|p[1]);}
static uint32_t word(const uint8_t *p){return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void put(uint8_t *p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
static int span(size_t n,size_t at,size_t length){return at<=n&&length<=n-at;}
int nflstreet_face_texture_plan_make(const nflstreet_archive *archive,const char *name,nflstreet_face_texture_plan *out){
 if(!archive||!name||!out)return 0;nflstreet_face_texture_plan p={0};
 /* Original formatter limit37 includes its trailing null. */
 if(snprintf(p.key,37,"PLATEX_FACE_%s",name)<0)return 0;
 char *space=strchr(p.key,' ');if(space)*space=0;
 unsigned index;p.index=-1;
 if(nflstreet_archive_find(archive,p.key,&index))p.index=(int32_t)index;
 else{p.fallback=1;strcpy(p.key,"PLATEX_TEMPLATE_FACE");if(nflstreet_archive_find(archive,p.key,&index))p.index=(int32_t)index;}
 *out=p;return 1;
}
int nflstreet_texture_request(uint8_t *record,size_t size,int32_t index,uint32_t palette,uint32_t force){
 if(!record||size<24)return 0;if(index==-1)return 1;
 if(!force&&word(record)==(uint32_t)index&&word(record+16)==palette)return 1;
 put(record,(uint32_t)index);put(record+16,palette);record[12]=1;record[13]=0;return 1;
}
int nflstreet_mmap_relocate(uint8_t *b,size_t n,uint32_t token){
 if(!b||n<40||!token||n>UINT32_MAX-token||memcmp(b,"MMAP",4)||half(b+4)!=2)return 0;
 if(half(b+6)&1)return 1;
 unsigned images=half(b+14),palettes=half(b+16);size_t rows=word(b+20),im=word(b+24),pa=word(b+28),names=word(b+32),extra=word(b+36);
 if(!span(n,rows,(size_t)half(b+12)*12)||!span(n,im,(size_t)images*16)||
    !span(n,pa,(size_t)palettes*12)||!span(n,names,(size_t)half(b+12)*16)||
    (extra&&(!span(n,extra,8)||!span(n,word(b+extra+4),0))))return 0;
 for(unsigned i=0;i<images;++i){uint8_t *r=b+im+i*16;if(word(r+8)&&!span(n,word(r+12),word(r+8)))return 0;}
 for(unsigned i=0;i<palettes;++i){uint8_t *r=b+pa+i*12;if(!span(n,word(r+8),word(r+4)))return 0;}
 put(b+20,(uint32_t)rows+token);put(b+24,(uint32_t)im+token);put(b+28,(uint32_t)pa+token);
 for(unsigned i=0;i<images;++i){uint8_t *r=b+im+i*16;put(r+12,word(r+8)?word(r+12)+token:0);}
 for(unsigned i=0;i<palettes;++i){uint8_t *r=b+pa+i*12;put(r+8,word(r+8)+token);}
 put(b+32,(uint32_t)names+token);
 if(extra){put(b+36,(uint32_t)extra+token);put(b+extra+4,word(b+extra+4)+token);}
 b[7]|=1;return 1;
}
void nflstreet_face_texture_resource_close(nflstreet_face_texture_resource *r){
 if(r){free(r->raw);free(r->processed);nflstreet_texture_close(&r->inspection_plane);memset(r,0,sizeof *r);}
}
int nflstreet_face_texture_resource_open(const nflstreet_archive *archive,const char *name,uint32_t token,
 nflstreet_face_texture_resource *out){
 if(!out)return 0;nflstreet_face_texture_resource r={0};nflstreet_archive_entry e;r.token=token;
 if(!nflstreet_face_texture_plan_make(archive,name,&r.plan)||r.plan.index<0||
    !nflstreet_archive_get(archive,(unsigned)r.plan.index,&e)||!nflstreet_archive_decode(&e,&r.raw,&r.size))goto fail;
 r.processed=malloc(r.size);if(!r.processed)goto fail;memcpy(r.processed,r.raw,r.size);
 if(!nflstreet_mmap_relocate(r.processed,r.size,token)||!nflstreet_texture_open(r.raw,r.size,&r.inspection_plane))goto fail;
 *out=r;return 1;
fail:nflstreet_face_texture_resource_close(&r);return 0;
}
