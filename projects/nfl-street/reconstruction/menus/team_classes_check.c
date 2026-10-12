#include <windows.h>
#include "moderngekko/module_abi.h"
#include "team_classes.h"
#include "screen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM_SIZE (24u*1024u*1024u)
#define BASE 0x81100000u
#define FILE_AT 0x1100000u
#define FONT_AT 0x1740000u
#define FONT_STRIDE 0x10000u
#define REGISTRY 0x17b0000u
#define MANAGER 0x817f0000u
#define FONT_RESOURCE 0x817f3100u

typedef struct {
 nflstreet_ui_service_request requests[NFLSTREET_TEAM_CLASSES_REQUEST_CAPACITY];
 uint32_t acquisitions[NFLSTREET_TEAM_CLASSES_ACQUIRE_CAPACITY];
 unsigned request_count, acquisition_count, callbacks, line_allocations;
 uint32_t line_allocation_sizes[NFLSTREET_TEAM_CLASSES_LINE_PLAN_CAPACITY];
} original_log;

static uint32_t be(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void put(uint8_t*p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}

static uint8_t*load_file(const char*path,size_t*size,size_t max){
 FILE*f=fopen(path,"rb");if(!f)return NULL;
 if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}long length=ftell(f);
 if(length<1||(size_t)length>max||fseek(f,0,SEEK_SET)){fclose(f);return NULL;}
 uint8_t*b=malloc((size_t)length);if(!b){fclose(f);return NULL;}
 if(fread(b,1,(size_t)length,f)!=(size_t)length||ferror(f)){free(b);fclose(f);return NULL;}
 fclose(f);*size=(size_t)length;return b;
}

static int load_fonts(const char*folder,nflstreet_font fonts[4],uint8_t*ram,
                      size_t font_sizes[4],uint8_t*font_bytes[4]){
 for(unsigned i=0;i<4;++i){char path[4096];
  int z=snprintf(path,sizeof path,"%s/uis_font/%05u.bin",folder,i);
  if(z<0||(size_t)z>=sizeof path)return 0;
  font_bytes[i]=load_file(path,&font_sizes[i],FONT_STRIDE);
  if(!font_bytes[i])return 0;
  unsigned at=FONT_AT+i*FONT_STRIDE;
  memcpy(ram+at,font_bytes[i],font_sizes[i]);
  if(!nflstreet_font_open(ram+at,font_sizes[i],&fonts[i]))return 0;
  unsigned record=0x17f3000u+i*0x20u,asset=0x17f3100u+i*0x20u;
  put(ram+record+4,0x80000000u+at);
  put(ram+record+8,0x80000000u+at+fonts[i].glyphs);
  put(ram+record+12,0x80000000u+at+fonts[i].kerning);
  put(ram+record+16,0x80000000u+at+fonts[i].atlas);
  put(ram+asset,0x80000000u+record);
 }
 return 1;
}

static int native_probe(const char*source_path,const char*folder){
 size_t n=0;uint8_t*source=load_file(source_path,&n,8u*1024u*1024u);
 uint8_t*ram=calloc(1,RAM_SIZE);nflstreet_team_classes*s=calloc(1,sizeof *s);
 if(!source||!ram||!s||n!=253987u)return 4;
 size_t sizes[4]={0};uint8_t*font_bytes[4]={0};
 if(!load_fonts(folder,s->fonts,ram,sizes,font_bytes))return 5;
 unsigned relocated=0;if(!nflstreet_ui_relocate(source,n,BASE,&relocated))return 6;
 if(!nflstreet_team_classes_construct(source,n,BASE,s)){fprintf(stderr,"Native construct failed requests%u class0%u class8%u lines%u acquisitions%u\n",s->request_count,s->class_requests[0],s->class_requests[8],s->line_plan_count,s->acquisition_count);return 7;}
 printf("Independent native team image/text construction passes: file=%u nodes=%u requests=%u class0=%u class8=%u resourceAcquisitionAndLookupIdentities=%u fontLookups=%u constructionMask=0x101; class3 actor preview remains outside.\n",
        (unsigned)n,be(source),s->request_count,s->class_requests[0],s->class_requests[8],
        s->acquisition_count,s->text_requests);
 for(unsigned i=0;i<4;++i)free(font_bytes[i]);free(s);free(ram);free(source);return 0;
}

