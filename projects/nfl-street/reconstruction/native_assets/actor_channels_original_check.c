/* Native gameplay-actor animation update (actor_channels.c, anim_stream.c)
   against the original on a saved context.
   - frames: every used stream cache slot of the saved RAM (two caches,
     8017BB78 reads decoded by 801519A8/8015E938 in the saved match)
     against the service's frame from the bank file, decoded by the
     verified native 8015E938.
   - update: per case a fresh copy of the saved RAM (case 0 as saved,
     others edited), the native nflstreet_actor_channels_update, then the
     original 8003A24C from the same RAM, comparing all RAM outside the
     oracle stack, the per-actor 8015E9C8 stack frames, and the provider
     calls (event callback r13-0x6734 and group callbacks of 801778C0).
     In the original run 80039990 and 80039A18 return at entry (not
     reconstructed), the event and group callbacks return at entry (the
     event callback with the r3 the native hook returns), and 8017BB78 is
     served from the bank files (the ARAM read); everything else runs.
   - direct: the same edited RAM through the direct stream service, actor
     by actor, against the cache path: every list entry's frame record and
     all RAM except the stream cache state must match.
   Edits: manager records (states, keys, flags, delays, blend curves
   0-3 and their phase bytes), channels (state, delay, weight curve, key,
   bank, flags, frame pair, fraction, program state, synthetic programs of
   frames, 0x8000-0x8006, 0xA000 and event commands with and without
   0x1000, loop stack, roots, travel fields), the actor's +C flag 0x200,
   +304/+308, and the update length. Synthetic programs live at 80F10000
   (zero in the saved contexts). */
