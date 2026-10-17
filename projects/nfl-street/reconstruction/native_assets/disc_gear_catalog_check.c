#include "disc_gear_catalog.h"
#include "dol_sections.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do {if(!(x)){fprintf(stderr,"comparison failed at line %u: %s\n",__LINE__,#x);return 4;}}while(0)
static uint8_t *readfile(const char *path,size_t *size){
 FILE *f=fopen(path,"rb");if(!f)return NULL;
 if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}long n=ftell(f);
 if(n<=0||fseek(f,0,SEEK_SET)){fclose(f);return NULL;}
 uint8_t *p=malloc((size_t)n);if(!p){fclose(f);return NULL;}
 if(fread(p,1,(size_t)n,f)!=(size_t)n){free(p);fclose(f);return NULL;}fclose(f);*size=(size_t)n;return p;
}
static uint32_t word(const uint8_t *p){return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static int equal_views(const nflstreet_gear_texture_catalog *a,const nflstreet_gear_texture_catalog *b){
 return !memcmp(a->window,b->window,0x868)&&!memcmp(a->categories,b->categories,48)&&!memcmp(a->tags,b->tags,104)&&
 !memcmp(a->excluded_dynamic_name,b->excluded_dynamic_name,32)&&!memcmp(a->templates,b->templates,100)&&
 !memcmp(a->color_names,b->color_names,360)&&!memcmp(a->level_names,b->level_names,60)&&!memcmp(a->color_tags,b->color_tags,52);
}
int main(int argc,char **argv){
 if(argc!=7)return 2;
 size_t dn=0,rn=0,tn=0,mn=0;uint8_t *dol=readfile(argv[2],&dn),*ref=readfile(argv[3],&rn),
 *db=readfile(argv[4],&tn),*ram=readfile(argv[5],&mn);
 if(!dol||!ref||!db||!ram||mn!=24u*1024u*1024u)return 3;
 unsigned cases=(unsigned)strtoul(argv[6],NULL,10);if(cases>10000)return 2;
 nflstreet_gear_texture_catalog expected;nflstreet_team_table table;
 CHECK(nflstreet_gear_texture_catalog_open(ref,rn,&expected));
 CHECK(nflstreet_team_table_open_tag(db,tn,0x52414547u,&table)&&table.used);
 nflstreet_disc_gear_catalog from_disc={0},owned={0};
 CHECK(nflstreet_disc_gear_catalog_open_disc(argv[1],&from_disc));
 CHECK(nflstreet_disc_gear_catalog_open_dol(dol,dn,&owned));
 CHECK(owned.size==rn&&from_disc.size==rn&&!memcmp(owned.bytes,ref,rn)&&!memcmp(from_disc.bytes,ref,rn));
 CHECK(equal_views(&owned.catalog,&expected)&&equal_views(&from_disc.catalog,&expected));
 /* Source lifetime is not borrowed after open. */
 memset(dol,0,dn);CHECK(!memcmp(owned.bytes,ref,rn));
 nflstreet_disc_gear_catalog zero={0},before={0};
 CHECK(!nflstreet_disc_gear_catalog_open_dol(NULL,0,&zero)&&!memcmp(&zero,&before,sizeof zero));
 CHECK(!nflstreet_disc_gear_catalog_open_dol(dol,dn,&zero)&&!memcmp(&zero,&before,sizeof zero));
 CHECK(!nflstreet_disc_gear_catalog_open_dol(dol,255,&zero)&&!memcmp(&zero,&before,sizeof zero));
 CHECK(!nflstreet_disc_gear_catalog_open_disc(NULL,&zero)&&!memcmp(&zero,&before,sizeof zero));
 CHECK(!nflstreet_disc_gear_catalog_open_disc("Z:\\daywater-missing-disc.iso",&zero)&&!memcmp(&zero,&before,sizeof zero));
 /* Restore the pinned source; reject a one-byte wrong revision while valid
    consumer data and the complete structural header remain intact. */
 free(dol);dol=readfile(argv[2],&dn);CHECK(dol);
 dol[dn-1]^=1;CHECK(!nflstreet_disc_gear_catalog_open_dol(dol,dn,&zero)&&!memcmp(&zero,&before,sizeof zero));dol[dn-1]^=1;
 before=owned;CHECK(!nflstreet_disc_gear_catalog_open_dol(dol,dn,&owned)&&!memcmp(&owned,&before,sizeof owned));
 CHECK(!nflstreet_disc_gear_catalog_open_disc(argv[1],&owned)&&!memcmp(&owned,&before,sizeof owned));
 uint32_t rng=word(ram+0x3a9a40u);unsigned digest=2166136261u;
 for(unsigned i=0;i<cases;++i){
  rng=rng*1664525u+1013904223u;unsigned row=rng%table.used,slot=(rng>>16)%27;
  uint8_t actual[112],wanted[112];memset(actual,(int)(rng>>24),sizeof actual);memcpy(wanted,actual,sizeof actual);
  CHECK(nflstreet_gear_record_from_table(&table,row,&owned.catalog,actual));
  CHECK(nflstreet_gear_record_from_table(&table,row,&expected,wanted));CHECK(!memcmp(actual,wanted,sizeof actual));
  uint32_t template=nflstreet_gear_texture_template(&owned.catalog,slot);
  CHECK(template==nflstreet_gear_texture_template(&expected,slot));
  for(unsigned j=0;j<sizeof actual;++j)digest=(digest^actual[j])*16777619u;digest=(digest^template)*16777619u;
  /* Successful independent owner each cycle, close twice, failed reopen then
     successful reopen. Covers disjoint buffers and repeated cleanup. */
  nflstreet_disc_gear_catalog cycle={0};CHECK(nflstreet_disc_gear_catalog_open_dol(dol,dn,&cycle));
  CHECK(cycle.bytes!=owned.bytes&&cycle.size==rn&&!memcmp(cycle.bytes,ref,rn));
  nflstreet_disc_gear_catalog_close(&cycle);nflstreet_disc_gear_catalog_close(&cycle);
  nflstreet_disc_gear_catalog clean={0};CHECK(!memcmp(&cycle,&clean,sizeof clean));
  CHECK(!nflstreet_disc_gear_catalog_open_dol(NULL,dn,&cycle));
  CHECK(nflstreet_disc_gear_catalog_open_dol(dol,dn,&cycle));nflstreet_disc_gear_catalog_close(&cycle);
 }
 for(unsigned slot=0;slot<27;++slot)CHECK(nflstreet_gear_texture_template(&owned.catalog,slot)==nflstreet_gear_texture_template(&expected,slot));
 nflstreet_disc_gear_catalog_close(&from_disc);CHECK(!memcmp(owned.bytes,ref,rn));
 nflstreet_disc_gear_catalog_close(&owned);nflstreet_disc_gear_catalog_close(NULL);
 printf("Disc gear catalog: %u consumer/lifecycle cases, %zu exact bytes, 8 invalid/occupied inputs; digest %08x\n",cases,rn,digest);
 free(dol);free(ref);free(db);free(ram);return 0;
}
