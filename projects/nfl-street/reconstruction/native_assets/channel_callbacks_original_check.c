/* Task K: whole 8003A24C, real group and program callbacks on both sides.
   Never substitute native code into the original module's callbacks.
   80039990/80039A18 are the only omitted actor routines; ARAM is a file
   service. Compare all RAM outside the oracle stack after EVERY update. */
#include <windows.h>
#include "moderngekko/module_abi.h"
#include "scenery_draw.h"
#include "check_fixture.h"
#include "channel_callbacks.h"
#define ACTORS 14u
#define CASES 401u
#define SOAK_CASES 3u
#define SOAK_UPDATES 600u
#define PROGRAMS 0x80f10000u
static const ModernGekkoModuleDesc *module;static uint8_t *oracle;static nflstreet_anim_aram aram;
static uint8_t *files[200];static size_t file_sizes[200];
static uint32_t gameplay[ACTORS],visual[ACTORS];
static uint32_t table_base(uint8_t *ram){return ram32(ram,ram32(ram,R13-0x7c30u));}
static void put16(uint8_t *ram,uint32_t a,uint32_t v){uint8_t *p=ram_at(ram,a,2);p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static void put32(uint8_t *ram,uint32_t a,uint32_t v){wr32(ram_at(ram,a,4),v);}
static void put8(uint8_t *ram,uint32_t a,uint32_t v){ram_at(ram,a,1)[0]=(uint8_t)v;}
static uint16_t get16(uint8_t *ram,uint32_t a){const uint8_t *p=ram_at(ram,a,2);return (uint16_t)(p[0]<<8|p[1]);}

static uint8_t frames_seed[ACTORS][NFLSTREET_POSE_LOCAL_BYTES],frames_after[ACTORS][NFLSTREET_POSE_LOCAL_BYTES];
static unsigned frames_seen,original_events,original_groups,original_operands;
static uint32_t callback_roots[224];static unsigned nroots;
static int is_group(uint32_t pc){for(unsigned i=0;i<nroots;++i)if(callback_roots[i]==pc)return 1;return 0;}
static int run_update(float dt){
 CPUState c;memset(&c,0,sizeof c);c.ram=oracle;c.ram_size=RAM_BYTES;c.msr=0x2000;c.hid2=0xe0000000u;c.gqr[6]=0x0f070007;
 c.gpr[1]=STACK_HI-0x40u;c.gpr[2]=0x803b9a40u;c.gpr[13]=R13;c.fpr[1]=(double)dt;c.ps1[1]=(double)dt;c.pc=0x8003a24cu;
 for(unsigned step=0;c.pc;++step){
  if(step>50000000u){fprintf(stderr,"8003A24C did not return (pc %08x)\n",c.pc);return 0;}
  const uint32_t at=c.pc;
  if(at==0x80039990u||at==0x80039a18u){c.pc=c.lr;continue;}
  /* Observe only. These callbacks execute through module->dispatch. */
  if(at==0x8001532cu)++original_events;
  if(c.lr==0x8017793cu&&is_group(at))++original_groups;
  if(at==0x801777a0u)++original_operands;
  if(at==0x8017bb78u){uint8_t *to=ram_at(oracle,c.gpr[3],c.gpr[5]);if(!to)return 0;
   nflstreet_anim_aram_read(&aram,c.gpr[4],to,c.gpr[5]);c.pc=c.lr;continue;}
  if(at==0x8015e9c8u){unsigned i=0;while(i<ACTORS&&visual[i]!=c.gpr[3])++i;if(i==ACTORS)return 0;
   memcpy(ram_at(oracle,c.gpr[1]-0x218u+8u,NFLSTREET_POSE_LOCAL_BYTES),frames_seed[i],NFLSTREET_POSE_LOCAL_BYTES);}
  if(at==0x801784b8u&&c.lr==0x8003a3b0u){
   unsigned i=0;while(i<ACTORS&&gameplay[i]!=c.gpr[31])++i;if(i==ACTORS)return 0;
   memcpy(frames_after[i],ram_at(oracle,c.gpr[1]-0x8u-0x218u+8u,NFLSTREET_POSE_LOCAL_BYTES),NFLSTREET_POSE_LOCAL_BYTES);++frames_seen;}
  c.downcount=-1000000000;module->dispatch(&c,c.pc);
  if(c.exception){fprintf(stderr,"exception at %08x (lr %08x)\n",c.pc,c.lr);return 0;}}
 return 1;}
static int same(const uint8_t *native){
 const uint32_t lo=STACK_LO-RAM_BASE,hi=STACK_HI-RAM_BASE;unsigned bad=0;
 for(uint32_t i=0;i<RAM_BYTES;++i)if((i<lo||i>=hi)&&oracle[i]!=native[i]){
  if(bad<24)fprintf(stderr,"RAM differs at %08x: original %02x native %02x\n",RAM_BASE+i,oracle[i],native[i]);++bad;}
 return !bad;}
static uint8_t *callback_span(void *ram,uint32_t a,uint32_t n){return span(ram,a,n);}
static void ctx_init(nflstreet_actor_channels *c,nflstreet_channel_callbacks *cb,uint8_t *ram){
 memset(c,0,sizeof *c);nflstreet_channel_callbacks_init(cb);c->span=callback_span;c->memory=ram;c->aram=&aram;
 c->event=nflstreet_channel_callbacks_event;c->group=nflstreet_channel_callbacks_group;c->user=cb;
}
/* Retain saved keys, resource identities and callback private records. Edits
   exercise real callback layouts rather than randomizing their pointers. */
static const uint16_t commands[]={0xc000,0xc001,0xc002,0xc003,0xc004,0xc005,0xc006,0xc007,0xc008,
 0xc009,0xc00a,0xc00b,0xc00c,0xc00d,0xc00e,0xc00f,0xc012,0xc014,0xc016,0xc018,0xc019,
 0xc01a,0xc01b,0xc01d,0xc021,0xc022,0xd000,0xd001,0xd002,0xd005,0xc024};
/* Synthetic streams must yield frames in BOTH interpreter modes. 801770FC
   initializes through 8017724C -> 80176E28(mode 1), whose event wrapper
   80173A74 calls r13-6730 (80015D8C), rather than the live 8001532C hook.
   Use the larger arity of these two callbacks for the commands above.
   In particular 80015E04 -> 80015F08 consumes TWO C009 operands, whereas
   800153C4 -> 80015ACC accepts C009 without consuming any. Without the
   explicit operands, the scan consumes our only frames (0, 1), then loops
   forever at 8001. The extra words are valid frames when the live callback
   consumes fewer operands; both sides execute that same original behavior.
   C019 similarly needs three scan operands; C01A and C022 each need one. */
static unsigned synthetic_operands(uint16_t cmd){switch(cmd){case 0xc009:case 0xc00d:return 2;
 case 0xc012:case 0xc019:return 3;
 case 0xc000:case 0xc00e:case 0xc00f:case 0xc014:case 0xc016:case 0xc01a:case 0xc01b:case 0xc021:case 0xc022:case 0xd001:return 1;default:return 0;}}
static void prepare_case(const uint8_t *pristine,uint8_t *ram,unsigned index,float *dt){
 memcpy(ram,pristine,RAM_BYTES);*dt=1.0f;if(!index)return;
 *dt=index%4==0?0.5f:index%4==1?1.0f:index%4==2?2.0f:rndf(0.1f,1.9f);
 for(unsigned i=0;i<ACTORS;++i){const uint32_t a=gameplay[i];
  put32(ram,a+0x304,fbits(rndf(0.6f,1.4f)));put8(ram,a+0x308,rnd()%2);
  for(unsigned k=0;k<4;++k){uint32_t m=a+0x9bc+100*k;if(!get16(ram,m+6))break;
   put32(ram,m+0xc,fbits(index%7==0?-1.0f:rndf(0.0f,2.0f)));
   if(index%9==0){put32(ram,m+0x50,fbits(index%18==0?0.0f:0.0000001f));put32(ram,m+0x58,0);}}
  for(unsigned k=0;k<get16(ram,a+0x4dc);++k){uint32_t ch=a+0x4e4+0x7c*k;if(ram_at(ram,ch+1,1)[0]!=2)continue;
   put32(ram,ch+0x48,fbits(rnd()%3?0.0f:rndf(0.0f,3.0f)));
   if(index%5==0)put32(ram,ch+0x58,fbits(rndf(-0.9f,0.9f)));}}
 const uint32_t a=gameplay[(index-1)%ACTORS];
 for(unsigned k=0;k<get16(ram,a+0x4dc);++k){const uint32_t ch=a+0x4e4+0x7c*k;
  if(ram_at(ram,ch+1,1)[0]!=2)continue;
  const uint32_t at=PROGRAMS;const uint16_t cmd=commands[(index-1)%(sizeof commands/sizeof *commands)];
  put32(ram,at,0x20000000u);unsigned n=0;put16(ram,at+4+2*n++,cmd);
  for(unsigned j=0;j<synthetic_operands(cmd);++j)put16(ram,at+4+2*n++,index%2?2:1);
  put16(ram,at+4+2*n++,0);put16(ram,at+4+2*n++,1);put16(ram,at+4+2*n++,0x8001);
  put32(ram,ch+0x4c,at);put16(ram,ch+0x64,0);put16(ram,ch+0x66,0xffff);put16(ram,ch+0x68,0);put8(ram,ch+0x6b,0);
  put32(ram,ch+0x48,0);put32(ram,ch+0x50,fbits(1.0f));put32(ram,ch+0x58,fbits(-0.5f));break;}}
static unsigned runs,events,groups;static uint32_t digest=2166136261u;
static int compare_update(nflstreet_actor_channels *c,nflstreet_channel_callbacks *cb,float dt,unsigned case_id,unsigned step){
 for(unsigned i=0;i<ACTORS;++i)memcpy(frames_seed[i],c->pose[i].local,NFLSTREET_POSE_LOCAL_BYTES);
 const unsigned ev=cb->events,gr=cb->groups,op=cb->operands,oo=original_operands;original_events=original_groups=frames_seen=0;
 if(!nflstreet_actor_channels_update(c,dt)||!cb->ok){
  fprintf(stderr,"native rejected case %u step %u: callback ok %d pc %08x address %08x\n",case_id,step,cb->ok,cb->failure_pc,cb->failure_address);
  if(cb->failure_budget_exhausted)fprintf(stderr,"callback basic-block budget exhausted (not an unsupported instruction or span failure)\n");return 0;}
 if(!run_update(dt)){fprintf(stderr,"original rejected case %u step %u\n",case_id,step);return 0;}
 if(cb->events-ev!=original_events||cb->groups-gr!=original_groups){
  fprintf(stderr,"callback counts case %u step %u: events %u/%u groups %u/%u\n",case_id,step,cb->events-ev,original_events,cb->groups-gr,original_groups);return 0;}
 if(cb->operands-op!=original_operands-oo){fprintf(stderr,"operand counts case %u step %u: %u/%u\n",case_id,step,cb->operands-op,original_operands-oo);return 0;}
 if(frames_seen!=ACTORS){fprintf(stderr,"blend count %u\n",frames_seen);return 0;}
 for(unsigned i=0;i<ACTORS;++i)for(unsigned b=0;b<NFLSTREET_POSE_LOCAL_BYTES;++b)if((b<0x30||b>=0x34)&&frames_after[i][b]!=c->pose[i].local[b]){
  fprintf(stderr,"pose local differs actor %u +%x case %u step %u\n",i,b,case_id,step);return 0;}
 if(!same(c->memory)){fprintf(stderr,"comparison failed case %u step %u\n",case_id,step);return 0;}
 ++runs;events+=original_events;groups+=original_groups;digest=fnv(digest,c->memory,RAM_BYTES);return 1;}
static int load_files(const char *dir){
 char path[MAX_PATH];unsigned n=0;
 for(unsigned i=0;i<200;++i){snprintf(path,sizeof path,"%s\\%05u.bin",dir,i);FILE *f=fopen(path,"rb");if(!f)continue;
  fseek(f,0,SEEK_END);const long len=ftell(f);rewind(f);if(len>0){files[i]=malloc((size_t)len);if(!files[i]||fread(files[i],1,(size_t)len,f)!=(size_t)len)return 0;file_sizes[i]=(size_t)len;++n;}
  fclose(f);}
 return n>0;}

int main(int argc,char **argv){
 if(argc!=4)return 2;HMODULE h=LoadLibraryA(argv[2]);if(!h)return 3;
 ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get||!(module=get()))return 3;
 if(!load_files(argv[3]))return 3;
 uint8_t *pristine=load_ram(argv[1]),*ram=malloc(RAM_BYTES);oracle=malloc(RAM_BYTES);if(!pristine||!ram||!oracle)return 3;
 const int mapped=nflstreet_anim_aram_map(&aram,span,pristine,(const uint8_t *const *)files,file_sizes,200);if(mapped<=0)return 4;
 const uint32_t base=table_base(pristine);if(get16(pristine,ram32(pristine,R13-0x7c30u)+10)!=ACTORS)return 4;
 for(unsigned i=0;i<ACTORS;++i){gameplay[i]=base+0x14d4u*i;visual[i]=ram32(pristine,gameplay[i]+4);}
 const uint32_t desc=ram32(pristine,ram32(pristine,R13-0x6718u)+4),table=ram32(pristine,desc+4);
 for(unsigned i=0;i<get16(pristine,desc)&&nroots<224;++i){const uint32_t fn=ram32(pristine,table+4*i);if(fn&&!is_group(fn))callback_roots[nroots++]=fn;}
 static nflstreet_actor_channels c;static nflstreet_channel_callbacks cb;
 for(unsigned i=0;i<CASES;++i){float dt;prepare_case(pristine,ram,i,&dt);memcpy(oracle,ram,RAM_BYTES);ctx_init(&c,&cb,ram);
  if(!compare_update(&c,&cb,dt,i,0))return 5;}
 for(unsigned test=0;test<SOAK_CASES;++test){
  float dt;prepare_case(pristine,ram,test,&dt);memcpy(oracle,ram,RAM_BYTES);ctx_init(&c,&cb,ram);
  static uint8_t pose_then[ACTORS][180];
  for(unsigned step=0;step<SOAK_UPDATES;++step){
   if(step==SOAK_UPDATES-60)for(unsigned i=0;i<ACTORS;++i)memcpy(pose_then[i],ram_at(ram,ram32(ram,visual[i]+0x5c),180),180);
   if(!compare_update(&c,&cb,1.0f,test,step+1))return 6;}
  unsigned active=0,moving=0,animating=0;for(unsigned i=0;i<ACTORS;++i){unsigned n=0;for(unsigned k=0;k<get16(ram,gameplay[i]+0x4dc);++k)n+=ram_at(ram,gameplay[i]+0x4e4+0x7c*k+1,1)[0]!=0;
   const int changed=memcmp(pose_then[i],ram_at(ram,ram32(ram,visual[i]+0x5c),180),180)!=0;active+=n>0;moving+=changed;animating+=n>0&&changed;}
  printf("Channel callbacks soak %u: 600 updates, all RAM compared after each; %u/14 active, %u/14 poses changed in last 60, %u/14 keep animating.\n",test,active,moving,animating);}
 if(!events||!groups||!original_operands){fprintf(stderr,"callback/operand coverage missing\n");return 7;}
 printf("Channel callbacks: %u cases (saved + 400 edited), 3 x 600 soak updates; %u full 8003A24C updates of 14 actors match all RAM and pose locals; %u events, %u groups, %u operand entries; digest %08x.\n",CASES,runs,events,groups,original_operands,digest);
 free(pristine);free(ram);free(oracle);for(unsigned i=0;i<200;++i)free(files[i]);FreeLibrary(h);return 0;}
