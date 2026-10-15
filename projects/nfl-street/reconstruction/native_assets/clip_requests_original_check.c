/* Integration check. Only the external event and gameplay group callbacks
   return at entry; no native implementation substitutes for an original body.
   Each actor/request gets its own prepared MEM1, native copy, original copy,
   full comparison outside the one declared oracle stack, and provider trace. */
#include <windows.h>
#include "moderngekko/module_abi.h"
#include "clip_requests.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define RAM_BASE 0x80000000u
#define RAM_BYTES (24u*1024u*1024u)
#define R13 0x803a9a40u
#define STACK_LO 0x80f38000u
#define STACK_HI 0x80f47000u
#define SCRATCH 0x80f10000u
#define CATALOG 0x80389e20u
#define CASES 301u
#define LOG 32768u
static const ModernGekkoModuleDesc *module;static uint8_t *oracle;
static uint32_t rng=0x13579bdu;
static uint32_t rnd(void){rng=rng*1103515245u+12345u;return rng>>8;}
static float rndf(float lo,float hi){return lo+(hi-lo)*(float)(rnd()&65535u)/65535.0f;}
static uint8_t *at(uint8_t *m,uint32_t a,uint32_t n){return a>=RAM_BASE&&(uint64_t)a-RAM_BASE+n<=RAM_BYTES?m+a-RAM_BASE:NULL;}
static uint8_t *span(void *m,uint32_t a,uint32_t n){if(a<STACK_HI&&(uint64_t)a+n>STACK_LO)return NULL;return at(m,a,n);}
static uint32_t word(uint8_t *m,uint32_t a){const uint8_t *p=at(m,a,4);return p?nflstreet_rd32(p):0;}
static uint16_t half(uint8_t *m,uint32_t a){const uint8_t *p=at(m,a,2);return p?nflstreet_rd16(p):0;}
static void p32(uint8_t *m,uint32_t a,uint32_t v){nflstreet_wr32(at(m,a,4),v);}
static void p16(uint8_t *m,uint32_t a,uint32_t v){nflstreet_wr16(at(m,a,2),v);}
static void pf(uint8_t *m,uint32_t a,float v){p32(m,a,nflstreet_float_bits(v));}
static uint32_t hash(uint32_t h,const void *p,size_t n){const uint8_t *b=p;for(size_t i=0;i<n;++i)h=(h^b[i])*16777619u;return h;}
typedef struct {uint32_t v[8];} call;
static call native_log[LOG],original_log[LOG];static unsigned nn,on;
static unsigned event_calls,scan_calls,group_calls,operands;
static uint32_t event_result(uint32_t actor,uint32_t cmd,uint32_t key){return ((actor>>2)^cmd^key)&1u;}
static int consume(uint32_t cmd){return (cmd&0x1000u)!=0;}
static uint32_t native_event(void *u,nflstreet_clip_requests *c,uint32_t actor,uint32_t cmd,uint32_t key,int scan){(void)u;
 uint32_t value=0;if(consume(cmd)&&!nflstreet_clip_request_operand(c,&value))return 0;
 if(nn<LOG)native_log[nn]=(call){{1,actor,cmd,key,(unsigned)scan,value}};++nn;return event_result(actor,cmd,key);}
static void native_group(void *u,nflstreet_clip_requests *c,uint32_t fn,uint32_t res,uint32_t key,uint32_t set,uint32_t ch,uint32_t actor,uint32_t mode){(void)u;(void)c;
 if(nn<LOG)native_log[nn]=(call){{2,fn,res,key,set,ch,actor,mode}};++nn;}
/* The fixture provider consumes through the ORIGINAL 801777A0/80176DD0.
   It then returns at the external callback entry; no stream helper is stubbed. */
static int original_operand(const CPUState *caller,uint32_t *value){CPUState c=*caller;c.pc=0x801777a0u;c.lr=0;
 for(unsigned step=0;c.pc;++step){if(step>1000u)return 0;c.downcount=-1000000000;module->dispatch(&c,c.pc);if(c.exception)return 0;}
 *value=c.gpr[3];return 1;}
