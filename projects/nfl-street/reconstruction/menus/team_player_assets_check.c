#include <windows.h>
#include "moderngekko/module_abi.h"
#include "team_preview.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM_SIZE (24u*1024u*1024u)
#define QUERY 0x817d1000u
#define STACK 0x817df000u
#define PLAY_TAG 0x59414c50u
#define TEAM_TAG 0x4d414554u
#define DIGT_TAG 0x44494754u
#define YTRP_TAG 0x59545250u
#define PXSP_TAG 0x50585350u
#define CFRP_TAG 0x43465250u
#define TBYP_TAG 0x54425950u
#define SENTINEL 0xabcddcbau

typedef struct {const uint8_t*bytes;size_t size;size_t rows;uint32_t record_bytes;uint32_t record_bits;uint16_t used,allocated;unsigned field_count;} db_table;
typedef struct {uint32_t tag,bit,width;} numeric_field;
typedef struct {uint32_t values[5];} supplied_fields;

static uint32_t be32(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static uint16_t be16(const uint8_t*p){return (uint16_t)((unsigned)p[0]<<8|p[1]);}
static int range(size_t n,size_t at,size_t z){return at<=n&&z<=n-at;}
static uint8_t*read_file(const char*path,size_t*size,size_t max){FILE*f=fopen(path,"rb");if(!f)return NULL;if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}long z=ftell(f);if(z<1||(size_t)z>max||fseek(f,0,SEEK_SET)){fclose(f);return NULL;}uint8_t*b=malloc((size_t)z);if(!b){fclose(f);return NULL;}if(fread(b,1,(size_t)z,f)!=(size_t)z){free(b);fclose(f);return NULL;}fclose(f);*size=(size_t)z;return b;}

static int open_table(const uint8_t*b,size_t n,uint32_t wanted,db_table*out){
 if(!b||!out||n<28||memcmp(b,"DB\0\x08",4)||be32(b+8)!=n)return 0;uint32_t count=be32(b+16);if(!count||count>64||!range(n,28,(size_t)count*8))return 0;
 size_t base=28+(size_t)count*8,at=0;unsigned found=0;
 for(unsigned i=0;i<count;++i){uint32_t tag=be32(b+24+i*8),offset=be32(b+28+i*8);if(tag==wanted){if(offset>n-base||++found!=1)return 0;at=base+offset;}}
 if(found!=1||!range(n,at,36))return 0;uint32_t bytes=be32(b+at+4),bits=be32(b+at+8);uint16_t alloc=be16(b+at+16),used=be16(b+at+18);unsigned fields=b[at+24];
 if(!bytes||bytes>4096||!bits||bits>(uint64_t)bytes*8||used>alloc||!fields||!range(n,at+36,(size_t)fields*16))return 0;
 size_t field_bytes=(size_t)fields*16,rows=at+36+field_bytes;if(!range(n,rows,(size_t)alloc*bytes))return 0;
 *out=(db_table){b+at,n-at,36+field_bytes,bytes,bits,used,alloc,fields};return 1;
}
static int get_field(const db_table*t,uint32_t tag,numeric_field*out){unsigned found=0;for(unsigned i=0;i<t->field_count;++i){const uint8_t*p=t->bytes+36+i*16;if(be32(p+8)==tag){if(be32(p)!=3||++found!=1)return 0;*out=(numeric_field){tag,be32(p+4),be32(p+12)};}}return found==1&&out->width&&out->width<=32;}
static int bits(const uint8_t*r,size_t n,uint32_t bit,uint32_t width,uint32_t*out){if(!out||!width||width>32||bit>(uint64_t)n*8||width>(uint64_t)n*8-bit)return 0;uint32_t v=0;for(unsigned i=0;i<width;++i){uint32_t at=bit+i;v=(v<<1)|((r[at>>3]>>(7u-(at&7u)))&1u);}*out=v;return 1;}

typedef struct {const supplied_fields*row;unsigned calls,noops;uint32_t tags[4];} original_inputs;
static int original_asset(const ModernGekkoModuleDesc*m,CPUState*c,uint8_t*ram,original_inputs*in,uint32_t*out){
 memset(c,0,sizeof *c);c->ram=ram;c->ram_size=RAM_SIZE;c->msr=0x2000;c->gpr[1]=STACK;c->gpr[2]=0x803b9a40u;c->gpr[13]=0x803a9a40u;c->gpr[3]=QUERY;c->pc=0x8014eac4;c->lr=0;c->cycle_budget=10000000;in->calls=in->noops=0;
 for(unsigned step=0;c->pc&&step<10000;++step){uint32_t pc=c->pc;
  if(pc==0x80149614){if(c->gpr[3]!=QUERY||in->calls>=4)return 0;uint32_t tag=c->gpr[4],v;
   switch(tag){case YTRP_TAG:v=in->row->values[0];break;case PXSP_TAG:v=in->row->values[1];break;case CFRP_TAG:v=in->row->values[2];break;case TBYP_TAG:v=in->row->values[3];break;default:return 0;}
   in->tags[in->calls++]=tag;c->gpr[3]=v;c->pc=c->lr;
  }else if(pc==0x8014fc3c){if(c->gpr[3]!=QUERY||++in->noops>1)return 0;c->pc=c->lr;
  }else{c->downcount=-1000000000;m->dispatch(c,pc);}
  if(c->exception){fprintf(stderr,"Original8014EAC4 exception pc=%08x ex=%08x\n",c->pc,c->exception);return 0;}
 }
 if(c->pc||c->gpr[1]!=STACK)return 0;*out=c->gpr[3];return 1;
}

