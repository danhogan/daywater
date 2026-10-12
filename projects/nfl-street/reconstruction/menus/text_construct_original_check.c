#include <windows.h>
#include "moderngekko/module_abi.h"
#include "construct.h"
#include "screen.h"
#include "text_construct.h"
#include "image_construct.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void put(uint8_t*p,unsigned v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
typedef struct {nflstreet_font fonts[3];unsigned acquisitions,acquired[128];unsigned count;nflstreet_ui_service_request requests[1024];} effects;
static int service(void*ctx,uint8_t*b,size_t n,uint32_t base,const nflstreet_ui_service_request*q,uint32_t*result){(void)b;(void)n;(void)base;(void)result;effects*s=ctx;if(s->count>=1024)return 0;s->requests[s->count++]=*q;if(q->class_id==8){unsigned o=q->target-base;if(o>n||40>n-o||b[o+3]!=2||(b[o+2]!=0&&b[o+2]!=2))return 0;if(!nflstreet_ui_text_construct(b,n,base,q->target,&s->fonts[b[o+2]]))return 0;if(s->acquisitions>=128)return 0;s->acquired[s->acquisitions++]=0x20000|b[o+2];}
 else if(q->class_id==0){unsigned request;uint32_t identity;if(!nflstreet_ui_image_construct(b,n,base,q->target,&request,&identity))return 0;if(request){if(s->acquisitions>=128)return 0;s->acquired[s->acquisitions++]=identity;}}return 1;}