/* Counts are dispatch blocks, except callbacks, not dynamic function entries. */
static unsigned branch_hits[8];
static const uint32_t branch_pc[8]={0x801789c4u,0x801789d4u,0x80178ae0u,0x80178b00u,0x80178cd0u,0x80177b90u,0x801770fcu,0x80173c24u};
typedef struct {uint32_t a[7];float weight;unsigned mode;uint32_t pc;} request;
static int run_original(const request *q,int32_t *ret){CPUState c;memset(&c,0,sizeof c);c.ram=oracle;c.ram_size=RAM_BYTES;
 c.msr=0x2000;c.hid2=0xe0000000u;c.gqr[6]=0x0f070007;c.gpr[1]=STACK_HI-0x40u;c.gpr[2]=0x803b9a40u;c.gpr[13]=R13;
 for(unsigned i=0;i<7;++i)c.gpr[3+i]=q->a[i];c.fpr[1]=q->weight;c.ps1[1]=q->weight;c.pc=q->pc;
 for(unsigned step=0;c.pc;++step){const uint32_t pc=c.pc;if(step>10000000u){fprintf(stderr,"%08x stuck at %08x lr %08x\n",q->pc,pc,c.lr);return 0;}
  for(unsigned i=0;i<8;++i)if(pc==branch_pc[i])++branch_hits[i];
  if((c.lr==0x80173a64u||c.lr==0x80173a98u)&&pc!=c.lr){
   const unsigned scan=c.lr==0x80173a98u;uint32_t value=0;if(consume(c.gpr[4])){if(!original_operand(&c,&value))return 0;++operands;}
   if(on<LOG)original_log[on]=(call){{1,c.gpr[3],c.gpr[4],c.gpr[5],scan,value}};++on;
   if(scan)++scan_calls;else ++event_calls;c.gpr[3]=event_result(c.gpr[3],c.gpr[4],c.gpr[5]);c.pc=c.lr;continue;}
  if(c.lr==0x8017793cu&&pc!=c.lr){if(on<LOG)original_log[on]=(call){{2,pc,c.gpr[3],c.gpr[4],c.gpr[5],c.gpr[6],c.gpr[7],c.gpr[8]}};++on;++group_calls;c.gpr[3]=0;c.pc=c.lr;continue;}
  if(pc>=0x80200000u&&pc<0x80210000u){fprintf(stderr,"unexpected SDK %08x lr %08x\n",pc,c.lr);return 0;}
  c.downcount=-1000000000;module->dispatch(&c,c.pc);if(c.exception){fprintf(stderr,"exception at %08x lr %08x\n",c.pc,c.lr);return 0;}}
 *ret=(int32_t)c.gpr[3];return 1;}
static int same(uint8_t *native){const uint32_t lo=STACK_LO-RAM_BASE,hi=STACK_HI-RAM_BASE;
 if(memcmp(native,oracle,lo)||memcmp(native+hi,oracle+hi,RAM_BYTES-hi)){
  unsigned n=0;for(uint32_t i=0;i<RAM_BYTES&&n<16;++i)if((i<lo||i>=hi)&&native[i]!=oracle[i]){
   fprintf(stderr,"RAM %08x original=%02x native=%02x words=%08x/%08x\n",RAM_BASE+i,oracle[i],native[i],word(oracle,(RAM_BASE+i)&~3u),word(native,(RAM_BASE+i)&~3u));++n;}return 0;}
 if(nn!=on||nn>LOG||memcmp(native_log,original_log,nn*sizeof(call))){fprintf(stderr,"callback trace differs native=%u original=%u\n",nn,on);
  for(unsigned i=0;i<nn&&i<on&&i<LOG;++i)if(memcmp(&native_log[i],&original_log[i],sizeof(call))){fprintf(stderr,"callback %u\n",i);for(unsigned j=0;j<8;++j)fprintf(stderr," %08x/%08x",original_log[i].v[j],native_log[i].v[j]);fprintf(stderr,"\n");break;}return 0;}return 1;}