static uint64_t hash_bytes(const uint8_t*p,size_t n){uint64_t h=1469598103934665603ull;for(size_t i=0;i<n;++i){h^=p[i];h*=1099511628211ull;}return h;}
static int check_file(const ModernGekkoModuleDesc*m,const uint8_t*baseline,const char*dir,unsigned entry,unsigned*players){
 char path[4096];int len=snprintf(path,sizeof path,"%s/%05u.bin",dir,entry);if(len<0||len>=(int)sizeof path)return 0;size_t n=0;uint8_t*db=read_file(path,&n,8u*1024u*1024u);if(!db)return 0;
 db_table team,play;if(!open_table(db,n,TEAM_TAG,&team)||!open_table(db,n,PLAY_TAG,&play)){free(db);return 0;}
 numeric_field team_id_field,fields[4];const uint32_t tags[4]={YTRP_TAG,PXSP_TAG,CFRP_TAG,TBYP_TAG};if(!get_field(&team,DIGT_TAG,&team_id_field)){free(db);return 0;}
 for(unsigned i=0;i<4;++i)if(!get_field(&play,tags[i],&fields[i])){free(db);return 0;}
 numeric_field play_team,play_player;if(!get_field(&play,DIGT_TAG,&play_team)||!get_field(&play,0x44494750u,&play_player)){free(db);return 0;}
 uint32_t team_id;if(team.used!=1||!bits(team.bytes+team.rows,team.record_bytes,team_id_field.bit,team_id_field.width,&team_id)){free(db);return 0;}
 uint32_t expected[NFLSTREET_TEAM_PREVIEW_MAX_PLAYERS],count=0;if(!nflstreet_team_preview_assets_from_db(db,n,team_id,expected,&count)){fprintf(stderr,"Native asset parser failed team file%u id%u\n",entry,team_id);free(db);return 0;}
 uint32_t seen=0;CPUState cpu={0};
 for(unsigned r=0;r<play.used;++r){const uint8_t*row=play.bytes+play.rows+(size_t)r*play.record_bytes;uint32_t id,v[4],player;
  if(!bits(row,play.record_bytes,play_team.bit,play_team.width,&id)||!bits(row,play.record_bytes,play_player.bit,play_player.width,&player)){free(db);return 0;}
  if(id!=team_id)continue;if(seen>=count||seen>=NFLSTREET_TEAM_PREVIEW_MAX_PLAYERS){free(db);return 0;}
  supplied_fields input={{0}};for(unsigned i=0;i<4;++i)if(!bits(row,play.record_bytes,fields[i].bit,fields[i].width,&v[i])){free(db);return 0;}
  memcpy(input.values,v,sizeof v);original_inputs trace={0};trace.row=&input;uint32_t original_value=0;
  if(!original_asset(m,&cpu,(uint8_t*)baseline,&trace,&original_value)){fprintf(stderr,"Original8014EAC4 failed team%u player%u\n",team_id,player);free(db);return 0;}
  uint32_t ytrp=input.values[0],wanted;if(!ytrp)wanted=input.values[1];else{wanted=input.values[2]*3u+0x1adu+(input.values[3]==1?1u:input.values[3]==2?2u:0u);}
  unsigned expected_calls=ytrp?3u:2u,expected_noops=ytrp?1u:0u;
  const uint32_t branch_tags_ytrp[3]={YTRP_TAG,CFRP_TAG,TBYP_TAG};const uint32_t branch_tags_pxsp[2]={YTRP_TAG,PXSP_TAG};const uint32_t*branch=ytrp?branch_tags_ytrp:branch_tags_pxsp;
  if(trace.calls!=expected_calls||trace.noops!=expected_noops||memcmp(trace.tags,branch,expected_calls*sizeof(uint32_t))||wanted!=expected[seen]||original_value!=expected[seen]){
   fprintf(stderr,"Player asset mismatch team%u player%u row%u ytrp%u native%u original%u expected%u trace%u/%u\n",team_id,player,r,ytrp,expected[seen],original_value,wanted,trace.calls,trace.noops);free(db);return 0;
  }
  ++seen;++*players;
 }
 if(seen!=count){fprintf(stderr,"Asset row count differs team%u actual%u model%u\n",team_id,seen,count);free(db);return 0;}free(db);return 1;
}

int main(int argc,char**argv){
 if(argc!=4){fprintf(stderr,"Usage: team-player-assets-check.exe <original-DLL> <snapshot.mem1> <db_teams-folder>\n");return 2;}
 HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;const ModernGekkoModuleDesc*m=get();if(!m||m->cpu_state_size!=sizeof(CPUState))return 3;
 size_t n=0;uint8_t*baseline=read_file(argv[2],&n,RAM_SIZE);if(!baseline||n!=RAM_SIZE)return 4;uint64_t before=hash_bytes(baseline,n);unsigned players=0;
 for(unsigned entry=1;entry<=32;++entry)if(!check_file(m,baseline,argv[3],entry,&players)){fprintf(stderr,"Team player asset check failed at database entry%u\n",entry);return 5;}
 size_t after_n=0;uint8_t*after=read_file(argv[2],&after_n,RAM_SIZE);if(!after||after_n!=n||hash_bytes(after,after_n)!=before){fprintf(stderr,"Snapshot baseline changed during check\n");return 6;}
 printf("Team player asset selection exact: 32 actual team DB blocks, %u PLAY rows across original8014EAC4 and team_preview_assets_from_db; raw PLAY field bits supplied to original getter, original8014FC3C is a no-op test boundary. DB archive/I/O integration not claimed. Baseline FNV64 %016llx unchanged.\n",players,(unsigned long long)before);
 free(after);free(baseline);FreeLibrary(h);return 0;
}
