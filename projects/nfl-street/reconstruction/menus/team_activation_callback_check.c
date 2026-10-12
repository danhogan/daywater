#define main activation_fixture_main
#include "team_activation_check.c"
#undef main
#include "team_screen.h"
#define SCREEN_BASE 0x81100000u
#define CALLBACK_ARGS 0x817d6100u
#define RESULT_AT 0x817d7000u
#define VM_STACK 0x817c0000u
static int callback_case(const ModernGekkoModuleDesc*m,const uint8_t*baseline,unsigned side,unsigned stack_owner,unsigned dirty,unsigned dims){
 uint8_t*ram=malloc(RAM_SIZE);nflstreet_team_screen*s=calloc(1,sizeof *s);if(!ram||!s)return 0;memcpy(ram,baseline,RAM_SIZE);
 uint8_t screen[1024],before[1024];uint32_t words[256];memset(screen,0xa5,sizeof screen);for(unsigned i=0;i<256;++i)words[i]=0xa5a5a5a5;
 fill_case(&s->activation.sides[side],side,7,(uint8_t)dirty,0,4);for(unsigned i=0;i<20;++i)s->activation.sides[side].roster[i].selected=i<7?(int)i:-1;
 nflstreet_team_activation_side input=s->activation.sides[side];s->supplied_activation_mask=1u<<side;s->supplied_activation_team[side]=s->game.startup.selected_id[side]=37;
 uint32_t descriptor=(stack_owner?VM_STACK:SCREEN_BASE)+64,address=descriptor+4*(dims+1);
 if(stack_owner){words[16]=dims;for(unsigned i=0;i<dims;++i)words[17+i]=7;}else{put(screen+64,dims);for(unsigned i=0;i<dims;++i)put(screen+68+i*4,7);}memcpy(before,screen,sizeof screen);
 nflstreet_ui_service_request q={0};q.kind=1;q.property=0x80000012u;q.argc=2;q.args[0]=side;q.args[1]=descriptor;uint32_t result=0xdeadbeef;
 if(!nflstreet_team_screen_memory(s,screen,sizeof screen,SCREEN_BASE,&q,&result,VM_STACK,words))return 0;
 seed(ram,&input,side,0);if(stack_owner){for(unsigned i=0;i<256;++i)put(ptr(ram,VM_STACK)+4*i,0xa5a5a5a5);put(ptr(ram,descriptor),dims);for(unsigned i=0;i<dims;++i)put(ptr(ram,descriptor+4+i*4),7);}else memcpy(ptr(ram,SCREEN_BASE),before,sizeof screen);
 put(ptr(ram,CALLBACK_ARGS),side);put(ptr(ram,CALLBACK_ARGS+4),descriptor);put(ptr(ram,RESULT_AT),0xdeadbeef);
 hooks h={0};h.input=&input;h.side=side;CPUState c={0};c.ram=ram;c.ram_size=RAM_SIZE;c.msr=0x2000;c.gpr[1]=STACK;c.gpr[2]=0x803b9a40;c.gpr[13]=R13;c.gpr[3]=0x80000012u;c.gpr[4]=CALLBACK_ARGS;c.gpr[6]=RESULT_AT;c.pc=0x8011e9d4;
 if(!run_original(m,&c,&h)||c.gpr[3]!=1||be(ptr(ram,RESULT_AT))!=result)return 0;
 for(unsigned i=0;i<7;++i){uint32_t native=stack_owner?words[(address-VM_STACK)/4+i]:be(screen+address-SCREEN_BASE+i*4);if(be(ptr(ram,address+i*4))!=native)return 0;}
 if(memcmp(screen,ptr(ram,SCREEN_BASE),sizeof screen)&&!stack_owner)return 0;
 if(h.effect_count!=s->activation.effect_count)return 0;
 for(unsigned i=0;i<h.effect_count;++i)if(h.effects[i].kind!=s->activation.effects[i].kind||h.effects[i].index!=s->activation.effects[i].index||memcmp(h.effects[i].words,s->activation.effects[i].words,20))return 0;
 free(s);free(ram);return 1;
}
static int guards(void){nflstreet_team_screen*s=calloc(1,sizeof *s),*before=malloc(sizeof *s);if(!s||!before)return 0;uint8_t b[1024],old[1024];uint32_t w[256],ow[256],out=0xabcdef01;memset(b,0x55,sizeof b);for(unsigned i=0;i<256;++i)w[i]=0x55555555;
 nflstreet_ui_service_request q={0};q.kind=1;q.property=0x80000012u;q.argc=2;q.args[0]=0;q.args[1]=SCREEN_BASE+64;
 for(unsigned v=0;v<4;++v){s->supplied_activation_mask=v?1:0;s->supplied_activation_team[0]=v==1?2:1;s->game.startup.selected_id[0]=1;put(b+64,v==2?9:1);s->activation.sides[0].dirty=v==3;s->activation.sides[0].rebuild=v==3;
  *before=*s;memcpy(old,b,sizeof b);memcpy(ow,w,sizeof w);if(nflstreet_team_screen_memory(s,b,sizeof b,SCREEN_BASE,&q,&out,VM_STACK,w)||memcmp(s,before,sizeof *s)||memcmp(old,b,sizeof b)||memcmp(ow,w,sizeof w)||out!=0xabcdef01)return 0;
 }free(s);free(before);return 1;}
int main(int argc,char**argv){if(argc!=3)return 2;HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;const ModernGekkoModuleDesc*m=get();if(!m||m->cpu_state_size!=sizeof(CPUState))return 3;uint8_t*seedram;if(!load_snapshot(argv[2],&seedram))return 4;unsigned count=0;
 for(unsigned side=0;side<2;++side)for(unsigned owner=0;owner<2;++owner)for(unsigned dirty=0;dirty<2;++dirty)for(unsigned d=0;d<3;++d){unsigned dims=d==2?8:d;if(!callback_case(m,seedram,side,owner,dirty,dims)){fprintf(stderr,"Callback12 differs side%u owner%u dirty%u dims%u\n",side,owner,dirty,dims);return 5;}++count;}
 if(!guards())return 6;printf("Team activation callback exact: %u controlled source callback12 comparisons and four atomic guards; original8011E9D4/E460/E3E8/F4EC execute, formationwrapper/poses/roster supplied. Owned screen/stack outputs and ordered effects match; no full activation/renderer claim.\n",count);free(seedram);FreeLibrary(h);return 0;}