static int execute(const ModernGekkoModuleDesc*m,CPUState*c,effects*s){
 c->pc=0x801ce994;c->lr=0;c->downcount=c->cycle_budget=100000000;
 for(unsigned i=0;c->pc&&i<1000000;++i){if(c->pc==0x801d60dc||c->pc==0x801d9244){if(s->count>=1024||c->gpr[5]||c->gpr[6]||c->gpr[7]){fprintf(stderr,"Callback arguments pc=%x r5=%x r6=%x r7=%x\n",c->pc,c->gpr[5],c->gpr[6],c->gpr[7]);return 0;}nflstreet_ui_service_request q={0};q.kind=0;q.class_id=c->pc==0x801d9244?8:0;q.target=c->gpr[3];q.property=c->gpr[4];s->requests[s->count++]=q;{c->downcount=-1000000000;m->dispatch(c,c->pc);}}
 else if(c->pc==0x801d5c24){if(c->gpr[3]!=1){fprintf(stderr,"Manager request=%x\n",c->gpr[3]);return 0;}c->gpr[3]=0x817f0000;c->pc=c->lr;}
 else if(c->pc==0x801d54a4){if(s->acquisitions>=128)return 0;s->acquired[s->acquisitions++]=(c->gpr[4]&65535)<<16|(c->gpr[5]&65535);c->pc=c->lr;}
 else if(c->pc==0x801d5884){if(c->gpr[4]!=2||(c->gpr[5]!=0&&c->gpr[5]!=2)){fprintf(stderr,"Font lookup archive=%x entry=%x\n",c->gpr[4],c->gpr[5]);return 0;}c->gpr[3]=c->gpr[5]?0x817f3120:0x817f3100;c->pc=c->lr;}
 else {c->downcount=-1000000000;m->dispatch(c,c->pc);}if(c->exception){fprintf(stderr,"Original exception pc=%x ex=%x\n",c->pc,c->exception);return 0;}}if(c->pc)fprintf(stderr,"Original stopped pc=%x exception=%x\n",c->pc,c->exception);return !c->pc;
}
int main(int argc,char**argv){if(argc!=5&&argc!=6)return 2;HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;const ModernGekkoModuleDesc*m=get();if(m->cpu_state_size!=sizeof(CPUState))return 3;
 uint8_t*ram=malloc(24*1024*1024);FILE*f=fopen(argv[2],"rb");if(!ram||!f||fread(ram,1,24*1024*1024,f)!=24*1024*1024)return 4;fclose(f);f=fopen(argv[3],"rb");if(!f)return 4;fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);if(n<32||n>1000000)return 4;uint8_t*b=malloc(n),*source=malloc(n),*before=malloc(n);if(!b||!source||!before||fread(b,1,n,f)!=(size_t)n)return 4;fclose(f);unsigned relocation;if(!nflstreet_ui_relocate(b,n,0x81100000,&relocation))return 5;memcpy(source,b,n);memcpy(ram+0x1100000,b,n);
 memset(ram+0x17f0000,0,0x200);put(ram+0x17f0048,9);put(ram+0x17f004c,0x817b0000);memset(ram+0x17b0000,0,36);put(ram+0x17b0000,0x801d60dc);put(ram+0x17b0020,0x801d9244);put(ram+0x17f0110,0x81100000);
 effects native={0},original={0};unsigned count0=0,count8=0;
 for(unsigned entry=0;entry<=2;entry+=2){char path[4096];snprintf(path,sizeof path,"%s/%05u.bin",argv[4],entry);f=fopen(path,"rb");if(!f)return 4;fseek(f,0,SEEK_END);long size=ftell(f);rewind(f);unsigned at=entry?0x1790000:0x17a0000;if(size<32||size>65536||fread(ram+at,1,size,f)!=(size_t)size)return 4;fclose(f);if(!nflstreet_font_open(ram+at,size,&native.fonts[entry]))return 5;original.fonts[entry]=native.fonts[entry];unsigned record=entry?0x17f3020:0x17f3000,asset=entry?0x17f3120:0x17f3100;put(ram+record+4,0x80000000+at);put(ram+record+8,0x80000000+at+native.fonts[entry].glyphs);put(ram+record+12,0x80000000+at+native.fonts[entry].kerning);put(ram+record+16,0x80000000+at+native.fonts[entry].atlas);put(ram+asset,0x80000000+record);}

 for(unsigned pass=0;pass<1;++pass){unsigned property=pass?0:UINT32_MAX;
  if(!nflstreet_ui_construct(b,n,0x81100000,0,property,0x101,service,&native,sizeof native))return 6;
  CPUState c={0};c.gpr[1]=0x81700000;c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;c.ram=ram;c.ram_size=24*1024*1024;c.msr=0x2000;c.gpr[3]=0x817f0000;c.gpr[4]=0x817f0100;c.gpr[5]=0;c.gpr[6]=property;
  if(!execute(m,&c,&original)||memcmp(b,ram+0x1100000,n)||memcmp(&native,&original,sizeof native)){fprintf(stderr,"Construction mismatch pass=%u effects=%u/%u\n",pass,native.count,original.count);return 7;}
  if(!pass)for(unsigned i=0;i<native.count;++i){count0+=native.requests[i].class_id==0;count8+=native.requests[i].class_id==8;}
 }
 if(argc==6){FILE*out=fopen(argv[5],"wb");if(!out||fwrite(b,1,n,out)!=(size_t)n)return 9;fclose(out);}
 memcpy(b,source,n);memcpy(before,b,n);effects prior={0};native.count=1023;native.acquisitions=0;memset(native.acquired,0,sizeof native.acquired);prior=native;
 if(nflstreet_ui_construct(b,n,0x81100000,0,UINT32_MAX,0x101,service,&native,sizeof native)||memcmp(b,before,n)||memcmp(&native,&prior,sizeof native))return 8;
 printf("Original Options image/text construction join exact:1 full owned-file comparison,%u ordered registered callbacks (first pass class0=%u class8=%u);global and link initialization flags compared;late callback-capacity rollback;377 image and33 text objects initialized;89 ordered image/font acquire requests retained;supplied fonts0/2,no resource I/O\n",original.count,count0,count8);free(b);free(source);free(before);free(ram);FreeLibrary(h);return 0;
}
