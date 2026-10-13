#define main source_actor_fixture_main
#include "actor_model_init_original_check.c"
#undef main
#include "attachment_palette_scale.h"
#include <math.h>
#define VIEW 0x80400000u
#define CLST 0x80400100u
#define ROWS 0x80400200u
#define OUTPUT 0x81000000u
static uint8_t locked[256*48];
static void *external(CPUState *c,uint32_t address,uint32_t size){(void)c;
 if(address>=0xe0000000u&&(uint64_t)address-0xe0000000u+size<=sizeof locked)return locked+address-0xe0000000u;return NULL;}
static uint64_t read_external(CPUState*c,uint32_t address,uint8_t size){uint8_t*p=external(c,address,size);uint64_t v=0;if(!p)return 0;for(unsigned i=0;i<size;++i)v=v<<8|p[i];return v;}
static void write_external(CPUState*c,uint32_t address,uint64_t v,uint8_t size){uint8_t*p=external(c,address,size);if(p)for(unsigned i=size;i;--i){p[i-1]=(uint8_t)v;v>>=8;}}
static void pf(uint8_t*p,float f){uint32_t v;memcpy(&v,&f,4);put(p,v);}
static int oracle(const ModernGekkoModuleDesc*m,uint8_t*ram,unsigned slot,unsigned lod,unsigned count,float scale,
 const uint8_t *rows,const float a[256][12],const float b[256][12]){
 static const uint32_t entries[]={0x80162864u,0x8016294cu,0x80162a7cu,0x80162c28u};
 static const unsigned offsets[]={0x5a8,0x788,0x968,0x698},destinations[]={0x1014,0x102c,0x1038,0x1020};
 memset(ptr(ram,ACTOR,NFLSTREET_ACTOR_MODEL_STORAGE),0,NFLSTREET_ACTOR_MODEL_STORAGE);put(ptr(ram,ACTOR+offsets[slot],4),VIEW);put(ptr(ram,VIEW+12,4),CLST);put(ptr(ram,CLST,4),ROWS);put(ptr(ram,CLST+4,4),count);
 memcpy(ptr(ram,ROWS,count*4),rows,count*4);put(ptr(ram,ACTOR+destinations[slot],4),OUTPUT);put(ptr(ram,ACTOR+0x1030,4),OUTPUT);pf(ptr(ram,ACTOR+0x1358,4),scale);
 for(unsigned i=0;i<count;++i)for(unsigned w=0;w<12;++w){pf(locked+i*48+w*4,a[i][w]);pf(ptr(ram,OUTPUT+i*48+w*4,4),b[i][w]);}
 CPUState c={0};c.ram=ram;c.ram_size=RAM_SIZE;c.msr=0x2000;c.hid2=0xe0000000u;c.gpr[1]=0x81700000u;c.gpr[30]=ACTOR;c.gpr[18]=lod;c.gpr[26]=0xe0000000u;
 c.external_pointer=external;c.external_read=read_external;c.external_write=write_external;c.pc=entries[slot];
 for(unsigned step=0;step<20000;++step){if(c.pc==0x801eacf4u){c.pc=c.lr;continue;}if(slot==0&&lod>=2&&c.pc==0x8018949cu)return 1;if(c.pc==0x801f0dd4u){if(c.gpr[3]!=OUTPUT||c.gpr[4]!=0xe0000000u||c.gpr[5]!=count*48u)return 0;
   memcpy(ptr(ram,OUTPUT,count*48u),locked,count*48u);return 1;}
  c.downcount=-1000000000;m->dispatch(&c,c.pc);if(c.exception){fprintf(stderr,"Scale exception pc=%08x ex=%08x\n",c.pc,c.exception);return 0;}}
 return 0;
}
int main(int argc,char**argv){if(argc!=3)return 2;HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;
 ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;
 const ModernGekkoModuleDesc*m=get();uint8_t*ram=load(argv[2]);if(!m||!ram)return 3;
 unsigned cases=0,matrices=0;const float scales[]={0.0f,0.51234567f,1.0f,1.1234567f,2.0f,-0.75f};const unsigned counts[]={1,2,7,23,255};
 for(unsigned slot=0;slot<4;++slot)for(unsigned lod=0;lod<4;++lod)for(unsigned n=0;n<5;++n)for(unsigned s=0;s<6;++s){unsigned count=counts[n];
  uint8_t rows[256*4]={0};float a[256][12],b[256][12];for(unsigned i=0;i<count;++i){rows[i*4]=i%3==0?0:i%3==1?13:7;
   for(unsigned w=0;w<12;++w){a[i][w]=((int)(i*12+w)-127)*0.031234567f;b[i][w]=a[i][w]+0.062345678f;}}
  if(!oracle(m,ram,slot,lod,count,scales[s],rows,a,b)||!nflstreet_attachment_palette_scale(rows,sizeof rows,count,(nflstreet_attachment_scale_slot)slot,lod,scales[s],a,b))return 4;
  for(unsigned i=0;i<count;++i)for(unsigned w=0;w<12;++w){uint32_t bits;memcpy(&bits,a[i]+w,4);if(bits!=be(locked+i*48+w*4)){fprintf(stderr,"scratch slot%u lod%u count%u scale%u matrix%u word%u native%08x original%08x\n",slot,lod,count,s,i,w,bits,be(locked+i*48+w*4));return 5;}
   memcpy(&bits,b[i]+w,4);if(bits!=be(ptr(ram,OUTPUT+i*48+w*4,4))){fprintf(stderr,"destination slot%u lod%u count%u scale%u matrix%u word%u native%08x original%08x\n",slot,lod,count,s,i,w,bits,be(ptr(ram,OUTPUT+i*48+w*4,4)));return 5;}}
  ++cases;matrices+=count;
 }
 float a[256][12]={{0}},b[256][12]={{0}},old[256][12];uint8_t rows[4]={0};memset(b,0xa5,sizeof b);memcpy(old,b,sizeof b);
 if(nflstreet_attachment_palette_scale(rows,4,1,NFLSTREET_SCALE_SLOT_698,0,NAN,a,b)||memcmp(b,old,sizeof b))return 6;
 if(nflstreet_attachment_palette_scale(rows,3,1,NFLSTREET_SCALE_SLOT_698,0,1,a,b)||memcmp(b,old,sizeof b))return 6;
 free(ram);FreeLibrary(h);printf("Attachment scale: %u original caller-block runs, %u matrices exact in scratch and destination; 4 model slots x4 LOD inputs x5 cluster counts x6 scales; 2 atomic guards; supplied matrices and copy service, no full caller.\n",cases,matrices);return 0;
}