#include <windows.h>
#include "moderngekko/module_abi.h"
#include "scenery_draw.h"
#include "check_fixture.h"
#include "actor_channels.h"
#define ACTORS 14u
#define CASES 301u
#define PROGRAMS 0x80f10000u
#define LIST 0x8032bf28u
static const ModernGekkoModuleDesc *module;static uint8_t *oracle;static nflstreet_anim_aram aram;
static uint8_t *files[200];static size_t file_sizes[200];
static uint32_t gameplay[ACTORS],visual[ACTORS];
static uint32_t table_base(uint8_t *ram){return ram32(ram,ram32(ram,R13-0x7c30u));}
static void put16(uint8_t *ram,uint32_t a,uint32_t v){uint8_t *p=ram_at(ram,a,2);p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static void put32(uint8_t *ram,uint32_t a,uint32_t v){wr32(ram_at(ram,a,4),v);}
static void put8(uint8_t *ram,uint32_t a,uint32_t v){ram_at(ram,a,1)[0]=(uint8_t)v;}
static uint16_t get16(uint8_t *ram,uint32_t a){const uint8_t *p=ram_at(ram,a,2);return (uint16_t)(p[0]<<8|p[1]);}
/* ---- provider calls ---- */
#define LOG 8192u
typedef struct {uint32_t v[7];} call;
static call ev_n[LOG],ev_o[LOG],gr_n[LOG],gr_o[LOG];static unsigned ev_nn,ev_on,gr_nn,gr_on;static uint32_t ev_seed;
static uint32_t ev_value(unsigned i,uint32_t actor,uint32_t command,uint32_t key){
 uint32_t h=ev_seed^(i*0x9e3779b9u)^(command*0x85ebca6bu)^(key*0xc2b2ae35u)^actor;h^=h>>15;h*=0x2c1b3c6du;h^=h>>12;return (h>>7)&1u;}
static uint32_t native_event(void *u,nflstreet_actor_channels *c,uint32_t actor,uint32_t command,uint32_t key){(void)u;(void)c;
 if(ev_nn<LOG)ev_n[ev_nn]=(call){{actor,command,key}};const uint32_t v=ev_value(ev_nn,actor,command,key);++ev_nn;return v;}
static void native_group(void *u,nflstreet_actor_channels *c,uint32_t fn,uint32_t res,uint32_t key,uint32_t set,uint32_t ch,uint32_t actor,uint32_t mode){(void)u;(void)c;
 if(gr_nn<LOG)gr_n[gr_nn]=(call){{fn,res,key,set,ch,actor,mode}};++gr_nn;}
/* ---- the original ---- */
static uint8_t frames_seed[ACTORS][NFLSTREET_POSE_LOCAL_BYTES],frames_after[ACTORS][NFLSTREET_POSE_LOCAL_BYTES];static unsigned frames_seen;
static unsigned hits[16];
/* Dispatched blocks inside these ranges (calls are not always separate
   dispatches, so these count blocks, not calls): the slerp, acosf,
   rem_pio2f's medium path, the curves, retire, evict, hit, the operand
   reader, 8017428C, 80175AF8, the mirrored sample, the 2D rotation,
   80188EF8, and rem_pio2f's large path with 80217548. */
static const uint32_t counted[16]={0x801a383cu,0x80215844u,0x802172b8u,0x80178e7cu,0x80178f04u,0x80178f28u,0x80177a64u,0x80179900u,0x80179830u,0x80176dd0u,0x8017428cu,0x80175af8u,0x8015d584u,0x801de2acu,0x80188ef8u,0x80217420u};
static const uint32_t counted_end[16]={0x801a39a8u,0x80215a94u,0x80217400u,0x80178f04u,0x80178f28u,0x80179098u,0x80177accu,0x80179a14u,0x801798ccu,0x80176e28u,0x801742ccu,0x80175b84u,0x8015db28u,0x801de310u,0x80188f28u,0x80217dd4u};
static int run_update(float dt){
 CPUState c;memset(&c,0,sizeof c);c.ram=oracle;c.ram_size=RAM_BYTES;c.msr=0x2000;c.hid2=0xe0000000u;c.gqr[6]=0x0f070007;
 c.gpr[1]=STACK_HI-0x40u;c.gpr[2]=0x803b9a40u;c.gpr[13]=R13;c.fpr[1]=(double)dt;c.ps1[1]=(double)dt;c.pc=0x8003a24cu;
 for(unsigned step=0;c.pc;++step){
  if(step>50000000u){fprintf(stderr,"8003A24C did not return (pc %08x)\n",c.pc);return 0;}
  const uint32_t at=c.pc;
  for(unsigned k=0;k<16;++k)if(at>=counted[k]&&at<counted_end[k]&&!(k==15&&at>=0x80217510u&&at<0x80217548u))++hits[k];
  if(at==0x80039990u||at==0x80039a18u){c.pc=c.lr;continue;}
  if(c.lr==0x80173a64u&&at!=0x80173a64u){ /* the event callback (8001532C) returns at entry */
   const uint32_t v=ev_value(ev_on,c.gpr[3],c.gpr[4],c.gpr[5]);if(ev_on<LOG)ev_o[ev_on]=(call){{c.gpr[3],c.gpr[4],c.gpr[5]}};++ev_on;
   c.gpr[3]=v;c.pc=c.lr;continue;}
  if(c.lr==0x8017793cu&&at!=0x8017793cu){ /* a group callback returns at entry */
   if(gr_on<LOG)gr_o[gr_on]=(call){{at,c.gpr[3],c.gpr[4],c.gpr[5],c.gpr[6],c.gpr[7],c.gpr[8]}};++gr_on;c.gpr[3]=0;c.pc=c.lr;continue;}
  if(at==0x8017bb78u){uint8_t *to=ram_at(oracle,c.gpr[3],c.gpr[5]);if(!to){fprintf(stderr,"ARAM read to %08x\n",c.gpr[3]);return 0;}
   nflstreet_anim_aram_read(&aram,c.gpr[4],to,c.gpr[5]);c.pc=c.lr;continue;}
  if(at==0x8015e9c8u){unsigned i=0;while(i<ACTORS&&visual[i]!=c.gpr[3])++i;if(i==ACTORS){fprintf(stderr,"blend of an unknown visual %08x\n",c.gpr[3]);return 0;}
   memcpy(ram_at(oracle,c.gpr[1]-0x218u+8u,NFLSTREET_POSE_LOCAL_BYTES),frames_seed[i],NFLSTREET_POSE_LOCAL_BYTES);}
  if(at==0x801784b8u&&c.lr==0x8003a3b0u){ /* after 800D4CBC: 8015E9C8's frame is still on the stack */
   unsigned i=0;const uint32_t r31=c.gpr[31];while(i<ACTORS&&gameplay[i]!=r31)++i;if(i==ACTORS){fprintf(stderr,"unknown actor after the blend\n");return 0;}
   memcpy(frames_after[i],ram_at(oracle,c.gpr[1]-0x8u-0x218u+8u,NFLSTREET_POSE_LOCAL_BYTES),NFLSTREET_POSE_LOCAL_BYTES);++frames_seen;}
  if(at>=0x80200000u&&at<0x80210000u){fprintf(stderr,"unexpected SDK entry %08x (lr %08x)\n",at,c.lr);return 0;}
  c.downcount=-1000000000;module->dispatch(&c,c.pc);
  if(c.exception){fprintf(stderr,"exception at %08x (lr %08x)\n",c.pc,c.lr);return 0;}}
 return 1;}
static int same(const uint8_t *native,const uint8_t *mask){
 const uint32_t lo=STACK_LO-RAM_BASE,hi=STACK_HI-RAM_BASE;
 unsigned bad=0;
 for(uint32_t i=0;i<RAM_BYTES;++i)if((i<lo||i>=hi)&&(!mask||!mask[i])&&oracle[i]!=native[i]){
  if(bad<24)fprintf(stderr,"RAM differs at %08x: expected %02x native %02x\n",RAM_BASE+i,oracle[i],native[i]);++bad;}
 return !bad;}
/* ---- edits ---- */
static const uint16_t KEYS[]={0x96,0x93,0x4a,0xa0,0x31,0x1e,0x55,0x4d,0x38,0xfffe};
static uint32_t bank_frames(uint8_t *ram,unsigned b){const uint32_t t=ram32(ram,R13-0x4f6cu);if(b>=ram32(ram,R13-0x4f70u))return 0;
 const uint32_t e=t+0x1cu*b;if(get16(ram,e+0x14)==0x8000u||!ram32(ram,e+0xc))return 0;return get16(ram,ram32(ram,e+0xc)+4);}
static unsigned synthetic(uint8_t *ram,uint32_t at,unsigned frames,unsigned *start){
 uint16_t w[64];unsigned n=0,depth=0,top[64],nt=0;const unsigned len=6+rnd()%36;
#define FRAME (uint16_t)(rnd()%(frames?frames:1)&0x7fffu)
 while(n<len){const unsigned r=rnd()%20;if(!depth)top[nt++]=n;
  if(r<9||!n)w[n++]=FRAME;
  else if(r==9&&depth<2){w[n++]=0x8002;++depth;}
  else if(r==10&&depth){w[n++]=0x8003;w[n++]=(uint16_t)(rnd()%4|(rnd()%6==0?0x800u:0u));--depth;}
  else if(r==11){w[n++]=0x8004;w[n++]=rnd()%8?(uint16_t)(rnd()%2?rnd()%24:0x1000u-1u-rnd()%24):(uint16_t)(0x1000u|(rnd()%2?0u:0xfffu));
   if(w[n-1]&0x1000u){const uint16_t hi=w[n-1];w[n++]=(uint16_t)(hi&0x800u?0x10000u-1u-rnd()%24:rnd()%24);}}
  else if(r==12)w[n++]=rnd()%2?0x8005:0x8006;
  else if(r==13){w[n++]=(uint16_t)(0xa000u|(rnd()%2?0x1000u:0u)|(rnd()&0xfffu));if(w[n-1]&0x1000u)w[n++]=FRAME;}
  else if(r==14||r==15){w[n++]=(uint16_t)(0xc000u|(rnd()%2?0x1000u:0u)|(rnd()&0x3fu)|(rnd()%4==0?0x2000u:0u));if(rnd()%2)w[n++]=FRAME;}
  else if(r==16)w[n++]=(uint16_t)(0x8007u+rnd()%0x1ff9u);
  else w[n++]=FRAME;}
 while(depth){w[n++]=FRAME;w[n++]=0x8003;w[n++]=(uint16_t)(rnd()%3);--depth;}
 top[nt++]=n;w[n++]=FRAME;w[n++]=rnd()%3?0x8001:0x8000;
#undef FRAME
 *start=top[rnd()%nt];put32(ram,at,0x20000000u|rnd()%0x100u);for(unsigned i=0;i<n;++i)put16(ram,at+4+2*i,w[i]);return n;}
static const unsigned BANKS[]={0,1,2,3,7,8,9,10,11,12,13,14,15,2,2,13};
static unsigned empty[128],nempty; /* bank table entries without a header (80175F48 returns 0) */
static void edit_curve(uint8_t *ram,uint32_t b){
 const unsigned mode=rnd()%5==0?0:rnd()%4;put32(ram,b+8,mode);
 put32(ram,b,fbits(rnd()%6==0?0.0f:rndf(0.0f,1.2f)));put32(ram,b+4,fbits(rnd()%3==0?0.0f:rnd()%2?1.0f:rndf(0.0f,1.0f)));
 put32(ram,b+0xc,fbits(rnd()%4==0?0.0f:rndf(-0.5f,1.0f)));
 if(mode==1)put32(ram,b+0x10,1+rnd()%20);
 else if(mode==2)put32(ram,b+0xc,fbits(rndf(-1.0f,2.5f)));
 else if(mode==3){put32(ram,b+0x10,fbits(rndf(-1.5f,3.0f)));put8(ram,b+0x14,rnd()%5==0?0:1+rnd()%10);put8(ram,b+0x15,rnd()%4);put16(ram,b+0x16,rnd()%8);}
 if(rnd()%6==0){const uint32_t k=ram32(ram,0x8026117cu);put32(ram,b,k);put32(ram,b+4,k);} /* exactly the retire / stop threshold (1e-7) */}
static void edit_actor(uint8_t *ram,uint32_t g){
 const uint32_t m=g+0x9bc,ch=g+0x4e4;uint16_t live[4];unsigned nl=0;
 if(rnd()%4==0)put32(ram,g+0xc,ram32(ram,g+0xc)^0x200u);
 if(rnd()%3==0)put32(ram,g+0x304,fbits(rnd()%2?1.0f:rndf(0.6f,1.4f)));if(rnd()%3==0)put8(ram,g+0x308,rnd()%4);
 for(unsigned k=0;k<4;++k){const uint32_t r=m+100u*k;
  if(rnd()%2){put16(ram,r+6,rnd()%5==0?0:1+rnd()%3);put16(ram,r+4,KEYS[rnd()%10]);put16(ram,r+8,rnd()%2);
   put32(ram,r+0xc,fbits(rnd()%3==0?0.0f:rndf(-1.5f,1.0f)));edit_curve(ram,r+0x50);}
  else if(rnd()%2)put32(ram,r+0xc,fbits(rndf(-1.5f,0.5f)));}
 for(unsigned k=0;k<4;++k)if(get16(ram,m+100u*k+6))live[nl++]=get16(ram,m+100u*k+4);
 for(unsigned k=0;k<10;++k){const uint32_t c=ch+0x7cu*k;uint8_t *p=ram_at(ram,c,0x7c);
  if(rnd()%3==0){if(p[1]&&nl){int found=0;for(unsigned j=0;j<nl;++j)found|=get16(ram,c+8)==live[j];if(!found)put16(ram,c+8,live[rnd()%nl]);}
   else if(p[1]&&!nl)p[1]=0;
   if(rnd()%2)put32(ram,c+0x58,fbits(rndf(-2.5f,1.0f)));continue;}
  p[1]=nl?(uint8_t)(rnd()%3):0;if(nl)put16(ram,c+8,live[rnd()%nl]);
  put32(ram,c+0x48,fbits(rnd()%4==0?rndf(-1.0f,1.5f):0.0f));edit_curve(ram,c+0x30);
  const unsigned bank=rnd()%8==0&&nempty?empty[rnd()%nempty]:BANKS[rnd()%16];p[6]=(uint8_t)bank;const unsigned frames=bank_frames(ram,bank);
  p[7]=(uint8_t)(rnd()%3==0?rnd()&0x37u:(uint32_t)(const uint8_t[]){0,1,2,4,6,0x10,0x11,0x20,0x27,0x17,0x14,0x22}[rnd()%12]);
  put16(ram,c+0xa,rnd()%8==0?rnd()%4:0xffff);
  for(unsigned w=0;w<3;++w)put32(ram,c+0x10+4*w,rnd()%3==0?rnd()<<8^rnd():(rnd()%3==0?rnd()<<9:rnd())&0xffffffu);
  put32(ram,c+0x1c,rnd()%2?rnd()&0xffffffu:0);put32(ram,c+0x20,rnd()%3?0x01000000u:rnd()&0x1ffffffu);put32(ram,c+0xc,rnd()<<8^rnd());
  put32(ram,c+0x24,fbits(rndf(-5.0f,5.0f)));put32(ram,c+0x28,fbits(rndf(-5.0f,5.0f)));put32(ram,c+0x2c,fbits(rnd()%3?1.0f:rndf(0.0f,2.0f)));
  if(rnd()%3||!ram32(ram,c+0x4c)){const uint32_t at=PROGRAMS+0x80u*(10u*(unsigned)((g-table_base(ram))/0x14d4u)+k);unsigned start;const unsigned n=synthetic(ram,at,frames,&start);
   put32(ram,c+0x4c,at);put16(ram,c+0x64,start);put16(ram,c+0x66,rnd()%3?0xffff:rnd()%n);put16(ram,c+0x68,rnd()%40);
   p[0x6b]=0;for(unsigned d=0;d<2;++d){put16(ram,c+0x6cu+8*d,rnd()%n);put16(ram,c+0x6eu+8*d,rnd()%4);put32(ram,c+0x70u+8*d,fbits(rndf(0.0f,30.0f)));}}
  put32(ram,c+0x50,fbits(rnd()%6==0?1.0f:rndf(0.3f,3.0f)));put32(ram,c+0x54,fbits(rndf(0.0f,40.0f)));put32(ram,c+0x58,fbits(rndf(-2.5f,1.5f)));
  const unsigned span=frames?frames+(rnd()%6==0?3u:0u):1u;put16(ram,c+0x5c,rnd()%span);put16(ram,c+0x5e,rnd()%span);
  put32(ram,c+0x60,fbits(rnd()%5==0?(float)(rnd()%2):rndf(0.0f,1.0f)));p[0x6a]=(uint8_t)(rnd()&0x3fu);}}
/* ---- frames ---- */
static int frames_check(uint8_t *ram,unsigned *checked,unsigned *stale){
 const uint32_t t=ram32(ram,R13-0x4f6cu),n=ram32(ram,R13-0x4f70u);uint32_t caches[4];unsigned nc=0;
 (void)n;for(unsigned m=0;m<aram.count;++m){const uint32_t cache=ram32(ram,t+0x1cu*aram.bank[m].index+0x18);int seen=0;if(!ram_at(ram,cache,0x20))continue;for(unsigned k=0;k<nc;++k)seen|=caches[k]==cache;if(!seen&&nc<4)caches[nc++]=cache;}
 static uint8_t scratch[RAM_BYTES];memcpy(scratch,ram,RAM_BYTES);
 for(unsigned k=0;k<nc;++k){const uint32_t cache=caches[k],slots=get16(ram,cache),size=get16(ram,cache+2),records=ram32(ram,cache+4),buffers=ram32(ram,cache+8);
  for(uint32_t s=0;s<slots;++s){const uint32_t r=records+0x14u*s;if(ram_at(ram,r+0xf,1)[0]!=1)continue;
   const unsigned bank=ram_at(ram,r+0xe,1)[0],frame=get16(ram,r+0xc);const uint32_t header=ram32(ram,t+0x1cu*bank+0xc),desc=ram32(ram,header+0x14);
   size_t avail;const uint8_t *src=nflstreet_anim_aram_frame(&aram,bank,frame,&avail);if(!src){fprintf(stderr,"no file frame for bank %u frame %u\n",bank,frame);return 0;}
   if(ram32(ram,ram32(ram,header+0x10)+4*frame)!=(0xf0000000u|s)){++*stale;continue;} /* the saved table no longer names this slot */
   /* the slot's saved ARAM word reads the same bytes through the service */
   {uint8_t probe[16];nflstreet_anim_aram_read(&aram,ram32(ram,r+8),probe,16);if(memcmp(probe,src,avail<16?avail:16)){fprintf(stderr,"slot %u ARAM word does not map to its frame\n",s);return 0;}}
   memcpy(ram_at(scratch,0x80f20000u,0x100),src,avail<0x100?avail:0x100);
   nflstreet_actor_pose_context c;memset(&c,0,sizeof c);c.span=span;c.memory=scratch;
   if(!nflstreet_actor_pose_stream(&c,0x80f21000u,0x80f20000u,desc,0)){fprintf(stderr,"decode failed\n");return 0;}
   const uint8_t first=src[0];const uint32_t len=10u+((first&0x40)?6u:0u)+((first&0x80)?6u:0u)+2u*ram32(ram,desc);
   if(len>size||memcmp(ram_at(scratch,0x80f21000u,len),ram_at(ram,buffers+s*size,len),len)){fprintf(stderr,"cache slot %u (bank %u frame %u) differs from the file's frame\n",s,bank,frame);return 0;}
   ++*checked;}}
 return 1;}
/* ---- update ---- */
static unsigned up_large,up_runs,up_events,up_groups,up_skip,up_dt[3];
static void ctx_init(nflstreet_actor_channels *c,uint8_t *ram,int direct){
 memset(c,0,sizeof *c);c->span=span;c->memory=ram;c->aram=&aram;c->direct=direct;c->event=native_event;c->group=native_group;
 for(unsigned i=0;i<ACTORS;++i)memcpy(c->pose[i].local,frames_seed[i],NFLSTREET_POSE_LOCAL_BYTES);}
static int prepare_case(uint8_t *pristine,uint8_t *ram,unsigned index,float *dt){
 memcpy(ram,pristine,RAM_BYTES);*dt=1.0f;ev_seed=index*2654435761u+7u;
 for(unsigned i=0;i<ACTORS;++i)for(unsigned b=0;b<NFLSTREET_POSE_LOCAL_BYTES;++b)frames_seed[i][b]=index?(uint8_t)rnd():0;
 if(!index)return 1;
 const unsigned r=rnd()%6;*dt=r<3?1.0f:r==3?2.0f:r==4?0.5f:rndf(0.0f,3.0f);up_dt[r<3?0:r<5?1:2]++;
 for(unsigned i=0;i<ACTORS;++i)if(rnd()%5)edit_actor(ram,gameplay[i]);
 return 1;}
static int update_case(uint8_t *pristine,uint8_t *ram,uint8_t *before,unsigned index){
 float dt;prepare_case(pristine,ram,index,&dt);memcpy(before,ram,RAM_BYTES);
 static nflstreet_actor_channels c;ctx_init(&c,ram,0);ev_nn=gr_nn=0;
 if(!nflstreet_actor_channels_update(&c,dt)){ /* only rem_pio2f's large-argument path (80217548) may reject */
  memcpy(oracle,before,RAM_BYTES);ev_on=gr_on=0;frames_seen=0;const unsigned large=hits[15];
  const int ran=run_update(dt);if(ran&&hits[15]>large){++up_large;return 1;}fprintf(stderr,"original ran %d, kernel %u; hits",ran,hits[15]-large);for(unsigned k=0;k<16;++k)fprintf(stderr," %08x:%u",counted[k],hits[k]);fprintf(stderr,"\n");
  fprintf(stderr,"native update rejected (case %u)\n",index);return 0;}
 memcpy(oracle,before,RAM_BYTES);ev_on=gr_on=0;frames_seen=0;
 if(!run_update(dt)){fprintf(stderr,"original update failed (case %u)\n",index);return 0;}
 ++up_runs;
 if(ev_nn!=ev_on||gr_nn!=gr_on){fprintf(stderr,"provider calls differ (case %u): events %u/%u groups %u/%u\n",index,ev_nn,ev_on,gr_nn,gr_on);return 0;}
 for(unsigned i=0;i<ev_nn&&i<LOG;++i)if(memcmp(&ev_n[i],&ev_o[i],sizeof(call))){fprintf(stderr,"event call %u differs (case %u)\n",i,index);return 0;}
 for(unsigned i=0;i<gr_nn&&i<LOG;++i)if(memcmp(&gr_n[i],&gr_o[i],sizeof(call))){fprintf(stderr,"group call %u differs (case %u): fn %08x/%08x\n",i,index,gr_n[i].v[0],gr_o[i].v[0]);return 0;}
 up_events+=ev_nn;up_groups+=gr_nn;
 for(unsigned i=0;i<ev_nn&&i<LOG;++i)up_skip+=!ev_value(i,ev_n[i].v[0],ev_n[i].v[1],ev_n[i].v[2])&&(ev_n[i].v[1]&0x1000u);
 if(frames_seen!=ACTORS){fprintf(stderr,"saw %u blend frames (case %u)\n",frames_seen,index);return 0;}
 for(unsigned i=0;i<ACTORS;++i)for(unsigned b=0;b<NFLSTREET_POSE_LOCAL_BYTES;++b)if((b<0x30||b>=0x34)&&frames_after[i][b]!=c.pose[i].local[b]){
  fprintf(stderr,"actor %u blend frame differs at +%03x (case %u)\n",i,b,index);return 0;}
 if(!same(ram,NULL)){fprintf(stderr,"update mismatch case %u\n",index);
  return 0;}
 return 1;}
/* ---- direct ---- */
static uint8_t mask[RAM_BYTES];
static void cache_mask(uint8_t *ram){
 memset(mask,0,sizeof mask);const uint32_t t=ram32(ram,R13-0x4f6cu),n=ram32(ram,R13-0x4f70u);
#define MARK(a,len) do{uint8_t *q_=ram_at(mask,(a),(len));if(q_)memset(q_,1,(len));}while(0)
 (void)n;for(unsigned m=0;m<aram.count;++m){const uint32_t e=t+0x1cu*aram.bank[m].index,h=ram32(ram,e+0xc),cache=ram32(ram,e+0x18);
  MARK(ram32(ram,h+0x10),4u*get16(ram,h+4));if(cache){MARK(cache,0x20);MARK(ram32(ram,cache+4),0x14u*get16(ram,cache));MARK(ram32(ram,cache+8),(uint32_t)get16(ram,cache+2)*get16(ram,cache));}}
 MARK(0x8030b8e0u,0x180);MARK(R13-0x5968u,1);for(unsigned i=0;i<0x18;++i)MARK(LIST+4+0x2cu*i+0x28,4);
#undef MARK
}
static unsigned di_runs,di_frames,di_large,di_stale;
static int direct_case(uint8_t *pristine,uint8_t *ram,uint8_t *other,unsigned index){
 float dt;prepare_case(pristine,ram,index,&dt);memcpy(other,ram,RAM_BYTES);cache_mask(pristine);
 static nflstreet_actor_channels a,b;ctx_init(&a,ram,0);ctx_init(&b,other,1);ev_nn=0;
 const uint32_t base=table_base(ram);
 for(unsigned i=0;i<ACTORS;++i){ev_nn=0;if(!nflstreet_actor_channels_manager(&a,dt,base+0x14d4u*i))return 0;ev_nn=0;if(!nflstreet_actor_channels_manager(&b,dt,base+0x14d4u*i))return 0;}
 for(unsigned i=0;i<ACTORS;++i){const unsigned e0=ev_nn;if(!nflstreet_actor_channels_step(&a,dt,base+0x14d4u*i))return 0;ev_nn=e0;if(!nflstreet_actor_channels_step(&b,dt,base+0x14d4u*i))return 0;}
 for(unsigned i=0;i<ACTORS;++i){
  {const int ra=nflstreet_actor_channels_pose(&a,i,base+0x14d4u*i),rb=nflstreet_actor_channels_pose(&b,i,base+0x14d4u*i);
   if(!ra&&!rb){++di_large;return 1;} /* both stop at the same unreconstructed path (kernel_rem_pio2f) */
   if(!ra||!rb){fprintf(stderr,"direct pose rejected (case %u): cache %d direct %d\n",index,ra,rb);return 0;}}
  const unsigned n=get16(ram,LIST);
  for(unsigned k=0;k<n;++k){const uint32_t e=LIST+4+0x2cu*k,fa=ram32(ram,e+0x28),fb=ram32(other,e+0x28);
   if(fb<NFLSTREET_CHANNEL_DIRECT){if(fa!=fb){fprintf(stderr,"resident frame differs (case %u)\n",index);return 0;}continue;}
   {const uint32_t cache=ram32(ram,ram32(ram,R13-0x4f6cu)+0x1cu*get16(ram,e+0x20)+0x18),size=get16(ram,cache+2),slot=(fa-ram32(ram,cache+8))/size,rec=ram32(ram,cache+4)+0x14u*slot;
    const uint32_t hdr=ram32(ram,ram32(ram,R13-0x4f6cu)+0x1cu*get16(ram,e+0x20)+0xc),fr=get16(ram,e+0x22)>=get16(ram,hdr+4)?0u:get16(ram,e+0x22); /* 801749E8 clamps */
    if(get16(ram,rec+0xc)!=fr||ram_at(ram,rec+0xe,1)[0]!=get16(ram,e+0x20)){++di_stale;return 1;}} /* the cache served a stale slot (saved table word naming another frame's slot): the original's frame is not the file's */
   const uint8_t *pa=ram_at(ram,fa,0xb2),*pb=b.host+(fb-NFLSTREET_CHANNEL_DIRECT);if(!pa){fprintf(stderr,"cache frame outside RAM\n");return 0;}
   const uint32_t desc=ram32(ram,ram32(ram,ram32(ram,R13-0x4f6cu)+0x1cu*get16(ram,e+0x20)+0xc)+0x14);
   const uint32_t len=10u+((pa[0]&0x40)?6u:0u)+((pa[0]&0x80)?6u:0u)+2u*ram32(ram,desc);
   if(len>0xb2||memcmp(pa,pb,len)){fprintf(stderr,"direct frame differs (case %u actor %u entry %u)\n",index,i,k);return 0;}++di_frames;}}
 memcpy(oracle,ram,RAM_BYTES);if(!same(other,mask)){fprintf(stderr,"direct mismatch case %u\n",index);return 0;}++di_runs;return 1;}
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
 uint8_t *pristine=load_ram(argv[1]),*ram=malloc(RAM_BYTES),*before=malloc(RAM_BYTES);oracle=malloc(RAM_BYTES);
 if(!pristine||!ram||!before||!oracle)return 3;
 const int mapped=nflstreet_anim_aram_map(&aram,span,pristine,(const uint8_t *const *)files,file_sizes,200);
 if(mapped<=0){fprintf(stderr,"stream banks not mapped (%d)\n",mapped);return 4;}
 const uint32_t base=table_base(pristine);if(get16(pristine,ram32(pristine,R13-0x7c30u)+10)!=ACTORS)return 4;
 for(unsigned i=0;i<ACTORS;++i){gameplay[i]=base+0x14d4u*i;visual[i]=ram32(pristine,gameplay[i]+4);}
 {const uint32_t t=ram32(pristine,R13-0x4f6cu),n=ram32(pristine,R13-0x4f70u);for(uint32_t i=0;i<n&&nempty<127;++i)if(!ram32(pristine,t+0x1cu*i+0xc)||get16(pristine,t+0x1cu*i+0x14)==0x8000u)empty[nempty++]=i;
  empty[nempty++]=n+1u;} /* past the table too */
 unsigned slots=0,stale=0;if(!frames_check(pristine,&slots,&stale))return 5;
 uint32_t digest=2166136261u;
 for(unsigned i=0;i<CASES;++i){if(!update_case(pristine,ram,before,i))return 6;digest=fnv(digest,ram,RAM_BYTES);}
 for(unsigned i=0;i<CASES;++i){if(!direct_case(pristine,ram,before,i))return 7;}
 /* Soak (plausibility, native only, no providers): 600 updates of all 14
    actors from the saved state. */
 unsigned soak=0,active=0,moving=0;{memcpy(ram,pristine,RAM_BYTES);static nflstreet_actor_channels c;memset(&c,0,sizeof c);c.span=span;c.memory=ram;c.aram=&aram;
  static uint8_t pose_then[ACTORS][180];
  for(;soak<600;++soak){if(soak==540)for(unsigned i=0;i<ACTORS;++i)memcpy(pose_then[i],ram_at(ram,ram32(ram,visual[i]+0x5c),180),180);
   if(!nflstreet_actor_channels_update(&c,1.0f))break;}
  for(unsigned i=0;i<ACTORS;++i){unsigned n=0;for(unsigned k=0;k<10;++k)n+=ram_at(ram,gameplay[i]+0x4e4u+0x7cu*k+1,1)[0]!=0;active+=n>0;
   moving+=memcmp(pose_then[i],ram_at(ram,ram32(ram,visual[i]+0x5c),180),180)!=0;}}
 printf("Actor channels soak: %u native updates of all 14 actors from the saved state without providers; %u actors keep active channels, %u poses changed over the last 60.\n",soak,active,moving);
 printf("Actor channels: %d streamed banks mapped to their files; %u saved stream cache slots equal the service's decoded frames (%u slots the saved frame tables no longer name); "
  "%u runs of 8003A24C (dt 1/2-or-0.5/random %u/%u/%u) match all RAM, the 8015E9C8 frames and %u event and %u group callback calls "
  "(%u skipped operands), %u cases rejected at kernel_rem_pio2f as the original enters it; original blocks: slerp %u, acosf %u, rem_pio2f %u, curves %u/%u/%u, retire %u, evict %u, hit %u, "
  "operand %u, 8017428C %u, resources %u, mirrored blend %u, rotate %u, 80188EF8 %u, kernel_rem_pio2f %u; direct service: %u runs (%u stopped where the cache path stops, %u where the cache serves a stale saved slot), "
  "%u frames equal the cache's; digest %08x.\n",mapped,slots,stale,up_runs,up_dt[0],up_dt[1],up_dt[2],up_events,up_groups,up_skip,up_large,
  hits[0],hits[1],hits[2],hits[3],hits[4],hits[5],hits[6],hits[7],hits[8],hits[9],hits[10],hits[11],hits[12],hits[13],hits[14],hits[15],di_runs,di_large,di_stale,di_frames,digest);
 return 0;}