static uint32_t banks[256],nbanks;static uint16_t keys[8192];static unsigned nkeys;
static int catalog(uint8_t *m,uint32_t id){nbanks=nkeys=0;const uint32_t t=word(m,R13-0x4f6cu),count=word(m,R13-0x4f70u);
 for(unsigned i=0;i<count&&i<256;++i){const uint32_t e=t+28*i,h=word(m,e+12);if(!h||half(m,e+0x14)!=id||at(m,e+0x12,1)[0]!=1)continue;
  const uint32_t clips=word(m,h+12),n=word(m,clips);if(n&&n<10000&&at(m,clips,4+4*n))banks[nbanks++]=i;
  const uint32_t rows=word(m,h+8),nr=word(m,rows);if(nr>8192)return 0;
  for(unsigned j=0;j<nr&&nkeys<8192;++j){const uint16_t key=half(m,rows+4+16*j);if(key&0x8000u)continue;
   const uint32_t rec=word(m,R13-0x4f48u)+20*id;if(key>=half(m,rec+4))continue;int found=0;for(unsigned k=0;k<nkeys;++k)found|=keys[k]==key;if(!found)keys[nkeys++]=key;}}
 return nbanks&&nkeys;}
static uint32_t header(uint8_t *m,uint32_t bank){return word(m,word(m,R13-0x4f6cu)+28*bank+12);}
static uint32_t key(void){return keys[rnd()%nkeys];}
static uint32_t choose_clip(uint8_t *m,uint32_t bank){return rnd()%word(m,word(m,header(m,bank)+12));}
static void synthetic(uint8_t *m,uint32_t program){ /* finite, with operand events and a rate command */
 p32(m,program,(1+rnd()%255)<<24|((rnd()&0x37u)<<8));const uint16_t code[]={0xc021,0xd022,3,0,0x8004,2,1,0x8002,2,0x8003,2,3,0xb001,4,5,0x8006,6,0x8000};
 for(unsigned i=0;i<sizeof code/sizeof code[0];++i)p16(m,program+4+2*i,code[i]);}
