/* Independent raw-row reader and original dispatch helpers reused as a fixture. */
#define main team_asset_fixture_main
#include "team_player_assets_check.c"
#undef main
#include "team_identity.h"
#define NAME_AT 0x817d2000u

static int original_name(const ModernGekkoModuleDesc*m,uint8_t*ram,const char*last,
 const char*first,unsigned limit,int profile,char result[64]){
 CPUState c={0};c.ram=ram;c.ram_size=RAM_SIZE;c.msr=0x2000;c.gpr[1]=STACK;c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;c.gpr[3]=QUERY;c.gpr[4]=NAME_AT;c.gpr[5]=limit;c.pc=0x8014eb58;c.cycle_budget=10000000;
 memset(ram+NAME_AT-0x80000000u,0x5a,64);unsigned calls=0;
 for(unsigned step=0;c.pc&&step<10000;++step){uint32_t pc=c.pc;
  if(pc==0x8014ea64){c.gpr[3]=(unsigned)profile;c.pc=c.lr;}
  else if(pc==0x801496c4){uint32_t dest=c.gpr[5],tag=c.gpr[4];unsigned width;const char*text;
   if(c.gpr[3]!=QUERY)return 0;
   if(tag==0x414e4c50u&&calls==0){width=14;text=last;}
   else if(tag==0x414e4650u&&calls==1){width=12;text=first;}else return 0;
   if(dest<0x80000000u||dest-0x80000000u>RAM_SIZE-width)return 0;
   memcpy(ram+dest-0x80000000u,text,width);++calls;c.pc=c.lr;
  }else{c.downcount=-1000000000;m->dispatch(&c,pc);}
  if(c.exception)return 0;
 }
 if(c.pc||c.gpr[1]!=STACK||c.gpr[3]!=(unsigned)!profile||calls!=(profile?0u:2u))return 0;
 memcpy(result,ram+NAME_AT-0x80000000u,64);return memchr(result,0,64)!=NULL;
}
static int string_field(const db_table*t,uint32_t tag,unsigned *at,unsigned *width){
 unsigned seen=0;for(unsigned i=0;i<t->field_count;++i){const uint8_t*p=t->bytes+36+i*16;if(be32(p+8)!=tag)continue;
  if(++seen!=1||be32(p)||be32(p+4)%8||be32(p+12)%8)return 0;*at=be32(p+4)/8;*width=be32(p+12)/8;
 }return seen==1&&*at<=t->record_bytes&&*width<=t->record_bytes-*at;
}
int main(int argc,char**argv){
 if(argc!=4){fprintf(stderr,"Usage: team-identity-check.exe <original-DLL> <snapshot.mem1> <db_teams>\n");return 2;}
 HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;const ModernGekkoModuleDesc*m=get();if(!m||m->cpu_state_size!=sizeof(CPUState))return 3;
 size_t seed_n=0;uint8_t*ram=read_file(argv[2],&seed_n,RAM_SIZE);if(!ram||seed_n!=RAM_SIZE)return 4;unsigned count=0;
 for(unsigned entry=1;entry<=32;++entry){char path[4096];snprintf(path,sizeof path,"%s/%05u.bin",argv[3],entry);size_t n;uint8_t*db=read_file(path,&n,8u*1024u*1024u);db_table play;
  if(!db||!open_table(db,n,PLAY_TAG,&play))return 5;numeric_field tf,pf;if(!get_field(&play,DIGT_TAG,&tf)||!get_field(&play,0x44494750u,&pf))return 5;
  unsigned la,lw,fa,fw;if(!string_field(&play,0x414e4c50u,&la,&lw)||!string_field(&play,0x414e4650u,&fa,&fw)||lw!=13||fw!=11)return 5;
  for(unsigned row=0;row<play.used;++row){const uint8_t*r=play.bytes+play.rows+(size_t)row*play.record_bytes;uint32_t tid,pid;if(!bits(r,play.record_bytes,tf.bit,tf.width,&tid)||!bits(r,play.record_bytes,pf.bit,pf.width,&pid))return 5;
   nflstreet_team_identity identity;char oracle[64],last[15]={0},first[13]={0};memcpy(last,r+la,lw);memcpy(first,r+fa,fw);if(!nflstreet_team_identity_from_db(db,n,tid,pid,&identity)||!original_name(m,ram,last,first,26,0,oracle))return 6;
   if(strcmp(identity.model_name,oracle)||memcmp(identity.plna,r+la,lw)||memcmp(identity.pfna,r+fa,fw)){fprintf(stderr,"Name mismatch team%u player%u\n",tid,pid);return 7;}++count;
  }free(db);
 }
 const char last[15]="O'Neil-2",first[13]="Jo. n";for(unsigned limit=0;limit<=26;++limit){char native[27],original[64];if(!nflstreet_team_identity_name(last,first,limit,native)||!original_name(m,ram,last,first,limit,0,original)||strcmp(native,original))return 8;}
 char unused[64];if(!original_name(m,ram,last,first,12,1,unused)||unused[0])return 9;
 nflstreet_team_identity unchanged={0},copy=unchanged;size_t z;uint8_t*db;char path[4096];snprintf(path,sizeof path,"%s/00001.bin",argv[3]);db=read_file(path,&z,8u*1024u*1024u);
 if(!db||nflstreet_team_identity_from_db(db,z,0xffffffffu,0xffffffffu,&unchanged)||memcmp(&unchanged,&copy,sizeof copy))return 10;
 printf("Team identity original name exact: %u actual PLAY rows, 27 punctuation/append-limit cases and original unsupported profile branch. Original8014EB58 and string normalization execute; supplied raw PLNA/PFNA DB getter and profile-false boundary. No model I/O/render claim.\n",count);
 free(db);free(ram);FreeLibrary(h);return 0;
}
