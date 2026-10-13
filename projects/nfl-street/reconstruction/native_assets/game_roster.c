#include "game_roster.h"
#include <stdlib.h>
#include <string.h>
#define TAG_DIGP 0x44494750u
#define TAG_DIGT 0x44494754u
#define TAG_PYTT 0x50595454u
static uint32_t rd32(const uint8_t *p){return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static uint16_t rd16(const uint8_t *p){return(uint16_t)((unsigned)p[0]<<8|p[1]);}
int nflstreet_db_schema_open(const uint8_t *b,size_t n,uint32_t tag,nflstreet_team_table *out){
 if(!b||!out||n<28||memcmp(b,"DB\0\x08",4)||rd32(b+8)!=n)return 0;uint32_t count=rd32(b+16);if(!count||count>64||28u+(size_t)count*8u>n)return 0;
 for(unsigned i=0;i<count;++i)if(rd32(b+24+i*8)==tag){size_t at=28u+(size_t)count*8u+rd32(b+28+i*8);if(at>n||n-at<36)return 0;
  nflstreet_team_table t={0};t.bytes=b;t.size=n;t.record_bytes=rd32(b+at+4);t.record_bits=rd32(b+at+8);t.allocated=rd16(b+at+16);
  t.field_count=b[at+24];t.fields_at=at+36;
  if(!t.record_bytes||t.record_bytes>4096||t.record_bits>t.record_bytes*8u||!t.field_count||(size_t)t.field_count*16u>n-t.fields_at)return 0;
  *out=t;return 1;}
 return 0;
}
static int field_index(const nflstreet_team_table *t,uint32_t tag,unsigned *index){
 unsigned found=0;for(unsigned i=0;i<t->field_count;++i){nflstreet_team_field f;if(!nflstreet_team_table_field(t,i,&f))return 0;
  if(f.tag==tag){if(found++)return 0;*index=i;}}
 return found==1;
}
static int number(const nflstreet_team_table *t,unsigned row,uint32_t tag,uint32_t *v){unsigned i;return field_index(t,tag,&i)&&nflstreet_team_table_number(t,row,i,v);}
static uint32_t get_bits(const uint8_t *p,uint32_t at,uint32_t width){uint32_t v=0;
 for(uint32_t i=0;i<width;++i){uint32_t b=at+i;v=(v<<1)|((p[b>>3]>>(7u-(b&7u)))&1u);}return v;}
static void put_bits(uint8_t *p,uint32_t at,uint32_t width,uint32_t v){
 for(uint32_t i=0;i<width;++i){uint32_t b=at+width-1u-i;uint8_t bit=(uint8_t)(0x80u>>(b&7u));if((v>>i)&1u)p[b>>3]|=bit;else p[b>>3]&=(uint8_t)~bit;}}
void nflstreet_game_roster_close(nflstreet_game_roster *r){if(r){free(r->bytes);free(r->source_rows);memset(r,0,sizeof *r);}}
typedef struct {const nflstreet_team_table *t;unsigned row;uint32_t digp;} source_row;
static int by_digp(const void *a,const void *b){uint32_t x=((const source_row*)a)->digp,y=((const source_row*)b)->digp;return(x>y)-(x<y);}
static int by_value(const void *a,const void *b){uint32_t x=*(const uint32_t*)a,y=*(const uint32_t*)b;return(x>y)-(x<y);}
/* 801E7154 assigns a GAME DIGP the first time a source DIGP is seen. The
   per-team copy visits THCD before PLAY, each in ascending source DIGP. */
static int map_id(uint32_t *from,uint32_t *to,unsigned *count,unsigned capacity,uint32_t source,uint32_t *next){
 for(unsigned i=0;i<*count;++i)if(from[i]==source)return 1;
 if(*count>=capacity)return 0;from[*count]=source;to[(*count)++]=(*next)++;return 1;
}
static int lookup_id(const uint32_t *from,const uint32_t *to,unsigned count,uint32_t source,uint32_t *id){
 for(unsigned i=0;i<count;++i)if(from[i]==source){*id=to[i];return 1;}return 0;}
int nflstreet_game_roster_build(const nflstreet_team_table *play,const nflstreet_team_table *thcd,const nflstreet_team_table *teams,
 const nflstreet_team_table *schema,const uint32_t *selected,unsigned team_count,const nflstreet_stat_override *overrides,unsigned override_count,
 nflstreet_game_roster *out){
 if(!play||!thcd||!teams||!schema||!selected||!out||!team_count||team_count>NFLSTREET_GAME_TEAMS_MAX||(override_count&&!overrides))return 0;
 const size_t fields=(size_t)schema->field_count*16u,capacity=schema->allocated;nflstreet_game_roster r;memset(&r,0,sizeof r);
 unsigned source_field[256],digp_index,digt_index;
 for(unsigned i=0;i<schema->field_count;++i){nflstreet_team_field g,s;if(!nflstreet_team_table_field(schema,i,&g)||!field_index(play,g.tag,source_field+i)||
  !nflstreet_team_table_field(play,source_field[i],&s)||s.type!=g.type||s.width!=g.width||g.bit_offset+g.width>schema->record_bits)return 0;}
 if(!field_index(schema,TAG_DIGP,&digp_index)||!field_index(schema,TAG_DIGT,&digt_index))return 0;
 const unsigned id_capacity=play->used+thcd->used+1u;
 source_row *rows=malloc(sizeof *rows*(play->used?play->used:1u));uint32_t *from=malloc(sizeof *from*id_capacity),*to=malloc(sizeof *to*id_capacity),
  *ids=malloc(sizeof *ids*id_capacity),next=0;
 r.bytes=calloc(fields+capacity*schema->record_bytes+1u,1);r.source_rows=malloc(sizeof *r.source_rows*(capacity?capacity:1u));
 int ok=0;unsigned used=0,copied=0,mapped=0;
 if(!rows||!from||!to||!ids||!r.bytes||!r.source_rows)goto done;
 memcpy(r.bytes,schema->bytes+schema->fields_at,fields);
 for(unsigned u=0;u<team_count;++u){unsigned count=0,team_row=0,found=0;uint32_t v;
  for(unsigned t=0;t<teams->used;++t)if(number(teams,t,TAG_DIGT,&v)&&v==selected[u]){if(found++)goto done;team_row=t;}
  if(found!=1||!number(teams,team_row,TAG_PYTT,&v)||v==5)goto done;r.team_rows[u]=team_row;
  for(unsigned p=0;p<play->used;++p){uint32_t digp;if(!number(play,p,TAG_DIGT,&v)||!number(play,p,TAG_DIGP,&digp))goto done;
   if(v==selected[u])rows[count++]=(source_row){play,p,digp};}
  qsort(rows,count,sizeof *rows,by_digp);unsigned n=0;
  for(unsigned h=0;h<thcd->used;++h){uint32_t digp;if(!number(thcd,h,TAG_DIGT,&v)||!number(thcd,h,TAG_DIGP,&digp))goto done;if(v==selected[u])ids[n++]=digp;}
  qsort(ids,n,sizeof *ids,by_value);
  for(unsigned k=0;k<n;++k)if(!map_id(from,to,&mapped,id_capacity,ids[k],&next))goto done;
  for(unsigned k=0;k<count;++k)if(!map_id(from,to,&mapped,id_capacity,rows[k].digp,&next))goto done;
  for(unsigned k=0;k<count;++k){uint32_t id;if(!lookup_id(from,to,mapped,rows[k].digp,&id))goto done;if(used>=capacity||(k&&rows[k].digp==rows[k-1].digp))goto done;
   uint8_t *d=r.bytes+fields+(size_t)used*schema->record_bytes;
   const uint8_t *s=play->bytes+play->records_at+(size_t)rows[k].row*play->record_bytes;
   for(unsigned i=0;i<schema->field_count;++i){nflstreet_team_field g,f;nflstreet_team_table_field(schema,i,&g);nflstreet_team_table_field(play,source_field[i],&f);
    uint32_t value=0;unsigned o;
    for(o=0;o<override_count;++o)if(overrides[o].digp==rows[k].digp&&overrides[o].tag==g.tag)break;
    if(o<override_count){if(g.type!=3||g.width>32)goto done;value=overrides[o].value;++copied;}
    if(g.type==3&&g.width<=32){if(o==override_count)value=get_bits(s,f.bit_offset,f.width);
     if(i==digp_index)value=id;else if(i==digt_index)value=u;put_bits(d,g.bit_offset,g.width,value);}
    else for(uint32_t b=0;b<g.width;b+=8){uint32_t w=g.width-b<8?g.width-b:8;put_bits(d,g.bit_offset+b,w,get_bits(s,f.bit_offset+b,w));}
   }
   r.source_rows[used++]=rows[k].row;}
 }
 if(copied!=override_count)goto done;
 r.source_play=*play;
 r.table.bytes=r.bytes;r.table.size=fields+capacity*schema->record_bytes;r.table.fields_at=0;r.table.records_at=fields;
 r.table.record_bytes=schema->record_bytes;r.table.record_bits=schema->record_bits;r.table.allocated=(uint16_t)capacity;
 r.table.used=(uint16_t)used;r.table.field_count=schema->field_count;ok=1;
done:
 free(rows);free(from);free(to);free(ids);if(!ok){nflstreet_game_roster_close(&r);return 0;}
 *out=r;return 1;
}
