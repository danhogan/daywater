#include <windows.h>
#include "moderngekko/module_abi.h"
#include "team_player_remove.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM_SIZE (24u*1024u*1024u)
#define BASE 0x80000000u
#define R13 0x803a9a40u
#define ROSTER 0x802f50d4u
#define LAST_ID 0x802f51c4u
#define QUERY_RESULT 0x802f52bcu
#define QUERY_CONTEXT 0x802f52ccu
#define RESET_POSE 0x802826f4u
#define SLOT_OUT 0x817d6000u
#define STACK 0x817df800u
#define NONE 0x7fffu

static uint32_t be(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void put(uint8_t*p,uint32_t v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static uint8_t*ptr(uint8_t*ram,uint32_t a){if(a<BASE||a-BASE>=RAM_SIZE)return NULL;return ram+a-BASE;}
static uint32_t r13addr(int32_t off){return R13+(uint32_t)off;}
static uint8_t*load_file(const char*path,size_t*size){FILE*f=fopen(path,"rb");if(!f)return NULL;if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}long z=ftell(f);if(z!=RAM_SIZE||fseek(f,0,SEEK_SET)){fclose(f);return NULL;}uint8_t*b=malloc((size_t)z);if(!b){fclose(f);return NULL;}if(fread(b,1,(size_t)z,f)!=(size_t)z){free(b);fclose(f);return NULL;}fclose(f);*size=(size_t)z;return b;}
static uint64_t hash_bytes(const uint8_t*p,size_t n){uint64_t h=1469598103934665603ull;for(size_t i=0;i<n;++i){h^=p[i];h*=1099511628211ull;}return h;}
static uint32_t roffset(unsigned side){return ROSTER+side*0xf4u;}
static uint32_t lastaddr(unsigned side){return LAST_ID+side*0xf4u;}
static uint32_t qresult(unsigned side){return QUERY_RESULT+side*8u;}
static uint32_t qcontext(unsigned side){return QUERY_CONTEXT+side*0x2cu;}

typedef struct{uint32_t kind,index,words[5];} effect;
typedef struct{unsigned side,query_calls,visibility_calls,effect_count;uint32_t id,ordinal,slot;const nflstreet_team_activation_pose*pose;effect effects[23];} hooks;
static void push(hooks*h,uint32_t kind,uint32_t index,const uint32_t*w){if(h->effect_count>=23)return;effect*e=&h->effects[h->effect_count++];memset(e,0,sizeof *e);e->kind=kind;e->index=index;if(w)memcpy(e->words,w,20);}
static int run_original(const ModernGekkoModuleDesc*m,CPUState*c,hooks*h){c->exception=0;c->downcount=c->cycle_budget=10000000;
 for(unsigned step=0;c->pc&&step<1000000;++step){uint32_t pc=c->pc;
  if(pc==0x8014e800){if(++h->query_calls!=1||c->gpr[3]!=qcontext(h->side)||c->gpr[4]!=h->id||c->gpr[5]!=qresult(h->side)){fprintf(stderr,"E234 query args %08x/%08x/%08x\n",c->gpr[3],c->gpr[4],c->gpr[5]);return 0;}uint8_t*out=ptr(c->ram,c->gpr[5]);if(!out)return 0;put(out,h->ordinal);c->pc=c->lr;
  }else if(pc==0x80128cf4){unsigned idx=c->gpr[3];if(idx<h->side*7u||idx>=h->side*7u+7u)return 0;float x,y,z;memcpy(&x,&h->pose->position[0],4);memcpy(&y,&h->pose->position[1],4);memcpy(&z,&h->pose->position[2],4);if(c->fpr[1]!=(double)x||c->fpr[2]!=(double)y||c->fpr[3]!=(double)z){fprintf(stderr,"E234 position data mismatch idx%u\n",idx);return 0;}uint32_t w[5]={h->pose->position[0],h->pose->position[1],h->pose->position[2],h->pose->rotation,h->pose->visibility};push(h,NFLSTREET_TEAM_ACTIVATION_POSITION,idx,w);c->pc=c->lr;
  }else if(pc==0x80128e44){unsigned idx=c->gpr[3];if(idx<h->side*7u||idx>=h->side*7u+7u)return 0;float f;memcpy(&f,&h->pose->rotation,4);if(c->fpr[1]!=(double)f)return 0;uint32_t w[5]={h->pose->position[0],h->pose->position[1],h->pose->position[2],h->pose->rotation,h->pose->visibility};push(h,NFLSTREET_TEAM_ACTIVATION_ROTATION,idx,w);c->pc=c->lr;
  }else if(pc==0x80129068){unsigned idx=c->gpr[3];uint32_t words[5]={h->pose->position[0],h->pose->position[1],h->pose->position[2],h->pose->rotation,h->pose->visibility};if(h->visibility_calls++==0){if(idx!=h->slot+h->side*7u||c->gpr[4]!=0)return 0;words[4]=0;}else {unsigned p=idx-h->side*7u;if(p>=7||c->gpr[4]!=h->pose->visibility)return 0;}push(h,NFLSTREET_TEAM_ACTIVATION_VISIBILITY,idx,words);c->pc=c->lr;
  }else{c->downcount=-1000000000;m->dispatch(c,pc);}
  if(c->exception){fprintf(stderr,"E234 exception pc%08x ex%08x\n",c->pc,c->exception);return 0;}
 }
 return !c->pc&&c->gpr[1]==STACK&&h->query_calls==1&&h->visibility_calls==8;
}

static int compare_case(const ModernGekkoModuleDesc*m,const uint8_t*baseline,unsigned side,unsigned count,unsigned row_match,unsigned query_match){
 uint8_t*ram=malloc(RAM_SIZE);if(!ram)return 0;memcpy(ram,baseline,RAM_SIZE);
 nflstreet_team_activation a={0};nflstreet_team_player_window w={0};nflstreet_team_activation_side*x=&a.sides[side];nflstreet_team_player_window_side*y=&w.sides[side];x->count=y->count=y->query_count=count;x->last_seventh_player_id=5000u+side*100u+row_match;
 for(unsigned i=0;i<count;++i){uint32_t id=5000u+side*100u+i;x->roster[i]=(nflstreet_team_activation_row){id,(int32_t)i,0xffu};y->rows[i]=(nflstreet_team_player_window_row){id,(int32_t)i,0xffu};y->query_ids[i]=7000u+side*100u+i;}
 uint32_t ordinal=0;if(query_match<count){y->query_ids[query_match]=x->last_seventh_player_id;ordinal=query_match+1u;}
 nflstreet_team_activation_pose pose={0};uint8_t*raw=ptr(ram,RESET_POSE+side*20u);for(unsigned i=0;i<3;++i)pose.position[i]=be(raw+i*4);pose.rotation=be(raw+12);pose.visibility=be(raw+16);
 uint8_t*rows=ptr(ram,roffset(side));for(unsigned i=0;i<count;++i){put(rows+i*12,x->roster[i].id);put(rows+i*12+4,(uint32_t)x->roster[i].selected);put(rows+i*12+8,x->roster[i].auxiliary);}
 put(ptr(ram,r13addr(-0x5a34)+(int32_t)side*4),count);put(ptr(ram,lastaddr(side)),x->last_seventh_player_id);put(ptr(ram,qresult(side)),0xccccccccu);put(ptr(ram,qresult(side)+4),0xddddddddu);put(ptr(ram,r13addr(-0x6e34)+(int32_t)side),0x7fu);for(unsigned i=0;i<7;++i)put(ptr(ram,SLOT_OUT)+i*4,0xa5a5a5a5u);
 uint32_t slot=0xffffffffu;if(!nflstreet_team_player_remove(&a,&w,side,&pose,&slot)){free(ram);return 0;}
 uint8_t*expected_roster=malloc(count*12u);if(!expected_roster){free(ram);return 0;}for(unsigned i=0;i<count;++i){put(expected_roster+i*12,a.sides[side].roster[i].id);put(expected_roster+i*12+4,(uint32_t)a.sides[side].roster[i].selected);put(expected_roster+i*12+8,a.sides[side].roster[i].auxiliary);}
 /* Restore original input for the oracle; native helper has already updated its separate state. */
 for(unsigned i=0;i<count;++i){put(rows+i*12,5000u+side*100u+i);put(rows+i*12+4,i);put(rows+i*12+8,0xffu);}put(ptr(ram,lastaddr(side)),5000u+side*100u+row_match);put(ptr(ram,qresult(side)),0xccccccccu);put(ptr(ram,qresult(side)+4),0xddddddddu);put(ptr(ram,r13addr(-0x6e34)+(int32_t)side),0x7fu);
 hooks h={0};h.side=side;h.id=5000u+side*100u+row_match;h.ordinal=ordinal;h.slot=row_match;h.pose=&pose;uint32_t de[5]={h.id,row_match,ordinal?(ordinal-1u):(uint32_t)-1,1,ordinal};push(&h,NFLSTREET_TEAM_ACTIVATION_DESELECT,row_match+side*7u,de);
 CPUState c={0};c.ram=ram;c.ram_size=RAM_SIZE;c.msr=0x2000;c.gpr[1]=STACK;c.gpr[2]=0x803b9a40u;c.gpr[13]=R13;c.gpr[3]=side;c.gpr[4]=SLOT_OUT;c.pc=0x8011e234;
 if(!run_original(m,&c,&h)){fprintf(stderr,"E234 original failed side%u row%u query%u\n",side,row_match,query_match);free(expected_roster);free(ram);return 0;}
 if(memcmp(rows,expected_roster,count*12u)||be(ptr(ram,lastaddr(side)))!=NONE||be(ptr(ram,qresult(side)))!=(ordinal?(ordinal-1u):(uint32_t)-1)||be(ptr(ram,qresult(side)+4))!=1||ptr(ram,r13addr(-0x6e34)+(int32_t)side)[0]!=1||be(ptr(ram,SLOT_OUT))!=row_match){fprintf(stderr,"E234 state mismatch side%u row%u query%u cursor%08x mode%u slot%u\n",side,row_match,query_match,be(ptr(ram,qresult(side))),be(ptr(ram,qresult(side)+4)),be(ptr(ram,SLOT_OUT)));free(expected_roster);free(ram);return 0;}
 if(a.effect_count!=23||h.effect_count!=23||slot!=row_match||a.sides[side].last_seventh_player_id!=NONE||a.sides[side].dirty!=1||w.sides[side].cursor!=(ordinal?(int32_t)ordinal-1:-1)||w.sides[side].mode!=1||w.sides[side].visibility_index!=row_match+side*7u){fprintf(stderr,"E234 native state mismatch side%u\n",side);free(expected_roster);free(ram);return 0;}
 for(unsigned i=0;i<23;++i){const nflstreet_team_activation_effect*e=&a.effects[i];if(e->kind!=h.effects[i].kind||e->index!=h.effects[i].index||memcmp(e->words,h.effects[i].words,20)){fprintf(stderr,"E234 effect mismatch side%u event%u\n",side,i);free(expected_roster);free(ram);return 0;}}
 free(expected_roster);free(ram);return 1;
}

int main(int argc,char**argv){if(argc!=3){fprintf(stderr,"Usage: team-player-remove-check.exe <original-DLL> <snapshot.mem1>\n");return 2;}HMODULE dll=LoadLibraryA(argv[1]);if(!dll)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(dll,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;const ModernGekkoModuleDesc*m=get();if(!m||m->cpu_state_size!=sizeof(CPUState))return 3;size_t n=0;uint8_t*baseline=load_file(argv[2],&n);if(!baseline)return 4;uint64_t before=hash_bytes(baseline,n);
 const struct{unsigned side,count,row,query;}cases[]={{0,8,0,0},{0,12,5,9},{1,7,6,3},{1,16,2,15},{0,9,8,9},{1,11,4,11}};for(unsigned i=0;i<sizeof cases/sizeof *cases;++i)if(!compare_case(m,baseline,cases[i].side,cases[i].count,cases[i].row,cases[i].query)){fprintf(stderr,"E234 case%u failed\n",i);return 5;}if(hash_bytes(baseline,n)!=before)return 6;printf("Team player removal callback original comparison passes: %u controlled side/query cases; original 8011E234 and 8014E800 body execute with ordered-ID lookup supplied. Cursor result, roster deselection, dirty flag, output slot, and one removal visibility plus 21 fixed-pose effects match; baseline FNV64 %016llx unchanged.\n",(unsigned)(sizeof cases/sizeof *cases),(unsigned long long)before);free(baseline);FreeLibrary(dll);return 0;}
