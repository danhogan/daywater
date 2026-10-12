#include <windows.h>
#include "moderngekko/module_abi.h"
#include "team_lineup.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM_SIZE (24u*1024u*1024u)
#define BASE 0x80000000u
#define R13 0x803a9a40u
#define ROWS 0x817d0000u
#define ROLES 0x817d2000u
#define EXCLUDED 0x817d2100u
#define OUT_IDS 0x817d2200u
#define OUT_SCORES 0x817d2240u
#define OUTPUT_PLAYER 0x817d2300u
#define OUTPUT_ROLE 0x817d2304u
#define STACK 0x817df000u
#define ROSTER 0x802f50d4u
#define LAST_PLAYER 0x802f51c4u

static void put(uint8_t*p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
static uint32_t be(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static uint8_t*load_file(const char*path,size_t*size){FILE*f=fopen(path,"rb");if(!f)return NULL;if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}long z=ftell(f);if(z!=RAM_SIZE||fseek(f,0,SEEK_SET)){fclose(f);return NULL;}uint8_t*b=malloc((size_t)z);if(!b){fclose(f);return NULL;}if(fread(b,1,(size_t)z,f)!=(size_t)z){free(b);fclose(f);return NULL;}fclose(f);*size=(size_t)z;return b;}
static uint64_t hash_bytes(const uint8_t*p,size_t n){uint64_t h=1469598103934665603ull;for(size_t i=0;i<n;++i){h^=p[i];h*=1099511628211ull;}return h;}

typedef struct{uint32_t count,role,excluded_count;uint32_t ids[20],scores[20],choice_id,choice_role;} test_case;
static const test_case cases[]={
 {5,2,0,{10,11,12,13,14},{80,80,70,70,60},0,0}, /* equal scores */
 {5,1,0,{20,21,22,23,24},{50,90,90,40,90},0,0}, /* intended-role preference */
 {6,4,2,{30,31,32,33,34,35},{100,80,80,60,50,40},0,0}, /* excluded row indices */
 {5,3,2,{40,41,42,43,44},{9,8,7,6,5},0,0} /* role exclusions and excluded flags */
};
static nflstreet_team_lineup_player full_weighted[8];

static void make_full_rows(void){
 for(unsigned i=0;i<8;++i){full_weighted[i]=(nflstreet_team_lineup_player){0};full_weighted[i].player_id=100+i;full_weighted[i].role=(i+2)%14;
  for(unsigned r=0;r<14;++r)full_weighted[i].scores[r]=(8-i)*10+(r%4);
 }
}

static int run_original_roster(const ModernGekkoModuleDesc*m,uint8_t*ram){
 CPUState c={0};c.ram=ram;c.ram_size=RAM_SIZE;c.msr=0x2000;c.gpr[1]=STACK;c.gpr[2]=0x803b9a40u;c.gpr[13]=R13;c.gpr[3]=0;c.pc=0x8011e7ec;c.lr=0;c.downcount=c.cycle_budget=100000000;
 for(unsigned step=0;c.pc&&step<300000;++step){uint32_t pc=c.pc;
  if(pc==0x80042568){uint32_t base=ROWS;put(ram+R13-BASE-0x7bb0,base);put(ram+R13-BASE-0x50b0,8);
   for(unsigned i=0;i<8;++i){uint8_t*row=ram+ROWS-BASE+i*0x44;put(row,full_weighted[i].player_id);for(unsigned r=0;r<14;++r)put(row+4+r*4,full_weighted[i].scores[r]);row[0x3c]=0;put(row+0x40,full_weighted[i].role);}c.pc=c.lr;
  }else if(pc==0x80042514){uint32_t list=c.gpr[3];for(unsigned i=0;i<8;++i){uint8_t*row=ram+ROWS-BASE+i*0x44;int found=0;for(unsigned j=0;j<22;++j){uint32_t id=be(ram+list-BASE+j*4);if(id==0x7fff)break;if(id==be(row)){found=1;break;}}row[0x3c]=found?0:1;}c.pc=c.lr;
  }else if(pc==0x8010b60c){c.gpr[3]=6;c.pc=c.lr;
  }else if(pc==0x80042748){c.pc=c.lr;
  }else{c.downcount=-1000000000;m->dispatch(&c,pc);}
  if(c.exception){fprintf(stderr,"Original E7EC exception pc=%08x ex=%08x\n",c.pc,c.exception);return 0;}
 }
 if(c.pc||c.gpr[1]!=STACK){fprintf(stderr,"Original E7EC stopped pc=%08x sp=%08x\n",c.pc,c.gpr[1]);return 0;}return 1;
}

static int compare_full_roster(const ModernGekkoModuleDesc*m,const uint8_t*baseline){
 uint8_t*ram=malloc(RAM_SIZE);if(!ram)return 0;memcpy(ram,baseline,RAM_SIZE);make_full_rows();
 nflstreet_team_lineup_roster native={0};native.count=8;
 for(unsigned i=0;i<8;++i){native.rows[i].player_id=100+i;native.rows[i].slot=0xffffffffu;native.rows[i].aux=0xffu;}
 if(!nflstreet_team_lineup_select_roster(&native,full_weighted,8)){free(ram);return 0;}
 for(unsigned i=0;i<8;++i){uint8_t*row=ram+ROSTER-BASE+i*12;put(row,100+i);put(row+4,0xffffffffu);put(row+8,0xffu);}
 put(ram+R13-BASE-0x5a34,8);put(ram+LAST_PLAYER-BASE,0xccccccccu);
 if(!run_original_roster(m,ram)){free(ram);return 0;}
 for(unsigned i=0;i<8;++i){uint8_t*row=ram+ROSTER-BASE+i*12;if(be(row)!=native.rows[i].player_id||be(row+4)!=native.rows[i].slot||be(row+8)!=native.rows[i].aux){fprintf(stderr,"E7EC triple%u original %u/%08x/%08x native %u/%08x/%08x\n",i,be(row),be(row+4),be(row+8),native.rows[i].player_id,native.rows[i].slot,native.rows[i].aux);free(ram);return 0;}}
 if(be(ram+LAST_PLAYER-BASE)!=native.last_seventh_player_id){fprintf(stderr,"E7EC seventh id original %u native %u\n",be(ram+LAST_PLAYER-BASE),native.last_seventh_player_id);free(ram);return 0;}
 free(ram);return 1;
}

static void seed_case(uint8_t*ram,const test_case*t,unsigned which,nflstreet_team_lineup_player*rows){
 memset(ram+ROWS-BASE,0,20*0x44);for(uint32_t i=0;i<t->count;++i){rows[i]=(nflstreet_team_lineup_player){0};rows[i].player_id=t->ids[i];rows[i].role=(i%3==0)?t->role:((t->role+1)%14);rows[i].flags=(which==3&&i==1)?1:0;
  for(uint32_t r=0;r<14;++r){uint32_t v=(r==t->role)?t->scores[i]:(uint32_t)(5+i+r);rows[i].scores[r]=v;uint8_t*row=ram+ROWS-BASE+i*0x44;put(row,t->ids[i]);put(row+4+r*4,v);}
  ram[ROWS-BASE+i*0x44+0x3c]=rows[i].flags;put(ram+ROWS-BASE+i*0x44+0x40,rows[i].role);
 }
}

static int call_original(const ModernGekkoModuleDesc*m,uint8_t*ram,uint32_t fn,
 uint32_t role_or_roles){
 CPUState c={0};c.ram=ram;c.ram_size=RAM_SIZE;c.msr=0x2000;c.gpr[1]=STACK;c.gpr[2]=0x803b9a40u;c.gpr[13]=R13;c.pc=fn;c.lr=0;c.downcount=c.cycle_budget=100000000;
 if(fn==0x8004231c){c.gpr[3]=role_or_roles;c.gpr[4]=OUT_IDS;c.gpr[5]=OUT_SCORES;}
 else{c.gpr[3]=0;c.gpr[4]=role_or_roles;c.gpr[5]=EXCLUDED;c.gpr[6]=OUTPUT_PLAYER;c.gpr[7]=OUTPUT_ROLE;}
 for(unsigned step=0;c.pc&&step<100000;++step){uint32_t pc=c.pc;
  if(pc==0x80042514){c.pc=c.lr;}
  else if(pc==0x8010b60c){c.gpr[3]=6;c.pc=c.lr;}
  else{c.downcount=-1000000000;m->dispatch(&c,pc);}
  if(c.exception){fprintf(stderr,"Original lineup exception pc=%08x ex=%08x\n",c.pc,c.exception);return 0;}
 }
 if(c.pc||c.gpr[1]!=STACK){fprintf(stderr,"Original lineup stopped pc=%08x sp=%08x\n",c.pc,c.gpr[1]);return 0;}return 1;
}

static int compare(const ModernGekkoModuleDesc*m,const uint8_t*baseline,unsigned which){
 test_case t=cases[which];uint8_t*ram=malloc(RAM_SIZE);if(!ram)return 0;memcpy(ram,baseline,RAM_SIZE);
 nflstreet_team_lineup_player players[20]={0};seed_case(ram,&t,which,players);put(ram+R13-BASE-0x50b0,t.count);put(ram+R13-BASE-0x7bb0,ROWS);
 put(ram+ROLES-BASE,0x7fff);put(ram+EXCLUDED-BASE,0xff);if(t.excluded_count){put(ram+EXCLUDED-BASE,1);put(ram+EXCLUDED-BASE+4,3);put(ram+EXCLUDED-BASE+8,0xff);}
 for(unsigned i=0;i<4;++i){put(ram+OUT_IDS-BASE+i*4,0x7fff);put(ram+OUT_SCORES-BASE+i*4,0);}
  nflstreet_team_lineup_ranked native[4];if(!nflstreet_team_lineup_rank(players,t.count,t.role,native)||!call_original(m,ram,0x8004231c,t.role)){free(ram);return 0;}
 for(unsigned i=0;i<4;++i){uint32_t id=be(ram+OUT_IDS-BASE+i*4),score=be(ram+OUT_SCORES-BASE+i*4);if(id!=native[i].player_id||score!=native[i].score){fprintf(stderr,"rank case%u slot%u original %u/%u native %u/%u\n",which,i,id,score,native[i].player_id,native[i].score);free(ram);return 0;}}
 {uint32_t excluded[2]={1,3};nflstreet_team_lineup_choice selected;
  put(ram+ROLES-BASE,0x7fff);
  if(t.excluded_count){put(ram+EXCLUDED-BASE,excluded[0]);put(ram+EXCLUDED-BASE+4,excluded[1]);put(ram+EXCLUDED-BASE+8,0xff);}else put(ram+EXCLUDED-BASE,0xff);
  if(!nflstreet_team_lineup_choose_role(players,t.count,NULL,0,t.excluded_count?excluded:NULL,t.excluded_count,&selected)||!call_original(m,ram,0x80042850,ROLES)){free(ram);return 0;}
  uint32_t oid=be(ram+OUTPUT_PLAYER-BASE),orole=be(ram+OUTPUT_ROLE-BASE);
  if(oid!=selected.player_id||orole!=selected.role){fprintf(stderr,"role chooser original %u/%u native %u/%u\n",oid,orole,selected.player_id,selected.role);free(ram);return 0;}
 }
 free(ram);return 1;
}

int main(int argc,char**argv){
 if(argc!=3){fprintf(stderr,"Usage: team-lineup-original-check.exe <original-DLL> <position-neutral-snapshot-folder>\n");return 2;}
 HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;const ModernGekkoModuleDesc*m=get();if(!m||m->cpu_state_size!=sizeof(CPUState))return 3;
 const unsigned snapshots[]={14,15,16};for(unsigned s=0;s<3;++s){char path[4096];int n=snprintf(path,sizeof path,"%s/position-neutral-%02u.mem1",argv[2],snapshots[s]);if(n<0||(size_t)n>=sizeof path)return 4;size_t z=0;uint8_t*base=load_file(path,&z);if(!base)return 4;uint64_t hash=hash_bytes(base,z);
  for(unsigned c=0;c<sizeof cases/sizeof cases[0];++c)if(!compare(m,base,c)){fprintf(stderr,"snapshot%u case%u mismatch\n",snapshots[s],c);free(base);return 5;}
  if(!compare_full_roster(m,base)){fprintf(stderr,"snapshot%u E7EC roster mismatch\n",snapshots[s]);free(base);return 7;}
  if(hash!=hash_bytes(base,z)){free(base);return 6;}printf("snapshot%u: original rank/role synthetic cases and E7EC seven-player roster projection passed; baseline FNV64 %016llx unchanged\n",snapshots[s],(unsigned long long)hash);free(base);
 }
 FreeLibrary(h);puts("Lineup bounded weighted-row checks passed; original 80042568 database weighting/bootstrap is outside this module.");return 0;
}
