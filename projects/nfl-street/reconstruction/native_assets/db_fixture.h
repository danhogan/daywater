#ifndef NFLSTREET_DB_FIXTURE_H
#define NFLSTREET_DB_FIXTURE_H
/* Check-only helpers for reading private inputs and live database tables in a
   24 MiB RAM snapshot (fixture input, never native runtime data). */
#include "../menus/team_catalog.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define RAM_BASE 0x80000000u
#define RAM_BYTES (24u*1024u*1024u)
static inline uint8_t *load_data(const char *path,size_t *size){FILE *f=fopen(path,"rb");if(!f)return NULL;
 if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}long n=ftell(f);if(n<=0||fseek(f,0,SEEK_SET)){fclose(f);return NULL;}
 uint8_t *b=malloc((size_t)n);if(!b||fread(b,1,(size_t)n,f)!=(size_t)n){free(b);fclose(f);return NULL;}fclose(f);*size=(size_t)n;return b;}
static inline uint32_t rd32(const uint8_t *p){return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static inline uint16_t rd16(const uint8_t *p){return(uint16_t)((unsigned)p[0]<<8|p[1]);}
static inline const uint8_t *ram_at(const uint8_t *ram,uint32_t a,size_t n){return a>=RAM_BASE&&(uint64_t)a-RAM_BASE+n<=RAM_BYTES?ram+(a-RAM_BASE):NULL;}
static inline uint32_t fnv(uint32_t h,const void *p,size_t n){const uint8_t *b=p;for(size_t i=0;i<n;++i)h=(h^b[i])*16777619u;return h;}
/* Live table header (candidate layout, consistent across three tables): +0
   records, +8 tag, +C record bytes, +10 record bits, +58 allocated, +5C used,
   +62 field count, +64 field descriptors. The schema must equal the on-disc one. */
static inline int ram_table(const uint8_t *ram,uint32_t header,uint32_t tag,const nflstreet_team_table *schema,nflstreet_team_table *out){
 const uint8_t *h=ram_at(ram,header,0x64);if(!h||rd32(h+8)!=tag)return 0;
 nflstreet_team_table t={0};t.bytes=ram;t.size=RAM_BYTES;t.records_at=rd32(h)-RAM_BASE;t.fields_at=header+0x64u-RAM_BASE;
 t.record_bytes=rd32(h+12);t.record_bits=rd32(h+16);t.allocated=rd16(h+0x58);t.used=rd16(h+0x5c);t.field_count=h[0x62];
 if(rd32(h)<RAM_BASE||t.used>t.allocated||t.record_bytes!=schema->record_bytes||t.record_bits!=schema->record_bits||t.field_count!=schema->field_count||
    !ram_at(ram,rd32(h),(size_t)t.used*t.record_bytes)||memcmp(ram+t.fields_at,schema->bytes+schema->fields_at,(size_t)t.field_count*16u))return 0;
 *out=t;return 1;
}
static inline int field_of(const nflstreet_team_table *t,uint32_t tag,nflstreet_team_field *f){
 for(unsigned i=0;i<t->field_count;++i)if(nflstreet_team_table_field(t,i,f)&&f->tag==tag)return 1;return 0;}
/* Writes one numeric field into a record laid out like t's rows. */
static inline int set_record_number(uint8_t *p,const nflstreet_team_table *t,uint32_t tag,uint32_t value){
 nflstreet_team_field f;if(!field_of(t,tag,&f)||f.type!=3||f.width>32||f.bit_offset+f.width>t->record_bytes*8u)return 0;
 for(unsigned i=0;i<f.width;++i){uint32_t at=f.bit_offset+f.width-1u-i;uint8_t bit=(uint8_t)(0x80u>>(at&7u));
  if((value>>i)&1u)p[at>>3]|=bit;else p[at>>3]&=(uint8_t)~bit;}
 return 1;
}
/* Writes one numeric field of a row in a RAM image laid out like t. */
static inline int set_number(uint8_t *ram,const nflstreet_team_table *t,unsigned row,uint32_t tag,uint32_t value){
 return row<t->used&&set_record_number(ram+t->records_at+(size_t)row*t->record_bytes,t,tag,value);
}
static inline int number_of(const nflstreet_team_table *t,unsigned row,uint32_t tag,uint32_t *value){
 for(unsigned i=0;i<t->field_count;++i){nflstreet_team_field f;if(nflstreet_team_table_field(t,i,&f)&&f.tag==tag)return nflstreet_team_table_number(t,row,i,value);}
 return 0;
}
static inline int find(const nflstreet_team_table *t,uint32_t tag,uint32_t id,unsigned *row){
 for(unsigned r=0;r<t->used;++r){uint32_t v;if(number_of(t,r,tag,&v)&&v==id){*row=r;return 1;}}
 return 0;
}
#endif
