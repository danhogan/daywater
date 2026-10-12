/* Controlled original text-class stage checks. Resource lookup returns a
   supplied font record; GPU setup/drawing are observed boundaries. */
#include <windows.h>
#include "moderngekko/module_abi.h"
#include "text.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned be(const uint8_t*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static void shortput(uint8_t*p,unsigned v){p[0]=v>>8;p[1]=v;}
static void put(uint8_t*p,unsigned v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static float fl(const uint8_t*p){unsigned v=be(p);float f;memcpy(&f,&v,4);return f;}
static void pf(uint8_t*p,float f){unsigned v;memcpy(&v,&f,4);put(p,v);}
static void cpu(CPUState*c,uint8_t*ram){memset(c,0,sizeof *c);c->gpr[1]=0x81700000;c->gpr[2]=0x803b9a40;c->gpr[13]=0x803a9a40;c->ram=ram;c->ram_size=24*1024*1024;c->msr=0x2000;}
static int run(const ModernGekkoModuleDesc*m,CPUState*c,unsigned pc,nflstreet_ui_text_plan*draw,unsigned*draws){
 c->pc=pc;c->lr=0;c->downcount=c->cycle_budget=100000000;c->exception=0;*draws=0;
 for(unsigned i=0;c->pc&&i<1000000;++i){unsigned at=c->pc;
  if(at==0x801d5c24){if(c->gpr[3]!=1)return 0;c->gpr[3]=0x817f2000;c->pc=c->lr;}
  else if(at==0x801d5884){if(c->gpr[4]!=2||c->gpr[5]!=0)return 0;c->gpr[3]=0x817f0100;c->pc=c->lr;}
  else if(at==0x80186cd8){c->pc=c->lr;}
  else if(at==0x80186b1c){
   if(!draw||c->fpr[1]!=0||c->gpr[3]!=0x817f0000||c->gpr[4]!=0x817e0000)return 0;
   draw->visible=1;draw->flags=c->gpr[5];unsigned p=c->gpr[6]-0x80000000;
   if(p>24*1024*1024-12)return 0;for(unsigned k=0;k<3;++k)draw->position[k]=fl(c->ram+p+k*4);
   for(unsigned pass=0;pass<2;++pass)for(unsigned k=0;k<4;++k)draw->color[pass][k]=fl(c->ram+0x39c484+pass*16+k*4);
   for(unsigned k=0;k<3;++k)draw->shadow_offset[k]=fl(c->ram+0x39c4a4+k*4);
   ++*draws;c->gpr[3]=0;c->pc=c->lr;
  }else m->dispatch(c,c->pc);if(c->exception)break;
 }if(c->pc||c->exception){fprintf(stderr,"Text stage pc=%08x ex=%08x\n",c->pc,c->exception);return 0;}return 1;
}
int main(int argc,char**argv){
 if(argc!=5)return 2;HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;const ModernGekkoModuleDesc*m=get();if(m->cpu_state_size!=sizeof(CPUState))return 3;
 uint8_t*ram=malloc(24*1024*1024);FILE*f=fopen(argv[2],"rb");if(!ram||!f||fread(ram,1,24*1024*1024,f)!=24*1024*1024)return 4;fclose(f);ram[0x3a33f0]=1;
 uint8_t title[1279];f=fopen(argv[4],"rb");if(!f||fread(title,1,sizeof title,f)!=sizeof title)return 4;fclose(f);
 const char*texts[]={"Press START","Quick Game","AVATAR To WAVE","","WWWiiii..."};unsigned prepared=0,drawn=0;
 for(unsigned font=0;font<4;++font){char path[4096];snprintf(path,sizeof path,"%s/%05u.bin",argv[3],font);f=fopen(path,"rb");if(!f)return 4;fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);if(n<32||n>65536||fread(ram+0x17a0000,1,(size_t)n,f)!=(size_t)n)return 4;fclose(f);nflstreet_font nf;if(!nflstreet_font_open(ram+0x17a0000,(size_t)n,&nf))return 5;
  memset(ram+0x17f0000,0,32);put(ram+0x17f0004,0x817a0000);put(ram+0x17f0008,0x817a0000+(unsigned)nf.glyphs);put(ram+0x17f000c,0x817a0000+(unsigned)nf.kerning);put(ram+0x17f0010,0x817a0000+(unsigned)nf.atlas);put(ram+0x17f0100,0x817f0000);put(ram+0x17f0104,0);
  for(unsigned t=0;t<5;++t)for(unsigned flags=0;flags<8;++flags)for(unsigned context=0;context<4;++context){
   uint8_t native[40];memcpy(native,title+0x140,40);native[0]=0x10;native[1]=0x40|flags;native[15]=0;native[16]=0;
   if(context==1){shortput(native+6,(uint16_t)-33);shortput(native+8,(uint16_t)-17);shortput(native+10,5);native[12]=(uint8_t)-3;native[13]=4;}
   if(context==2){shortput(native+6,32000);shortput(native+8,123);shortput(native+10,(uint16_t)-20);native[14]=(uint8_t)-7;}
   put(native+32,0x817f1000);memcpy(ram+0x17b0000,native,40);strcpy((char*)ram+0x17e0000,texts[t]);put(ram+0x17f1008,0x817e0000);
   CPUState c;cpu(&c,ram);c.gpr[3]=0x817b0000;unsigned calls;if(!nflstreet_ui_text_prepare(native,&nf,texts[t])||!run(m,&c,0x801d8e54,NULL,&calls)||memcmp(native,ram+0x17b0000,40)){fprintf(stderr,"Prepare mismatch font=%u text=%u flags=%u\n",font,t,flags);return 6;}++prepared;
   float ctx[8];for(unsigned k=0;k<8;++k){ctx[k]=k<4?(context==0?1:context==1?0.5f:context==2?0.25f:1.25f):(context==0?0:context==1?-8:context==2?32:0);pf(ram+0x17f3000+k*4,ctx[k]);}put(ram+0x3a3678,0x817f3000); /* r13 -63C8 */
   nflstreet_ui_text_plan want,actual={0};if(!nflstreet_ui_text_draw(native,texts[t],ctx,&want))return 7;
   cpu(&c,ram);c.gpr[3]=0x817b0000;if(!run(m,&c,0x801d97d4,&actual,&calls)||calls!=want.visible||memcmp(actual.position,want.position,12)||memcmp(actual.color,want.color,32)||memcmp(actual.shadow_offset,want.shadow_offset,12)||actual.flags!=want.flags){fprintf(stderr,"Draw mismatch font=%u text=%u flags=%u ctx=%u calls=%u\n",font,t,flags,context,calls);return 8;}++drawn;
  }
 }
 /* Unsupported native branches reject atomically rather than silently approximating. */
 unsigned guards=0;uint8_t bad[40],before[40];memcpy(before,title+0x140,40);
 for(unsigned mode=0;mode<3;++mode){memcpy(bad,before,40);if(mode==0)bad[1]|=1; /* alignment is supported; wrap bit is high byte bit1 */
  if(mode==0)bad[0]|=1;else if(mode==1)bad[15]=2;
  uint8_t expected[40];memcpy(expected,bad,40);nflstreet_font fake={0};
  const char*input=mode==2?"A\nB":"Press START";
  if(nflstreet_ui_text_prepare(bad,&fake,input)||memcmp(bad,expected,40))return 9;++guards;
 }
 uint8_t valid[40];memcpy(valid,ram+0x17b0000,40);nflstreet_ui_text_plan untouched;memset(&untouched,0xa5,sizeof untouched);float unit[8]={1,1,1,1,0,0,0,0};
 for(unsigned mode=0;mode<4;++mode){memcpy(bad,valid,40);if(mode==0)bad[0]|=0x80;else if(mode==1)bad[0]|=1;else if(mode==2)bad[4]=0;else bad[15]=2;
  nflstreet_ui_text_plan out=untouched;if(nflstreet_ui_text_draw(bad,texts[4],unit,&out)||memcmp(&out,&untouched,sizeof out))return 9;++guards;
 }
 printf("Original single-line text exact: prepared=%u complete 40-byte objects; draw plans=%u; atomic guards=%u; resource/font records supplied, GX setup/draw boundaries observed; no projection/navigation claim\n",prepared,drawn,guards);free(ram);FreeLibrary(h);return 0;
}
