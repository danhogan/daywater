#include <windows.h>
#include "moderngekko/module_abi.h"
#include "team_player_window.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM_SIZE (24u*1024u*1024u)
#define BASE 0x80000000u
#define R13 0x803a9a40u
#define ROSTER 0x802f50d4u
#define QUERY 0x802f52ccu
#define CURSOR 0x802f52bcu
#define F_OUT 0x817d6000u
#define F_NEXT 0x817d6020u
#define F_NEXT2 0x817d6040u
#define F_MODE 0x817d6060u
#define D_OUT 0x817d6080u
#define STACK 0x817df800u

static uint32_t be(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void put(uint8_t*p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
static uint8_t*ptr(uint8_t*ram,uint32_t a){if(a<BASE||a-BASE>=RAM_SIZE)return NULL;return ram+a-BASE;}
static uint8_t*load_file(const char*path,size_t*size){FILE*f=fopen(path,"rb");if(!f)return NULL;if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}long n=ftell(f);if(n<1||(size_t)n>RAM_SIZE||fseek(f,0,SEEK_SET)){fclose(f);return NULL;}uint8_t*p=malloc((size_t)n);if(!p){fclose(f);return NULL;}if(fread(p,1,(size_t)n,f)!=(size_t)n){free(p);fclose(f);return NULL;}fclose(f);*size=(size_t)n;return p;}
static uint64_t hash_bytes(const uint8_t*p,size_t n){uint64_t h=1469598103934665603ull;for(size_t i=0;i<n;++i){h^=p[i];h*=1099511628211ull;}return h;}
typedef struct {const nflstreet_team_player_window_side*x;unsigned side;int32_t query_index;unsigned query_calls,visibility_calls;uint32_t visible_index,visible_value,query_indices[4];} services;
static int query_side(uint32_t address){return address==QUERY?0:address==QUERY+44u?1:-1;}
static int32_t requested_index(const nflstreet_team_player_window_side*x,int32_t delta){int64_t v=(int64_t)x->cursor+x->mode;if(v<(int64_t)x->query_count)v+=delta;else v=v-(int64_t)x->query_count+delta-1;if(v< -1)v=(int64_t)x->query_count-1;else if(v>=(int64_t)x->query_count)v=-1;return (int32_t)v;}
static int run_original(const ModernGekkoModuleDesc*m,CPUState*c,services*s){
 c->exception=0;c->downcount=c->cycle_budget=100000000;
 for(unsigned step=0;c->pc&&step<300000;++step){uint32_t pc=c->pc;
  if(pc==0x8014928c){int side=query_side(c->gpr[3]);if(side!=(int)s->side||c->gpr[4]>=s->x->query_count||s->query_calls>=4){fprintf(stderr,"Query setter args side%d/%u ordinal%u count%u\n",side,s->side,c->gpr[4],s->x->query_count);return 0;}s->query_index=(int32_t)c->gpr[4];s->query_indices[s->query_calls++]=c->gpr[4];c->pc=c->lr;
  }else if(pc==0x8014e760){int side=query_side(c->gpr[3]);if(side!=(int)s->side||s->query_index<0||(uint32_t)s->query_index>=s->x->query_count)return 0;c->gpr[3]=s->x->query_ids[s->query_index];c->pc=c->lr;
  }else if(pc==0x80129068){if(s->visibility_calls++||c->gpr[4]!=0)return 0;s->visible_index=c->gpr[3];s->visible_value=c->gpr[4];c->pc=c->lr;
  }else{c->downcount=-1000000000;m->dispatch(c,pc);}
  if(c->exception){fprintf(stderr,"Original player-window exception pc%08x ex%08x\n",c->pc,c->exception);return 0;}
 }
 if(c->pc||c->gpr[1]!=STACK){fprintf(stderr,"Original player-window stopped pc%08x sp%08x\n",c->pc,c->gpr[1]);return 0;}return 1;
}
static void seed(uint8_t*ram,const nflstreet_team_player_window_side*x,unsigned side){
 put(ptr(ram,R13-0x5a34u+side*4u),x->count);put(ptr(ram,CURSOR+side*8u),(uint32_t)x->cursor);put(ptr(ram,CURSOR+side*8u+4u),(uint32_t)x->mode);
 uint8_t*rows=ptr(ram,ROSTER+side*0xf4u);for(unsigned i=0;i<20;++i){put(rows+i*12,x->rows[i].id);put(rows+i*12+4,(uint32_t)x->rows[i].slot);put(rows+i*12+8,x->rows[i].auxiliary);}
 put(ptr(ram,R13-0x6e1cu),x->timer_word);put(ptr(ram,R13-0x6e18u+side*4u),0xa5a5a5a5u);
 put(ptr(ram,F_OUT-4),0xdeadbeefu);put(ptr(ram,F_OUT),0xa5a5a5a5u);put(ptr(ram,F_OUT+4),0xa5a5a5a5u);put(ptr(ram,F_NEXT),0xa5a5a5a5u);put(ptr(ram,F_NEXT+4),0xa5a5a5a5u);put(ptr(ram,F_NEXT2),0xa5a5a5a5u);put(ptr(ram,F_NEXT2+4),0xa5a5a5a5u);put(ptr(ram,F_MODE),0xa5a5a5a5u);put(ptr(ram,D_OUT),0xa5a5a5a5u);put(ptr(ram,D_OUT+4),0xa5a5a5a5u);put(ptr(ram,D_OUT+8),0xdeadbeefu);
}
static int setup_cpu(CPUState*c,uint8_t*ram,uint32_t fn,unsigned side,uint32_t out){memset(c,0,sizeof *c);c->ram=ram;c->ram_size=RAM_SIZE;c->msr=0x2000;c->gpr[1]=STACK;c->gpr[2]=0x803b9a40u;c->gpr[13]=R13;c->gpr[3]=side;c->pc=fn;c->lr=0;c->downcount=c->cycle_budget=100000000;if(fn==0x8011d6f8){c->gpr[4]=F_OUT;c->gpr[5]=F_NEXT;c->gpr[6]=F_NEXT2;c->gpr[7]=F_MODE;}else{c->gpr[4]=0xabcdef01;c->gpr[5]=out;c->gpr[6]=D_OUT;}return 1;}
static int compare_f(const ModernGekkoModuleDesc*m,const uint8_t*base,const nflstreet_team_player_window_side*x,unsigned side){
 uint8_t*ram=malloc(RAM_SIZE);if(!ram)return 0;memcpy(ram,base,RAM_SIZE);seed(ram,x,side);nflstreet_team_player_window state={0};state.sides[side]=*x;nflstreet_team_player_window_pair expected[3];uint32_t mode;
 if(!nflstreet_team_player_window_callback_f(&state,side,expected,&mode)){free(ram);return 0;}services sv={0};sv.x=x;sv.side=side;sv.query_index=-1;CPUState c;setup_cpu(&c,ram,0x8011d6f8,side,0);if(!run_original(m,&c,&sv)){free(ram);return 0;}
 const uint32_t addrs[3]={F_OUT,F_NEXT,F_NEXT2};for(unsigned i=0;i<3;++i)if(be(ptr(ram,addrs[i]))!=expected[i].id||be(ptr(ram,addrs[i]+4))!=expected[i].slot){fprintf(stderr,"D6F8 side%u cursor%d mode%d pair%u got%08x/%08x wanted%08x/%08x\n",side,x->cursor,x->mode,i,be(ptr(ram,addrs[i])),be(ptr(ram,addrs[i]+4)),expected[i].id,expected[i].slot);free(ram);return 0;}
 uint32_t expected_indices[3];unsigned expected_count=0;int32_t ordinals[3]={x->cursor,x->cursor==(int32_t)x->count-1?-1:x->cursor+1,0};ordinals[2]=ordinals[1]==(int32_t)x->count-1?-1:ordinals[1]+1;for(unsigned i=0;i<3;++i)if(ordinals[i]>=0)expected_indices[expected_count++]=(uint32_t)ordinals[i];
 if(be(ptr(ram,F_MODE))!=mode||sv.query_calls!=expected_count||memcmp(sv.query_indices,expected_indices,expected_count*4)||be(ptr(ram,F_OUT-4))!=0xdeadbeefu){fprintf(stderr,"D6F8 mode/query sequence/guard side%u cursor%d mode%d calls%u expected%u\n",side,x->cursor,x->mode,sv.query_calls,expected_count);free(ram);return 0;}
 free(ram);return 1;
}
static int compare_d(const ModernGekkoModuleDesc*m,const uint8_t*base,const nflstreet_team_player_window_side*x,unsigned side,int32_t delta){
 uint8_t*ram=malloc(RAM_SIZE);if(!ram)return 0;memcpy(ram,base,RAM_SIZE);seed(ram,x,side);nflstreet_team_player_window state={0};state.sides[side]=*x;nflstreet_team_player_window_pair expected;if(!nflstreet_team_player_window_callback_10(&state,side,delta,&expected)){free(ram);return 0;}
 services sv={0};sv.x=x;sv.side=side;sv.query_index=-1;CPUState c;setup_cpu(&c,ram,0x8011d988,side,D_OUT);c.gpr[5]=(uint32_t)delta;if(!run_original(m,&c,&sv)){free(ram);return 0;}
 if(be(ptr(ram,D_OUT))!=expected.id||be(ptr(ram,D_OUT+4))!=expected.slot||be(ptr(ram,CURSOR+side*8u))!=(uint32_t)state.sides[side].cursor||be(ptr(ram,CURSOR+side*8u+4u))!=(uint32_t)state.sides[side].mode||be(ptr(ram,R13-0x6e18u+side*4u))!=x->timer_word){fprintf(stderr,"D988 mismatch side%u cursor%d mode%d delta%d gotpair%08x/%08x cursor%08x mode%08x timer%08x\n",side,x->cursor,x->mode,delta,be(ptr(ram,D_OUT)),be(ptr(ram,D_OUT+4)),be(ptr(ram,CURSOR+side*8u)),be(ptr(ram,CURSOR+side*8u+4u)),be(ptr(ram,R13-0x6e18u+side*4u)));free(ram);return 0;}
 int32_t qindex=requested_index(x,delta);unsigned expected_queries=qindex>=0;if(sv.query_calls!=expected_queries||(expected_queries&&sv.query_indices[0]!=(uint32_t)qindex)||state.sides[side].visibility_count!=sv.visibility_calls||(sv.visibility_calls&&(sv.visible_index!=state.sides[side].visibility_index||sv.visible_value!=0))||be(ptr(ram,D_OUT+8))!=0xdeadbeefu){fprintf(stderr,"D988 query/visibility/guard mismatch side%u cursor%d mode%d delta%d queries%u/%u calls%u expected%u index%u/%u value%u guard%08x\n",side,x->cursor,x->mode,delta,sv.query_calls,expected_queries,sv.visibility_calls,state.sides[side].visibility_count,sv.visible_index,state.sides[side].visibility_index,sv.visible_value,be(ptr(ram,D_OUT+8)));free(ram);return 0;}
 free(ram);return 1;
}
static void fill(nflstreet_team_player_window_side*x,uint32_t count,int32_t cursor,int32_t mode,unsigned side,unsigned salt){memset(x,0,sizeof *x);x->count=x->query_count=count;x->cursor=cursor;x->mode=mode;x->timer_word=0x12340000u|side<<8|salt;for(unsigned i=0;i<20;++i){x->rows[i].id=3000u+side*100u+salt*20u+i;x->rows[i].slot=(i<7&&i%3u==2u)?-1:(int32_t)i;x->rows[i].auxiliary=0xabc00000u+i;x->query_ids[i]=x->rows[i].id;}}
static int native_guards(void){nflstreet_team_player_window s={0},before;nflstreet_team_player_window_pair out[3]={{0}},save[3];uint32_t mode=0xdeadbeefu;fill(&s.sides[0],3,1,1,0,7);before=s;memcpy(save,out,sizeof out);if(nflstreet_team_player_window_callback_10(&s,0,0,&out[0])||memcmp(&s,&before,sizeof s)||memcmp(out,save,sizeof out))return 0;if(nflstreet_team_player_window_callback_10(&s,0,1,(nflstreet_team_player_window_pair*)&s)||memcmp(&s,&before,sizeof s))return 0;s.sides[0].cursor=3;before=s;if(nflstreet_team_player_window_callback_f(&s,0,out,&mode)||memcmp(&s,&before,sizeof s)||mode!=0xdeadbeefu||memcmp(out,save,sizeof out))return 0;return 1;}
static int run_context(const ModernGekkoModuleDesc*m,const char*path,unsigned context){size_t n=0;uint8_t*base=load_file(path,&n);if(!base||n!=RAM_SIZE){free(base);return 0;}uint64_t hash=hash_bytes(base,n);unsigned checks=0;
 for(unsigned side=0;side<2;++side)for(unsigned count_case=0;count_case<2;++count_case){uint32_t count=count_case?1:6;int32_t cursors[3]={-1,0,(int32_t)count-1};for(unsigned ci=0;ci<3;++ci)for(int32_t mode=0;mode<=2;++mode){nflstreet_team_player_window_side x;fill(&x,count,cursors[ci],mode,side,checks+context);if(!compare_f(m,base,&x,side)){fprintf(stderr,"context%u callbackF case failed side%u count%u cursor%d mode%d\n",context,side,count,x.cursor,x.mode);free(base);return 0;}for(int32_t delta=-1;delta<=1;delta+=2){if(!compare_d(m,base,&x,side,delta)){fprintf(stderr,"context%u callback10 case failed side%u count%u cursor%d mode%d delta%d\n",context,side,count,x.cursor,x.mode,delta);free(base);return 0;}++checks;}}}
 nflstreet_team_player_window_side empty;fill(&empty,0,-1,0,0,context);for(unsigned side=0;side<2;++side){fill(&empty,0,-1,(int32_t)side,side,context+1);if(!compare_f(m,base,&empty,side)||!compare_d(m,base,&empty,side,1)){fprintf(stderr,"context%u empty-list case failed side%u\n",context,side);free(base);return 0;}checks+=2;}
 if(hash_bytes(base,n)!=hash){free(base);return 0;}printf("context%u: %u D6F8/D988 callback comparisons passed; supplied ordered PLAY-ID getter, roster triples and timer word. Baseline FNV64 %016llx unchanged.\n",context,checks,(unsigned long long)hash);free(base);return 1;}
int main(int argc,char**argv){if(argc!=3){fprintf(stderr,"Usage: team-player-window-check.exe <original-DLL> <snapshot-folder>\n");return 2;}if(!native_guards()){fprintf(stderr,"Native player-window invalid-state/alias guard failed\n");return 3;}HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;const ModernGekkoModuleDesc*m=get();if(!m||m->cpu_state_size!=sizeof(CPUState))return 3;unsigned contexts[3]={14,15,16};for(unsigned i=0;i<3;++i){char path[4096];int n=snprintf(path,sizeof path,"%s/position-neutral-%02u.mem1",argv[2],contexts[i]);if(n<0||(size_t)n>=sizeof path||!run_context(m,path,contexts[i]))return 4;}FreeLibrary(h);puts("Team player-window callbacks exact on 3 saved contexts; database getter and visibility requests were explicit supplies; original callbacks and roster-slot projection matched.");return 0;}
