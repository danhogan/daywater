#define main lineup_fixture_main
#include "team_lineup_original_check.c"
#undef main
#include "team_formation.h"
#define FORMATION_OUT 0x817d3000u
static int compare_formation(const ModernGekkoModuleDesc*m,const uint8_t*baseline,unsigned variant){
 uint8_t*ram=malloc(RAM_SIZE);if(!ram)return 0;memcpy(ram,baseline,RAM_SIZE);nflstreet_team_lineup_player rows[20]={0};unsigned count=variant==2?8:7;uint8_t order[2][7];
 for(unsigned i=0;i<count;++i){rows[i].player_id=100+i;rows[i].role=(i+variant)%14;for(unsigned r=0;r<14;++r)rows[i].scores[r]=variant==1?50:(i*13+r*17)%101;}
 for(unsigned s=0;s<2;++s)for(unsigned i=0;i<7;++i)order[s][i]=(uint8_t)((i+variant+s)%7);
 uint32_t native[14];if(!nflstreet_team_formation_assign(rows,count,order,native)){free(ram);return 0;}
 memcpy(ram+R13-BASE-0x7bac,order[0],7);memcpy(ram+R13-BASE-0x7ba4,order[1],7);memset(ram+FORMATION_OUT-BASE,0x55,64);
 CPUState c={0};c.ram=ram;c.ram_size=RAM_SIZE;c.msr=0x2000;c.gpr[1]=STACK;c.gpr[2]=0x803b9a40;c.gpr[13]=R13;c.gpr[3]=variant;c.gpr[4]=ROLES;c.gpr[5]=FORMATION_OUT;c.pc=0x8004278c;c.cycle_budget=10000000;unsigned builds=0,frees=0;
 for(unsigned step=0;c.pc&&step<300000;++step){uint32_t pc=c.pc;
  if(pc==0x80042568){if(++builds!=1||c.gpr[3]!=variant||c.gpr[4]!=ROLES||c.gpr[5]){free(ram);return 0;}
   put(ram+R13-BASE-0x7bb0,ROWS);put(ram+R13-BASE-0x50b0,count);
   for(unsigned i=0;i<count;++i){uint8_t*r=ram+ROWS-BASE+i*0x44;put(r,rows[i].player_id);for(unsigned j=0;j<14;++j)put(r+4+j*4,rows[i].scores[j]);r[0x3c]=rows[i].flags;put(r+0x40,rows[i].role);}c.pc=c.lr;
  }else if(pc==0x80042748){++frees;c.pc=c.lr;}else{c.downcount=-1000000000;m->dispatch(&c,pc);}if(c.exception){free(ram);return 0;}
 }
 if(c.pc||c.gpr[1]!=STACK||builds!=1||frees!=1){free(ram);return 0;}
 for(unsigned r=0;r<14;++r)if(be(ram+FORMATION_OUT-BASE+r*4)!=native[r]){fprintf(stderr,"Formation mismatch variant%u role%u\n",variant,r);free(ram);return 0;}
 for(unsigned i=56;i<64;++i)if(ram[FORMATION_OUT-BASE+i]!=0x55){free(ram);return 0;}free(ram);return 1;
}
int main(int argc,char**argv){if(argc!=3)return 2;HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;const ModernGekkoModuleDesc*m=get();if(!m||m->cpu_state_size!=sizeof(CPUState))return 3;size_t n;uint8_t*seed=load_file(argv[2],&n);if(!seed)return 4;
 for(unsigned v=0;v<3;++v)if(!compare_formation(m,seed,v))return 5;
 nflstreet_team_lineup_player p[7]={0};uint8_t order[2][7]={{0,1,2,3,4,5,5},{0,1,2,3,4,5,6}};uint32_t out[14],before[14];memset(out,0xcc,sizeof out);memcpy(before,out,sizeof out);if(nflstreet_team_formation_assign(p,7,order,out)||memcmp(before,out,sizeof out))return 6;
 puts("Team formation original assignment exact: 3 controlled weighted-roster/permutation cases; original8004278C/rank/mark/reset execute, database weighting/free supplied. Invalid permutation rejects atomically; no native bootstrap/actor rendering claim.");free(seed);FreeLibrary(h);return 0;}
