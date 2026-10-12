#include "palette_recolor.h"
#include <stdlib.h>
#include <string.h>
static uint32_t word(const uint8_t *p){return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static uint16_t half(const uint8_t *p){return(uint16_t)((unsigned)p[0]<<8|p[1]);}
static void put16(uint8_t *p,unsigned v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
int nflstreet_color_table_build(const uint8_t *db,size_t n,const nflstreet_gear_texture_catalog *c,nflstreet_color_table *out){
 static const uint32_t tables[2]={0x4c415043u,0x52485043u};static const unsigned offsets[12]={0,1,2,4,5,6,8,9,10,12,13,14};
 if(!db||!c||!out)return 0;nflstreet_color_table t;memset(&t,0,sizeof t);
 for(unsigned k=0;k<2;++k){nflstreet_team_table table;unsigned field[13];
  if(!nflstreet_team_table_open_tag(db,n,tables[k],&table))return 0;
  for(unsigned j=0;j<13;++j){unsigned found=0;uint32_t tag=word(c->color_tags+4*j);
   for(unsigned i=0;i<table.field_count;++i){nflstreet_team_field f;if(!nflstreet_team_table_field(&table,i,&f))return 0;if(f.tag==tag){++found;field[j]=i;}}
   if(found!=1)return 0;}
  for(unsigned row=0;row<table.used;++row){uint32_t v[13];
   for(unsigned j=0;j<13;++j)if(!nflstreet_team_table_number(&table,row,field[j],v+j))return 0;
   unsigned at=k?((v[0]&0xffu)+0x80u)*16u&0xff0u:(v[0]&0xffu)*16u;
   for(unsigned j=0;j<12;++j)t.bytes[at+offsets[j]]=(uint8_t)v[1+j];}
 }
 *out=t;return 1;
}
/* 801CE1E8 into a 15-byte buffer: at most 14 characters. */
static int attribute(const char *tag,const char *end,const char *name,char value[15]){
 size_t len=strlen(name);
 for(const char *p=tag;p+len+2<=end;++p){
  if((p==tag||p[-1]==' '||p[-1]=='\t'||p[-1]=='\r'||p[-1]=='\n')&&!memcmp(p,name,len)){
   const char *q=p+len;while(q<end&&(*q==' '||*q=='\t'))++q;if(q>=end||*q!='=')continue;++q;
   while(q<end&&(*q==' '||*q=='\t'))++q;if(q>=end||*q!='"')continue;++q;
   size_t i=0;while(q<end&&*q!='"'){if(i<14)value[i++]=*q;++q;}if(q>=end)return 0;value[i]=0;return 1;}
 }
 return 0;
}
/* 8017CFE8 */
static int decimal(const char *s){int sign=0,v=0;while(*s==' '||*s=='\t'||*s=='\r'||*s=='\n')++s;
 if(*s=='+'||*s=='-')sign=*s++=='-';while(*s>='0'&&*s<='9')v=v*10+(*s++-'0');return sign?-v:v;}
/* 8017CEC0 with limit 15: ASCII case-insensitive. */
static int same(const char *a,const char *b){for(unsigned i=0;i<15;++i){unsigned x=(unsigned char)a[i],y=(unsigned char)b[i];
 if(x>='A'&&x<='Z')x+=32;if(y>='A'&&y<='Z')y+=32;if(x!=y)return 0;if(!x)return 1;}return 1;}
static uint8_t number(const char *tag,const char *end,const char *name,uint8_t fallback){
 char v[15];return attribute(tag,end,name,v)?(uint8_t)decimal(v):fallback;}
static uint8_t lookup_name(const char *tag,const char *end,const char *name,const uint8_t *names,unsigned count,uint8_t missing){
 char v[15];if(!attribute(tag,end,name,v)||!v[0])return missing;
 for(unsigned i=0;i<count;++i)if(same(v,(const char*)names+15*i))return(uint8_t)i;return missing;}
static int parse_op(const char *tag,const char *end,const nflstreet_gear_texture_catalog *c,nflstreet_dynclut_op *op){
 nflstreet_dynclut_op o={0};char v[15];
 o.color=number(tag,end,"colornum",0xff);if(o.color==0xff)o.color=lookup_name(tag,end,"colorname",c->color_names,24,0xff);
 o.level=number(tag,end,"level",0xff);if(o.level==0xff)o.level=lookup_name(tag,end,"levelname",c->level_names,4,0);
 if(!attribute(tag,end,"range",v))v[0]=0;char *dash=strchr(v,'-');const char *high=v;if(dash){*dash=0;high=dash+1;}
 o.start=(uint8_t)decimal(v);o.end=(uint8_t)decimal(high);
 if(!attribute(tag,end,"type",v))v[0]=0;int indexed=o.color==0xff;
 if(same(v,"set"))o.kind=indexed?1:0;else if(same(v,"blendfrom"))o.kind=indexed?5:3;
 else if(same(v,"blendto"))o.kind=indexed?6:4;else if(same(v,"blend"))o.kind=2;else if(same(v,"mult"))o.kind=7;else return 0;
 if(o.kind==1||o.kind==5||o.kind==6)o.color=number(tag,end,"palindex",0xff);
 if(o.start>o.end){uint8_t t=o.start;o.start=o.end;o.end=t;
  if(o.kind==3)o.kind=4;else if(o.kind==4)o.kind=3;else if(o.kind==5)o.kind=6;else if(o.kind==6)o.kind=5;}
 *op=o;return 1;
}
static const char *find(const char *p,const char *end,const char *s){size_t n=strlen(s);
 for(;p+n<=end;++p)if(!memcmp(p,s,n))return p;return NULL;}
int nflstreet_dynclut_parse(const uint8_t *text,size_t n,const nflstreet_gear_texture_catalog *c,nflstreet_dynclut_op *ops,unsigned max,unsigned *count){
 if(!text||!c||!count||(max&&!ops))return 0;const char *p=(const char*)text,*end=p+n;
 if(!(p=find(p,end,"<dynclut"))||!(p=find(p,end,"<operations"))||!(p=strchr(p,'>'))||p>=end)return 0;
 const char *close=find(p,end,"</operations>");if(!close)return 0;unsigned k=0;
 for(const char *q=p;(q=find(q,close,"<op"))!=NULL;){const char *after=q+3;
  if(after>=close||!(*after==' '||*after=='\t'||*after=='\r'||*after=='\n'||*after=='/'||*after=='>')){q=after;continue;}
  const char *gt=find(after,close,">");if(!gt||k>=max||!parse_op(after,gt,c,ops+k))return 0;++k;q=gt;}
 *count=k;return 1;
}
void nflstreet_dynclut_table_close(nflstreet_dynclut_table *t){if(t){free(t->ops);free(t->first);free(t->count);memset(t,0,sizeof t[0]);}}
int nflstreet_dynclut_table_open(const nflstreet_archive *a,const nflstreet_gear_texture_catalog *c,nflstreet_dynclut_table *out){
 if(!a||!c||!out)return 0;nflstreet_dynclut_table t={0};t.entries=a->count;nflstreet_dynclut_op parsed[256];
 if(!(t.first=calloc(t.entries+1u,sizeof *t.first))||!(t.count=calloc(t.entries+1u,sizeof *t.count)))goto fail;
 for(unsigned i=0;i<t.entries;++i){nflstreet_archive_entry e;uint8_t *b=NULL;size_t n=0;unsigned k=0;
  if(!nflstreet_archive_get(a,i,&e)||!nflstreet_archive_decode(&e,&b,&n))goto fail;
  int ok=nflstreet_dynclut_parse(b,n,c,parsed,256,&k);free(b);if(!ok)goto fail;
  if(k){nflstreet_dynclut_op *grown=realloc(t.ops,(size_t)(t.total+k)*sizeof *grown);if(!grown)goto fail;t.ops=grown;
   memcpy(t.ops+t.total,parsed,k*sizeof *parsed);}
  t.first[i]=t.total;t.count[i]=k;t.total+=k;}
 *out=t;return 1;
fail:nflstreet_dynclut_table_close(&t);return 0;
}
typedef struct {unsigned r,g,b;} rgb;
static rgb decode(uint16_t format,unsigned v){rgb c={0,0,0};
 if(format==1){c.r=v>>8&0xf8u;c.g=v>>3&0xfcu;c.b=v<<3&0xffu;}
 else if(format==2){if(!(v&0x8000u)){c.r=v>>4&0xf0u;c.g=v&0xf0u;c.b=v<<4&0xffu;}else{c.r=v>>7&0xf8u;c.g=v>>2&0xf8u;c.b=v<<3&0xffu;}}
 return c;}
static void encode(uint16_t format,uint8_t *p,rgb c){unsigned old=half(p);
 if(format==1)put16(p,(c.r&0xf8u)<<8|(c.g&0xfcu)<<3|c.b>>3);
 else if(format==2)put16(p,old&0x8000u?(old&0x8000u)|(c.r&0xf8u)<<7|(c.g&0xf8u)<<2|c.b>>3:(old&0xf000u)|(c.r&0xf0u)<<4|(c.g&0xf0u)|c.b>>4);}
static rgb word_color(const uint8_t *e){rgb c={e[0],e[1],e[2]};return c;}
static unsigned clamp(int v){return v>0xff?0xffu:(unsigned)v&0xffu;}
static rgb mix(rgb a,rgb b,int wa,int wb,int n){rgb c={clamp(((int)a.r*wa+(int)b.r*wb)/n),clamp(((int)a.g*wa+(int)b.g*wb)/n),clamp(((int)a.b*wa+(int)b.b*wb)/n)};return c;}
int nflstreet_dynclut_apply(uint16_t format,uint8_t *pal,size_t bytes,const nflstreet_dynclut_op *ops,unsigned count,const uint8_t *colors[NFLSTREET_COLOR_STATE_BYTES]){
 if(!pal||(count&&!ops)||!colors)return 0;size_t entries=bytes/2;
 for(unsigned i=0;i<count;++i){const nflstreet_dynclut_op *o=ops+i;unsigned s=o->start,e=o->end;
  if(o->kind>7)return 0;
  int uses_color=o->kind==0||o->kind==3||o->kind==4||o->kind==7;
  if(uses_color&&(o->color>=NFLSTREET_COLOR_STATE_BYTES||!colors[o->color]||o->level>3))return 0;
  if((o->kind==1||o->kind>=5)&&o->color>=entries)return 0;
  if(s>e)continue;if(e>=entries)return 0;int n=(int)(e-s);
  if(o->kind>=2&&o->kind<=6&&!n)return 0; /* original divides by zero */
  rgb color=uses_color?word_color(colors[o->color]+4u*o->level):decode(format,0);
  rgb a,b;
  switch(o->kind){
   case 0:for(unsigned k=s;k<=e;++k)encode(format,pal+2*k,color);break;
   case 1:a=decode(format,half(pal+2u*o->color));for(unsigned k=s;k<=e;++k)encode(format,pal+2*k,a);break;
   case 2:a=decode(format,half(pal+2*s));b=decode(format,half(pal+2*e));for(unsigned k=s;k<=e;++k)encode(format,pal+2*k,mix(a,b,n-(int)(k-s),(int)(k-s),n));break;
   case 3:b=decode(format,half(pal+2*e));for(unsigned k=s;k<=e;++k)encode(format,pal+2*k,mix(color,b,n-(int)(k-s),(int)(k-s),n));break;
   case 4:a=decode(format,half(pal+2*s));for(unsigned k=s;k<=e;++k)encode(format,pal+2*k,mix(a,color,n-(int)(k-s),(int)(k-s),n));break;
   case 5:a=decode(format,half(pal+2u*o->color));b=decode(format,half(pal+2*e));for(unsigned k=s;k<=e;++k)encode(format,pal+2*k,mix(a,b,n-(int)(k-s),(int)(k-s),n));break;
   case 6:a=decode(format,half(pal+2*s));b=decode(format,half(pal+2u*o->color));for(unsigned k=s;k<=e;++k)encode(format,pal+2*k,mix(a,b,n-(int)(k-s),(int)(k-s),n));break;
   case 7:a=decode(format,half(pal+2u*o->color));
    for(unsigned k=s;k<=e;++k){a.r=a.r*(color.r*2u)/0xffu;if(a.r>0xff)a.r=0xff;a.g=a.g*(color.g*2u)/0xffu;if(a.g>0xff)a.g=0xff;
     a.b=a.b*(color.b*2u)/0xffu;if(a.b>0xff)a.b=0xff;encode(format,pal+2*k,a);}break;
  }
 }
 return 1;
}
int nflstreet_palette_compose(uint8_t *m,size_t size,uint32_t token,const nflstreet_dynclut_table *t,uint32_t entry,
 const nflstreet_color_table *table,const uint8_t state[NFLSTREET_COLOR_STATE_BYTES],nflstreet_texture_flush flush,void *opaque){
 if(!m||size<40||!t||entry>=t->entries||!table||!state||!flush||memcmp(m,"MMAP",4)||half(m+4)!=2)return 0;
 uint32_t records=word(m+28);if(records<token||records-token>size-12u)return 0;const uint8_t *r=m+(records-token);
 uint32_t at=word(r+8),bytes=word(r+4);if(at<token||at-token>size||bytes>size-(at-token))return 0;
 const uint8_t *colors[NFLSTREET_COLOR_STATE_BYTES];for(unsigned i=0;i<NFLSTREET_COLOR_STATE_BYTES;++i)colors[i]=table->bytes+16u*state[i];
 if(!nflstreet_dynclut_apply(half(r+2),m+(at-token),bytes,t->ops+t->first[entry],t->count[entry],colors))return 0;
 return flush(opaque,at,bytes);
}
int nflstreet_palette_compose_slot(void *context,unsigned slot,uint32_t palette,uint8_t *destination,size_t size,uint32_t token){
 const nflstreet_palette_context *c=context;(void)slot;
 return c&&nflstreet_palette_compose(destination,size,token,c->table,palette,c->colors,c->state,c->flush,c->opaque);
}
