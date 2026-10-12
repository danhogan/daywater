#include <windows.h>
#include "moderngekko/module_abi.h"
#include "team_confirm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM_SIZE (24u*1024u*1024u)
#define R13 0x803a9a40u
#define STACK 0x817df000u
#define CURSOR 0x817d1000u
#define ROSTER 0x802f50d4u
#define POSE_A 0x802f5340u
#define POSE_B 0x802f5324u
#define PREVIEW 0x802a4430u
#define VIS 0x80282cfcu
#define LAST 0x802f51c4u

static uint32_t be(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void put(uint8_t*p,uint32_t v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static uint8_t*read_file(const char*path,size_t*n){FILE*f=fopen(path,"rb");if(!f)return NULL;if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}long z=ftell(f);if(z<0||z>(long)RAM_SIZE){fclose(f);return NULL;}rewind(f);uint8_t*b=malloc((size_t)z);if(!b||fread(b,1,(size_t)z,f)!=(size_t)z){free(b);fclose(f);return NULL;}fclose(f);*n=(size_t)z;return b;}

typedef struct {unsigned visibility_calls;uint32_t visibility_index,visibility_value;} oracle_trace;
static void seed_side(uint8_t*ram,nflstreet_team_confirm*model,unsigned side,
 const int32_t*slots,uint32_t count){
 nflstreet_team_confirm_side*s=&model->sides[side];s->count=count;
 for(unsigned i=0;i<count;++i){s->rows[i]=(nflstreet_team_confirm_row){1000u+side*100u+i,slots[i],0xa5000000u+i};
  uint32_t at=ROSTER+side*0xf4+i*12;put(ram+at-0x80000000,s->rows[i].player_id);put(ram+at-0x80000000+4,(uint32_t)s->rows[i].slot);put(ram+at-0x80000000+8,s->rows[i].auxiliary);}
 put(ram+R13-0x80000000-0x5a34+side*4,count);
 uint32_t last=LAST+side*0xf4;put(ram+last-0x80000000,s->last_confirmed_id);
 ram[R13-0x80000000-0x6e34+side]=s->source_flag_6e34;
 ram[R13-0x80000000-0x6e30+side]=s->source_flag_6e30;
}
static int compare_one(const ModernGekkoModuleDesc*m,const uint8_t*baseline,size_t n,unsigned test){
 uint8_t*ram=malloc(n);if(!ram)return 0;memcpy(ram,baseline,n);
 nflstreet_team_confirm model={0};uint32_t output=0xabcddcba,retval=0x12345678;
 int32_t a_slots[8]={0,1,-1,2,3,-1,4,-1};int32_t b_slots[6]={0,-1,1,2,-1,3};
 unsigned side=test&1;model.sides[0].source_flag_6e34=(test&2)?1:0;model.sides[1].source_flag_6e34=(test&4)?1:0;
 model.sides[0].source_flag_6e30=1;model.sides[1].source_flag_6e30=1;
 model.sides[0].last_confirmed_id=0xdead0000;model.sides[1].last_confirmed_id=0xdead0001;
 uint32_t count0=test&8?6:8,count1=test&16?5:6;
 if((test&8)&&side==0)for(unsigned i=0;i<count0;++i)a_slots[i]=(int32_t)i;
 if((test&16)&&side==1)for(unsigned i=0;i<count1;++i)b_slots[i]=(int32_t)i;
 seed_side(ram,&model,0,a_slots,count0);seed_side(ram,&model,1,b_slots,count1);
 for(unsigned i=0;i<14;++i){model.visibility[i]=(uint8_t)(0x30+i);ram[VIS-0x80000000+i]=model.visibility[i];}
 for(unsigned i=0;i<28;++i){model.preview_map[i]=0x70000000u+i;put(ram+PREVIEW-0x80000000+i*4,model.preview_map[i]);}
 uint32_t pa[2][7],pb[2][7];for(unsigned s=0;s<2;++s)for(unsigned i=0;i<7;++i){pa[s][i]=i;pb[s][i]=i+7;put(ram+POSE_A-0x80000000+s*0x38+i*4,pa[s][i]);put(ram+POSE_B-0x80000000+s*0x38+i*4,pb[s][i]);}
 uint32_t player= (test&32)?UINT32_MAX:(side?1102u:1005u);
 if(player!=UINT32_MAX&&count0==8&&count1==6&&side==0)player=1005; /* known matching row */
 if(player!=UINT32_MAX&&((count0==6&&side==0)||(count1==5&&side==1)))player=9999; /* source's no-match-at-count case */
 if(!nflstreet_team_confirm_apply(&model,side,player,&output,&retval,pa,pb)){free(ram);return 0;}
 put(ram+CURSOR-0x80000000,0xabcddcba);
 CPUState c={0};c.ram=ram;c.ram_size=RAM_SIZE;c.msr=0x2000;c.gpr[1]=STACK;c.gpr[2]=0x803b9a40u;c.gpr[13]=R13;c.gpr[3]=side;c.gpr[4]=CURSOR;c.gpr[5]=player;c.pc=0x8011df88;c.lr=0;c.cycle_budget=10000000;
 oracle_trace trace={0};
 for(unsigned step=0;c.pc&&step<100000;++step){uint32_t pc=c.pc;
  if(pc==0x800dfa2c){if(++trace.visibility_calls!=1)return 0;trace.visibility_index=c.gpr[3];trace.visibility_value=c.gpr[4];c.pc=c.lr;}
  else{c.downcount=-1000000000;m->dispatch(&c,pc);}
  if(c.exception){fprintf(stderr,"Confirm exception PC%08x ex%08x test%u\n",c.pc,c.exception,test);free(ram);return 0;}
 }
 if(c.pc||c.gpr[1]!=STACK){free(ram);return 0;}
 if(c.gpr[3]!=retval||be(ram+CURSOR-0x80000000)!=output){fprintf(stderr,"Gate/cursor mismatch test%u result%u/%u cursor%x/%x\n",test,c.gpr[3],retval,be(ram+CURSOR-0x80000000),output);free(ram);return 0;}
 uint32_t expected_visibility=0,expected_value=0;for(unsigned i=0;i<model.effect_count;++i)if(model.effects[i].kind==NFLSTREET_TEAM_CONFIRM_VISIBILITY_BYTE){expected_visibility=model.effects[i].words[0];expected_value=model.effects[i].words[1];}
 if(trace.visibility_calls!=(player==UINT32_MAX?0u:1u)||(trace.visibility_calls&&(trace.visibility_value!=expected_value||trace.visibility_index!=expected_visibility))){fprintf(stderr,"Visibility trace mismatch test%u calls%u index%u value%x expected%u/%x\n",test,trace.visibility_calls,trace.visibility_index,trace.visibility_value,expected_visibility,expected_value);free(ram);return 0;}
 for(unsigned i=0;i<14;++i)if(ram[VIS-0x80000000+i]!=model.visibility[i]){fprintf(stderr,"Visibility state mismatch test%u idx%u original%x native%x\n",test,i,ram[VIS-0x80000000+i],model.visibility[i]);free(ram);return 0;}
 for(unsigned s=0;s<2;++s){uint32_t count=s?count1:count0,base=ROSTER+s*0xf4;
  for(unsigned i=0;i<count;++i){uint32_t at=base+i*12;nflstreet_team_confirm_row*r=&model.sides[s].rows[i];if(be(ram+at-0x80000000)!=r->player_id||be(ram+at-0x80000000+4)!=(uint32_t)r->slot||be(ram+at-0x80000000+8)!=r->auxiliary){fprintf(stderr,"Roster triple mismatch test%u side%u row%u\n",test,s,i);free(ram);return 0;}}
  if(ram[R13-0x80000000-0x6e34+s]!=model.sides[s].source_flag_6e34||ram[R13-0x80000000-0x6e30+s]!=model.sides[s].source_flag_6e30||be(ram+LAST+s*0xf4-0x80000000)!=model.sides[s].last_confirmed_id){fprintf(stderr,"Confirm side state mismatch test%u side%u\n",test,s);free(ram);return 0;}
 }
 for(unsigned i=0;i<28;++i)if(be(ram+PREVIEW-0x80000000+i*4)!=model.preview_map[i]){fprintf(stderr,"Preview map mismatch test%u idx%u original%x native%x\n",test,i,be(ram+PREVIEW-0x80000000+i*4),model.preview_map[i]);free(ram);return 0;}
 free(ram);return 1;
}

static int service_135(const ModernGekkoModuleDesc*m,const uint8_t*baseline,size_t n,uint32_t sentinel){
 uint8_t*ram=malloc(n);if(!ram)return 0;memcpy(ram,baseline,n);put(ram+CURSOR-0x80000000,sentinel);
 CPUState c={0};c.ram=ram;c.ram_size=RAM_SIZE;c.msr=0x2000;c.gpr[1]=STACK;c.gpr[2]=0x803b9a40u;c.gpr[13]=R13;
 c.gpr[3]=0x135;c.gpr[4]=0x817d2000;c.gpr[5]=0;c.gpr[6]=CURSOR;c.pc=0x8010cb80;c.cycle_budget=10000000;
 for(unsigned step=0;c.pc&&step<10000;++step){uint32_t pc=c.pc;c.downcount=-1000000000;m->dispatch(&c,pc);if(c.exception){free(ram);return 0;}}
 int ok=!c.pc&&c.gpr[3]==1&&be(ram+CURSOR-0x80000000)==sentinel;free(ram);return ok;
}

static int callback_13(const ModernGekkoModuleDesc*m,const uint8_t*baseline,size_t n,uint32_t mode){
 uint8_t*ram=malloc(n);if(!ram)return 0;memcpy(ram,baseline,n);uint32_t sentinel=0x87654321;put(ram+0x17d2000,0x67);put(ram+CURSOR-0x80000000,sentinel);put(ram+R13-0x80000000-0x6c68,0xdeadbeef);
 CPUState c={0};c.ram=ram;c.ram_size=RAM_SIZE;c.msr=0x2000;c.gpr[1]=STACK;c.gpr[2]=0x803b9a40u;c.gpr[13]=R13;
 c.gpr[3]=0x80000013;c.gpr[4]=0x817d2000;c.gpr[5]=0;c.gpr[6]=CURSOR;c.pc=0x8011e9d4;c.cycle_budget=10000000;
 for(unsigned step=0;c.pc&&step<10000;++step){uint32_t pc=c.pc;
  if(pc==0x8010b60c){c.gpr[3]=mode;c.pc=c.lr;}
  else{c.downcount=-1000000000;m->dispatch(&c,pc);}
  if(c.exception){free(ram);return 0;}
 }
 int ok=!c.pc&&c.gpr[3]==1&&be(ram+R13-0x80000000-0x6c68)==0&&be(ram+CURSOR-0x80000000)==sentinel;
 free(ram);return ok;
}

int main(int argc,char**argv){
 if(argc!=3){fprintf(stderr,"Usage: team-confirm-original-check.exe <original-DLL> <snapshot.mem1>\n");return 2;}
 HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;const ModernGekkoModuleDesc*m=get();if(!m||m->cpu_state_size!=sizeof(CPUState))return 3;
 size_t n=0;uint8_t*baseline=read_file(argv[2],&n);if(!baseline||n!=RAM_SIZE)return 4;
 unsigned passed=0;for(unsigned t=0;t<64;++t){if(!compare_one(m,baseline,n,t)){fprintf(stderr,"Confirmation case failed %u\n",t);return 5;}++passed;}
 for(unsigned i=0;i<2;++i)if(!service_135(m,baseline,n,i?0xabcdef01u:0u)){fprintf(stderr,"Property135 preserved-output case%u failed\n",i);return 6;}
 const uint32_t modes[]={0,1,3,4,5,7,8};for(unsigned i=0;i<sizeof modes/sizeof modes[0];++i)if(!callback_13(m,baseline,n,modes[i])){fprintf(stderr,"Callback13 mode%u failed\n",modes[i]);return 7;}
 printf("Original8011DF88 confirmation gate exact: %u controlled side/flag/count/ID cases, original roster triple swaps, cursor output, visibility byte and preview index writes compared. Property135 preserves zero and nonzero output sentinels; original callback13 arg67 clears r13-6C68 in seven supplied modes other than6. `8003F6EC` table inputs and `800DFA2C` visibility side effect are controlled boundaries; mode6 500ms service and source-input/render integration are not claimed.\n",passed);
 free(baseline);FreeLibrary(h);return 0;
}