static int original_construct(const ModernGekkoModuleDesc*m,CPUState*c,
                              original_log*log){
 c->pc=0x801ce994;c->lr=0;c->downcount=c->cycle_budget=100000000;
 for(unsigned step=0;c->pc&&step<1000000;++step){uint32_t pc=c->pc;
  if(pc==0x801d60dc||pc==0x801d9244){
   if(log->request_count>=NFLSTREET_TEAM_CLASSES_REQUEST_CAPACITY||
      c->gpr[5]||c->gpr[6]||c->gpr[7])return 0;
   nflstreet_ui_service_request*q=&log->requests[log->request_count++];memset(q,0,sizeof *q);
   q->kind=0;q->class_id=pc==0x801d9244?8u:0u;q->target=c->gpr[3];q->property=c->gpr[4];
   ++log->callbacks;c->downcount=-1000000000;m->dispatch(c,pc);
  }else if(pc==0x801d5c24){
   if(c->gpr[3]!=1)return 0;c->gpr[3]=MANAGER;c->pc=c->lr;
  }else if(pc==0x801d5884){
   uint32_t entry=c->gpr[5];
   if(c->gpr[4]!=2||entry>=NFLSTREET_TEAM_CLASSES_FONT_COUNT||
      log->acquisition_count>=NFLSTREET_TEAM_CLASSES_ACQUIRE_CAPACITY)return 0;
   log->acquisitions[log->acquisition_count++]=0x20000u|entry;
   c->gpr[3]=FONT_RESOURCE+entry*0x20u;c->pc=c->lr;
  }else if(pc==0x801d54a4){
   if(log->acquisition_count>=NFLSTREET_TEAM_CLASSES_ACQUIRE_CAPACITY)return 0;
   log->acquisitions[log->acquisition_count++]=((c->gpr[4]&0xffffu)<<16)|(c->gpr[5]&0xffffu);
   c->pc=c->lr;
  }else if(pc==0x8018b7b8){
   if(log->line_allocations>=NFLSTREET_TEAM_CLASSES_LINE_PLAN_CAPACITY||
      c->gpr[3]>1020||c->gpr[4])return 0;
   log->line_allocation_sizes[log->line_allocations]=c->gpr[3];
   c->gpr[3]=0x81610000u+log->line_allocations*1024u;
   ++log->line_allocations;c->pc=c->lr;
  }else{c->downcount=-1000000000;m->dispatch(c,pc);}
  if(c->exception){fprintf(stderr,"Original constructor exception pc=%08x ex=%08x\n",c->pc,c->exception);return 0;}
 }
 if(c->pc){fprintf(stderr,"Original constructor stopped pc=%08x lr=%08x\n",c->pc,c->lr);return 0;}
 return 1;
}

static uint8_t*ram_pointer(uint8_t*ram,uint32_t address){
 if(address<0x80000000u)return NULL;
 uint32_t offset=address&0x1ffffffu;
 return offset<RAM_SIZE?ram+offset:NULL;
}

static void normalize_line_pointer(uint8_t*file,size_t n,uint32_t base,
                                   uint32_t target,uint32_t canonical){
 if(target<base||target-base>n||40>n-(target-base))return;
 uint8_t*object=file+target-base;
 object[36]=(uint8_t)(canonical>>24);object[37]=(uint8_t)(canonical>>16);
 object[38]=(uint8_t)(canonical>>8);object[39]=(uint8_t)canonical;
}