static unsigned prepared[12];
static void prepare(uint8_t *m,uint32_t g,unsigned index,unsigned mode,request *q){
 const uint32_t mgr=word(m,g+0x318),set=word(m,g+0x31c),ch=word(m,g+0x320);uint32_t current=0xffff;
 for(int i=3;i>=0;--i)if(half(m,mgr+100u*(unsigned)i+6)==3)current=half(m,mgr+100u*(unsigned)i+4);
 unsigned slot=0;for(unsigned i=0;i<half(m,set+4);++i)if(at(m,ch+0x7c*i+1,1)[0]==2){slot=i;break;}
 uint32_t bank=at(m,ch+0x7c*slot+6,1)[0],clip=half(m,ch+0x7c*slot+2),target=current;
 q->weight=1.0f;q->mode=mode;memset(q->a,0,sizeof q->a);
 if(index){bank=banks[rnd()%nbanks];clip=choose_clip(m,bank);target=index%13==0?0xffff:key();q->weight=index%7==0?0.0f:rndf(0.1f,1.25f);
  const unsigned n=half(m,set+4);slot=rnd()%n;
  for(unsigned j=0;j<4;++j){const uint32_t a=mgr+100*j;uint32_t k=rnd()%5==0?0xffff:rnd()%5==0?0xfffeu-j:key();
   p16(m,a+4,k);p16(m,a+6,rnd()%4);p16(m,a+8,rnd()&3u);pf(m,a+0xc,index%6==0?rndf(-1,3):0.0f);pf(m,a+0x50,rndf(0,1));pf(m,a+0x54,rndf(0,1));}
  /* Channel priorities are the bit-15 keys: only those may be stolen. */
  for(unsigned j=0;j<n;++j){const uint32_t a=ch+0x7c*j;at(m,a+1,1)[0]=(uint8_t)(rnd()%3);p16(m,a+8,rnd()%3?key():0xfffeu-(rnd()%4));
   pf(m,a+0x30,rnd()%3==0?0.0f:rndf(0,1));pf(m,a+0x34,rndf(0,1));p32(m,a+0x38,rnd()%4);pf(m,a+0x48,rndf(0,8));}
  const uint32_t a=ch+0x7c*slot;
  if(mode==0){at(m,a+1,1)[0]=(uint8_t)(index%5==0?index%3:2);pf(m,a+0x50,rndf(0.25f,3));pf(m,a+0x54,rndf(0,80));p16(m,a+0x66,index%2?0xffff:1+rnd()%40);}
  if(mode==3&&index%11==0){for(unsigned j=0;j<n;++j){at(m,ch+0x7c*j+1,1)[0]=2;p16(m,ch+0x7c*j+8,index%22==0?key():0xfffe);pf(m,ch+0x7c*j+0x30,0.25f);}++prepared[0];}
  if(index%3==0&&(mode==0||mode==3)){const uint32_t entry=word(m,header(m,bank)+12)+4+4*clip;synthetic(m,SCRATCH+0x400);p32(m,entry,SCRATCH+0x400);++prepared[1];}
  /* Isolated direct 78780 coverage: exact and progressively wildcard rules,
     types 0/1/2/3 plus fallback. All bank/clip/group ids remain from catalogs. */
  if(mode==2){const uint32_t from=ch,to=ch+0x7c,old=key();target=key();if(target==old){for(unsigned k=0;k<nkeys;++k)if(keys[k]!=old){target=keys[k];break;}}
   p16(m,mgr+4,old);p16(m,mgr+6,2);p16(m,mgr+104,0xffff);p16(m,mgr+106,1);p16(m,mgr+206,3);p16(m,mgr+304,old);p16(m,mgr+306,0);
   at(m,from+1,1)[0]=2;p16(m,from+8,old);at(m,to+1,1)[0]=2;p16(m,to+8,target);pf(m,from+0x30,2);pf(m,to+0x30,2);
   p16(m,from+2,clip);p16(m,from+4,half(m,header(m,bank)+2));pf(m,from+0x50,rndf(0.3f,3));p16(m,from+0x64,rnd()%100);
   p16(m,to+2,choose_clip(m,bank));p16(m,to+4,half(m,header(m,bank)+2));pf(m,to+0x50,rndf(0.3f,3));
   p16(m,CATALOG,1);p16(m,CATALOG+2,8);p16(m,CATALOG+4,old);p16(m,CATALOG+8,bank);p16(m,CATALOG+10,half(m,header(m,bank)+2));p32(m,CATALOG+12,SCRATCH);
   p32(m,SCRATCH,2);const unsigned wildcard=index%4;const uint32_t type=index%5;
   for(unsigned j=0;j<2;++j){const uint32_t r=SCRATCH+16*j;p16(m,r+4,wildcard&1?0xffff:half(m,to+4)|0x8000u);
    p16(m,r+6,wildcard&1?target:half(m,to+2));p16(m,r+8,wildcard&2?0xffff:clip);p16(m,r+10,0);p16(m,r+12,0xffff);p32(m,r+16,SCRATCH+0x100+0x20*j);}
   for(unsigned j=0;j<2;++j){const uint32_t r=SCRATCH+0x100+0x20*j;p16(m,r+4,j?type:4);p16(m,r+6,1+rnd()%31);pf(m,r+8,rndf(0,1));p16(m,r+0xc,half(m,header(m,bank)+2));p16(m,r+0xe,clip);}
   if(type==3){const uint32_t entry=word(m,header(m,bank)+12)+4+4*clip;synthetic(m,SCRATCH+0x400);p32(m,entry,SCRATCH+0x400);}
   ++prepared[2+type];q->a[4]=old;
  }
 }
 if(mode==0){q->pc=0x8017413cu;q->a[0]=set;q->a[1]=ch;q->a[2]=slot;q->a[3]=bank;q->a[4]=clip;q->a[5]=target;q->a[6]=g;}
 else if(mode==1){q->pc=0x80177ef4u;q->a[0]=mgr;q->a[1]=set;q->a[2]=ch;q->a[3]=target;q->a[4]=g;}
 else if(mode==2){q->pc=0x80178780u;q->a[0]=mgr;q->a[1]=set;q->a[2]=ch;q->a[3]=target;if(!index)q->a[4]=current;q->a[5]=2;q->a[6]=g;}
 else{q->pc=0x80173e78u;q->a[0]=set;q->a[1]=ch;q->a[2]=bank;q->a[3]=clip;q->a[4]=target;q->a[5]=g;q->a[6]=index%3?0xffff:target;}
}
static int native_request(uint8_t *m,const request *q,int32_t *ret){nflstreet_clip_requests c;memset(&c,0,sizeof c);c.span=span;c.memory=m;c.event=native_event;c.group=native_group;
 const uint32_t *a=q->a;*ret=0;
 switch(q->mode){case 0:return nflstreet_clip_request_replace(&c,a[0],a[1],a[2],a[3],a[4],a[5],a[6]);
 case 1:return nflstreet_clip_request_group(&c,q->weight,a[0],a[1],a[2],a[3],a[4]);
 case 2:return nflstreet_clip_request_transition(&c,a[0],a[1],a[2],a[3],a[4],a[5],a[6]);
 default:return nflstreet_clip_request_start(&c,q->weight,a[0],a[1],a[2],a[3],a[4],a[5],a[6],ret);}}
