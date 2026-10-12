#include "team_catalog.h"
#include <string.h>
static uint32_t be32(const uint8_t *p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static uint16_t be16(const uint8_t *p){return (uint16_t)((unsigned)p[0]<<8|p[1]);}
int nflstreet_team_table_open_tag(const uint8_t *b,size_t n,uint32_t tag,nflstreet_team_table *out){
 if(!b||!out||n<28||memcmp(b,"DB\0\x08",4)||be32(b+8)!=n)return 0;
 uint32_t count=be32(b+16);if(!count||count>64||28u+(size_t)count*8u>n)return 0;
 size_t table=0;unsigned found=0;
 for(unsigned i=0;i<count;++i)if(be32(b+24+i*8)==tag){
  size_t offset=be32(b+28+i*8);size_t base=28u+(size_t)count*8u;
  if(offset>n-base||++found!=1)return 0;table=base+offset;
 }
 if(found!=1||n-table<36)return 0;
 nflstreet_team_table t={0};t.bytes=b;t.size=n;t.record_bytes=be32(b+table+4);t.record_bits=be32(b+table+8);
 t.allocated=be16(b+table+16);t.used=be16(b+table+18);t.field_count=b[table+24];t.fields_at=table+36;
 if(!t.record_bytes||t.record_bytes>4096||!t.record_bits||t.record_bits>(uint64_t)t.record_bytes*8u||t.used>t.allocated||!t.field_count)return 0;
 if((size_t)t.field_count*16u>n-t.fields_at)return 0;t.records_at=t.fields_at+(size_t)t.field_count*16u;
 if((size_t)t.allocated*t.record_bytes>n-t.records_at)return 0;
 for(unsigned i=0;i<t.field_count;++i){nflstreet_team_field f;if(!nflstreet_team_table_field(&t,i,&f))return 0;
  if(!f.width||f.bit_offset>t.record_bits||f.width>t.record_bits-f.bit_offset)return 0;
 }
 *out=t;return 1;
}
int nflstreet_team_table_field(const nflstreet_team_table *t,unsigned index,nflstreet_team_field *out){
 if(!t||!t->bytes||!out||index>=t->field_count||t->fields_at>t->size||((size_t)index+1u)*16u>t->size-t->fields_at)return 0;
 const uint8_t *p=t->bytes+t->fields_at+index*16u;*out=(nflstreet_team_field){be32(p),be32(p+4),be32(p+8),be32(p+12)};return 1;
}
static int row(const nflstreet_team_table *t,unsigned r,const uint8_t **out){
 if(!t||!t->bytes||r>=t->used||t->records_at>t->size||((size_t)r+1u)*t->record_bytes>t->size-t->records_at)return 0;
 *out=t->bytes+t->records_at+(size_t)r*t->record_bytes;return 1;
}
int nflstreet_team_table_number(const nflstreet_team_table *t,unsigned r,unsigned index,uint32_t *out){
 nflstreet_team_field f;const uint8_t *p;if(!out||!row(t,r,&p)||!nflstreet_team_table_field(t,index,&f)||f.type!=3||f.width>32||!f.width||f.bit_offset>(uint64_t)t->record_bytes*8||f.width>(uint64_t)t->record_bytes*8-f.bit_offset)return 0;
 uint32_t value=0;for(unsigned i=0;i<f.width;++i){uint32_t at=f.bit_offset+i;value=(value<<1)|((p[at>>3]>>(7u-(at&7u)))&1u);}*out=value;return 1;
}
int nflstreet_team_table_text(const nflstreet_team_table *t,unsigned r,unsigned index,char *out,size_t cap){
 nflstreet_team_field f;const uint8_t *p;if(!out||!row(t,r,&p)||!nflstreet_team_table_field(t,index,&f)||f.type!=0||(f.bit_offset&7)||(f.width&7)||!f.width||f.bit_offset>(uint64_t)t->record_bytes*8||f.width>(uint64_t)t->record_bytes*8-f.bit_offset)return 0;
 size_t count=f.width/8;if(cap<=count)return 0;memcpy(out,p+f.bit_offset/8,count);out[count]=0;return 1;
}

int nflstreet_team_table_open(const uint8_t*b,size_t n,nflstreet_team_table*out){
 return nflstreet_team_table_open_tag(b,n,0x4d414554u,out);
}