static int original_check(const char*dll,const char*snapshot,const char*source_path,
                         const char*folder){
 HMODULE h=LoadLibraryA(dll);if(!h)return 3;
 ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);
 if(!get){FreeLibrary(h);return 3;}const ModernGekkoModuleDesc*m=get();
 if(!m||m->cpu_state_size!=sizeof(CPUState)){FreeLibrary(h);return 3;}
 uint8_t*ram=malloc(RAM_SIZE);size_t snapshot_size=0,source_size=0;
 uint8_t*snap=load_file(snapshot,&snapshot_size,RAM_SIZE);
 uint8_t*source=load_file(source_path,&source_size,8u*1024u*1024u);
 uint8_t*native_file=NULL;original_log*log=calloc(1,sizeof *log);
 nflstreet_team_classes*native=calloc(1,sizeof *native);
 if(!ram||!snap||snapshot_size!=RAM_SIZE||!source||source_size!=253987u||!log||!native){FreeLibrary(h);return 4;}
 memcpy(ram,snap,RAM_SIZE);free(snap);
 size_t sizes[4]={0};uint8_t*font_bytes[4]={0};
 if(!load_fonts(folder,native->fonts,ram,sizes,font_bytes)){FreeLibrary(h);return 5;}
 unsigned relocation=0;if(!nflstreet_ui_relocate(source,source_size,BASE,&relocation)){FreeLibrary(h);return 6;}
 native_file=malloc(source_size);if(!native_file){FreeLibrary(h);return 4;}memcpy(native_file,source,source_size);
 memcpy(ram+FILE_AT,source,source_size);
 memset(ram+0x17f0000,0,0x3000);
 put(ram+0x17f0048,9);put(ram+0x17f004c,0x817b0000);put(ram+0x17f0104,7);put(ram+0x17f0108,5);put(ram+0x17f0110,BASE);
 memset(ram+REGISTRY,0,36);put(ram+REGISTRY,0x801d60dc);put(ram+REGISTRY+0x20,0x801d9244);
 if(!nflstreet_team_classes_construct(native_file,source_size,BASE,native)){fprintf(stderr,"Native entry-7 construction failed\n");return 7;}
 CPUState c={0};c.ram=ram;c.ram_size=RAM_SIZE;c.msr=0x2000;c.gpr[1]=0x81700000;c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;
 c.gpr[3]=MANAGER;c.gpr[4]=0x817f0100;c.gpr[5]=0;c.gpr[6]=UINT32_MAX;
 if(!original_construct(m,&c,log)){fprintf(stderr,"Original entry-7 class construction reached an unsupported boundary\n");return 8;}
 if(log->line_allocations!=native->line_plan_count){fprintf(stderr,"Independent line allocation count differs native=%u original=%u\n",native->line_plan_count,log->line_allocations);return 12;}
 for(unsigned i=0;i<native->line_plan_count;++i){
  const nflstreet_team_classes_line_plan*p=&native->line_plans[i];
  if(log->line_allocation_sizes[i]!=p->plan.count*4u){fprintf(stderr,"Line allocation size differs at %u\n",i);return 13;}
  size_t object_offset=p->target-BASE;uint32_t original_lines=be(ram+FILE_AT+object_offset+36);
  if(original_lines!=0x81610000u+i*1024u){fprintf(stderr,"Original line table pointer differs at %u: %08x\n",i,original_lines);return 14;}
  uint8_t*table=ram_pointer(ram,original_lines);if(!table||p->plan.count*4u>RAM_SIZE-(size_t)(table-ram))return 15;
  for(unsigned j=0;j<p->plan.count;++j){uint32_t expected=(uint32_t)p->plan.lines[j].begin<<16|p->plan.lines[j].end;
   if(be(table+j*4)!=expected){fprintf(stderr,"Line range differs plan%u line%u native=%08x original=%08x\n",i,j,expected,be(table+j*4));return 16;}
  }
  normalize_line_pointer(native_file,source_size,BASE,p->target,p->line_base);
  normalize_line_pointer(ram+FILE_AT,source_size,BASE,p->target,p->line_base);
 }
 if(native->request_count!=log->request_count||
    memcmp(native->requests,log->requests,native->request_count*sizeof native->requests[0])){
  fprintf(stderr,"Ordered construct trace differs native=%u original=%u\n",native->request_count,log->request_count);return 9;
 }
 if(native->acquisition_count!=log->acquisition_count||
    memcmp(native->acquisitions,log->acquisitions,native->acquisition_count*sizeof native->acquisitions[0])){
  fprintf(stderr,"Ordered acquisition identities differ native=%u original=%u\n",native->acquisition_count,log->acquisition_count);return 10;
 }
 if(memcmp(native_file,ram+FILE_AT,source_size)){
  for(size_t i=0;i<source_size;++i)if(native_file[i]!=ram[FILE_AT+i]){fprintf(stderr,"Owned source differs at %zx native=%02x original=%02x\n",i,native_file[i],ram[FILE_AT+i]);break;}return 11;
 }
 printf("Team image/text original construction exact: %u-byte complete source file, %u ordered class callbacks (class0=%u class8=%u), %u ordered image/font acquisition and font lookup identities; original 801CE994, 801D60DC/801D9244 and resource helpers executed. Construction mask0x101; entry-7 class3 actor previews remain outside.\n",
        (unsigned)source_size,log->request_count,native->class_requests[0],native->class_requests[8],log->acquisition_count);
 for(unsigned i=0;i<4;++i)free(font_bytes[i]);free(native);free(log);free(native_file);free(source);free(ram);FreeLibrary(h);return 0;
}

int main(int argc,char**argv){
 if(argc==3)return native_probe(argv[1],argv[2]);
 if(argc==5)return original_check(argv[1],argv[2],argv[3],argv[4]);
 fprintf(stderr,"Usage: team-classes-check.exe <source> <unpacked-folder> | <original-DLL> <snapshot> <source> <unpacked-folder>\n");
 return 2;
}