int main(int argc,char **argv){if(argc!=3)return 2;HMODULE dll=LoadLibraryA(argv[2]);if(!dll)return 3;
 ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(dll,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get||!(module=get()))return 3;
 FILE *f=fopen(argv[1],"rb");uint8_t *saved=malloc(RAM_BYTES),*prepared_ram=malloc(RAM_BYTES),*native=malloc(RAM_BYTES);oracle=malloc(RAM_BYTES);
 if(!f||!saved||!prepared_ram||!native||!oracle)return 3;if(fread(saved,1,RAM_BYTES,f)!=RAM_BYTES||fgetc(f)!=EOF)return 3;fclose(f);
 const uint32_t table=word(saved,R13-0x7c30u),base=word(saved,table),actors=half(saved,table+10);if(actors!=14||!at(saved,base,actors*0x14d4u))return 4;
 if(!catalog(saved,half(saved,base+0x4da)))return 4;unsigned runs[4]={0},rejected=0;uint32_t digest=2166136261u;
 for(unsigned mode=0;mode<4;++mode)for(unsigned index=0;index<CASES;++index)for(unsigned actor=0;actor<actors;++actor){
  const uint32_t g=base+0x14d4u*actor;memcpy(prepared_ram,saved,RAM_BYTES);request q;prepare(prepared_ram,g,index,mode,&q);
  memcpy(native,prepared_ram,RAM_BYTES);memcpy(oracle,prepared_ram,RAM_BYTES);nn=on=0;memset(native_log,0,sizeof native_log);memset(original_log,0,sizeof original_log);
  int32_t nr,orr;if(!native_request(native,&q,&nr)){fprintf(stderr,"native rejected mode=%u case=%u actor=%u pc=%08x\n",mode,index,actor,q.pc);return 5;}
  if(!run_original(&q,&orr)){fprintf(stderr,"oracle failed mode=%u case=%u actor=%u\n",mode,index,actor);return 6;}
  if(!same(native)||((mode==1||mode==3)&&nr!=orr)){fprintf(stderr,"mismatch mode=%u case=%u actor=%u returns=%d/%d\n",mode,index,actor,orr,nr);return 7;}
  ++runs[mode];if(mode==3&&nr==-1)++rejected;digest=hash(digest,native+(g-RAM_BASE),0x14d4);digest=hash(digest,native_log,nn*sizeof(call));
 }
 if(!event_calls||!scan_calls||!group_calls||!operands||!rejected)return 8;for(unsigned i=2;i<=6;++i)if(!prepared[i])return 8;
 printf("Original clip requests: actors=%u cases=%u replace=%u group=%u transition=%u start=%u; all RAM outside oracle stack and callbacks match; rejected=%u events=%u scans=%u groups=%u operands=%u; digest=%08x.\n",actors,CASES,runs[0],runs[1],runs[2],runs[3],rejected,event_calls,scan_calls,group_calls,operands,digest);
 printf("Coverage: types=%u/%u/%u/%u fallback=%u synthetic=%u; dispatch",prepared[2],prepared[3],prepared[4],prepared[5],prepared[6],prepared[1]);for(unsigned i=0;i<8;++i)printf(" %u",branch_hits[i]);printf(".\n");return 0;}
