#include "texture_apply.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct {uint32_t token[64],size[64];unsigned count;} flush_trace;
static unsigned application_cases,application_flushes;
static int trace_flush(void *opaque,uint32_t token,uint32_t size){flush_trace *t=opaque;if(t->count>=64)return 0;t->token[t->count]=token;t->size[t->count++]=size;return 1;}
#ifdef NFLSTREET_FACE_TEXTURE_ORACLE
static int original_application(uint32_t d,uint32_t s,flush_trace *trace){
 CPUState c={0};c.ram=texture_ram;c.ram_size=RAM_SIZE;c.msr=0x2000;c.gpr[1]=0x81700000u;
 c.gpr[2]=0x803b9a40u;c.gpr[13]=0x803a9a40u;c.gpr[3]=d;c.gpr[4]=s;c.pc=0x80164ce4u;
 for(unsigned step=0;c.pc&&step<2000000;++step){
  if(c.pc==0x801f0bd0u){if(!trace_flush(trace,c.gpr[3],c.gpr[4]))return 0;c.pc=c.lr;continue;}
  c.downcount=-1000000000;texture_module->dispatch(&c,c.pc);if(c.exception)return 0;
 }return !c.pc;
}
#endif
static int check_texture_application(const nflstreet_archive *archive,const nflstreet_face_texture_resource *source){
 nflstreet_applied_face_texture destination={0};flush_trace native={0};
 if(!nflstreet_face_texture_apply_template(archive,source,0x81200000u,trace_flush,&native,&destination))return 0;
#ifdef NFLSTREET_FACE_TEXTURE_ORACLE
 nflstreet_archive_entry entry;uint8_t *raw=NULL;size_t size=0;flush_trace original={0};
 if(!nflstreet_archive_get(archive,destination.template_index,&entry)||!nflstreet_archive_decode(&entry,&raw,&size)||
    size!=destination.size||!ptr(texture_ram,destination.token,size)||!ptr(texture_ram,source->token,source->size))return 0;
 memcpy(ptr(texture_ram,destination.token,size),raw,size);free(raw);
 memcpy(ptr(texture_ram,source->token,source->size),source->processed,source->size);
 if(!original_mmap_call(destination.token)||!original_application(destination.token,source->token,&original)||
    memcmp(destination.bytes,ptr(texture_ram,destination.token,size),size)||memcmp(&native,&original,sizeof native))return 0;
#endif
 application_flushes+=native.count;memset(&native,0,sizeof native);
 if(!nflstreet_texture_apply(destination.bytes,destination.size,destination.token,source->processed,source->size,source->token,trace_flush,&native))return 0;
#ifdef NFLSTREET_FACE_TEXTURE_ORACLE
 memset(&original,0,sizeof original);
 if(!original_application(destination.token,source->token,&original)||
    memcmp(destination.bytes,ptr(texture_ram,destination.token,size),size)||memcmp(&native,&original,sizeof native))return 0;
#endif
 application_flushes+=native.count;++application_cases;nflstreet_applied_face_texture_close(&destination);return 1;
}
