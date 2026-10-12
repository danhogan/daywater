/* Controlled original text-class stage checks. Resource lookup returns a
   supplied font record; GPU setup/drawing are observed boundaries. */
#include <windows.h>
#include "moderngekko/module_abi.h"
#include "text.h"
#include "text_lines.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned be(const uint8_t*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static void shortput(uint8_t*p,unsigned v){p[0]=v>>8;p[1]=v;}
static void put(uint8_t*p,unsigned v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static float fl(const uint8_t*p){unsigned v=be(p);float f;memcpy(&f,&v,4);return f;}
static void cpu(CPUState*c,uint8_t*ram){memset(c,0,sizeof *c);c->gpr[1]=0x81700000;c->gpr[2]=0x803b9a40;c->gpr[13]=0x803a9a40;c->ram=ram;c->ram_size=24*1024*1024;c->msr=0x2000;}
static int run(const ModernGekkoModuleDesc*m,CPUState*c,unsigned pc,nflstreet_ui_text_plan*draw,unsigned*draws){
 c->pc=pc;c->lr=0;c->downcount=c->cycle_budget=100000000;c->exception=0;*draws=0;
 for(unsigned i=0;c->pc&&i<1000000;++i){unsigned at=c->pc;
  if(at==0x801d5c24){if(c->gpr[3]!=1)return 0;c->gpr[3]=0x817f2000;c->pc=c->lr;}
  else if(at==0x801d5884){if(c->gpr[4]!=2||c->gpr[5]!=0)return 0;c->gpr[3]=0x817f0100;c->pc=c->lr;}
  else if(at==0x8018b7b8){if(c->gpr[3]>1020)return 0;c->gpr[3]=0x817d0000;c->pc=c->lr;}
  else if(at==0x80186cd8){c->pc=c->lr;}
  else if(at==0x80186b1c){
   if(!draw||c->fpr[1]!=0||c->gpr[3]!=0x817f0000||c->gpr[4]!=0x817e0000)return 0;
   draw->visible=1;draw->flags=c->gpr[5];unsigned p=c->gpr[6]-0x80000000;
   if(p>24*1024*1024-12)return 0;for(unsigned k=0;k<3;++k)draw->position[k]=fl(c->ram+p+k*4);
   for(unsigned pass=0;pass<2;++pass)for(unsigned k=0;k<4;++k)draw->color[pass][k]=fl(c->ram+0x39c484+pass*16+k*4);
   for(unsigned k=0;k<3;++k)draw->shadow_offset[k]=fl(c->ram+0x39c4a4+k*4);
   ++*draws;c->gpr[3]=0;c->pc=c->lr;
  }else {c->downcount=-1000000000;m->dispatch(c,c->pc);}if(c->exception)break;
 }if(c->pc||c->exception){fprintf(stderr,"Text stage pc=%08x ex=%08x\n",c->pc,c->exception);return 0;}return 1;
}
int main(int argc,char**argv){
 if(argc!=4)return 2;HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;const ModernGekkoModuleDesc*m=get();if(m->cpu_state_size!=sizeof(CPUState))return 3;
 uint8_t*ram=malloc(24*1024*1024);FILE*f=fopen(argv[2],"rb");if(!ram||!f||fread(ram,1,24*1024*1024,f)!=24*1024*1024)return 4;fclose(f);
 const char*texts[]={"Data has changed on WWWWWWWWWWWWW.\nAll unsaved data will be lost.","Controller Port 1, Expansion Slot 1-A","Cancel ","Continue Without Saving ","Save ","","A\nB\n","WWWiiii..."," a  b longwords","AAAAAAA"};
 const unsigned widths[]={1,16,60,172,255};unsigned cases=0;
 for(unsigned font=0;font<4;++font){char path[4096];snprintf(path,sizeof path,"%s/%05u.bin",argv[3],font);f=fopen(path,"rb");if(!f)return 4;fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);if(n<32||n>65536||fread(ram+0x17a0000,1,n,f)!=(size_t)n)return 4;fclose(f);nflstreet_font nf;if(!nflstreet_font_open(ram+0x17a0000,n,&nf))return 4;
 memset(ram+0x17f0000,0,32);put(ram+0x17f0004,0x817a0000);put(ram+0x17f0008,0x817a0000+nf.glyphs);put(ram+0x17f000c,0x817a0000+nf.kerning);put(ram+0x17f0100,0x817f0000);
 for(unsigned t=0;t<10;++t)for(unsigned mode=0;mode<2;++mode)for(unsigned width=0;width<5;++width)for(unsigned align=0;align<3;++align){uint8_t obj[40]={0};shortput(obj,0x8140|(mode?0x2000:0x1000)|align);obj[2]=0;obj[3]=2;shortput(obj+20,widths[width]);put(obj+32,0x817f1000);memcpy(ram+0x17b0000,obj,40);strcpy((char*)ram+0x17e0000,texts[t]);put(ram+0x17f1008,0x817e0000);memset(ram+0x17d0000,0,1020);
 nflstreet_ui_text_lines plan={0};if(!nflstreet_ui_text_prepare_lines(obj,&nf,texts[t],0x817d0000,&plan))return 5;CPUState c;cpu(&c,ram);c.gpr[3]=0x817b0000;unsigned draws;if(!run(m,&c,0x801d8e54,NULL,&draws))return 6;
 if(memcmp(obj,ram+0x17b0000,40)){fprintf(stderr,"Wrap object font%u text%u mode%u width%u align%u count%u/%u\n",font,t,mode,widths[width],align,plan.count,ram[0x17b000f]);return 7;}
 if(plan.count>1)for(unsigned j=0;j<plan.count;++j)if(((unsigned)plan.lines[j].begin<<16|plan.lines[j].end)!=be(ram+0x17d0000+j*4)){fprintf(stderr,"Wrap range font%u text%u mode%u width%u line%u\n",font,t,mode,widths[width],j);return 8;}++cases;
 }}
 printf("Original801D8E54 word/character wrapping exact:%u cases;4fonts,10strings,5widths,3alignments;40byte objects and all owned line ranges;supplier allocator boundary;no menu rendering claim\n",cases);FreeLibrary(h);free(ram);return 0;
}
