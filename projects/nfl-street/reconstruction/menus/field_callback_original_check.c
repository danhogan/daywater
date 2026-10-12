#include <windows.h>
#include "moderngekko/module_abi.h"
#include "font.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM_SIZE (24u*1024u*1024u)
#define BASE 0x80000000u
#define OBJECT 0x817e1000u
#define ARGS 0x817e1100u
#define RESULT 0x817e1200u
#define R13 0x803a9a40u
#define FILE_AT 0x1100000u
#define FONT_AT 0x1740000u
#define FONT_RESOURCE 0x817f3100u
#define FONT_RECORD 0x817f3000u
#define MANAGER 0x817f0000u
static const nflstreet_font *g_active_font;
static unsigned g_glyph_calls;
static int32_t g_glyph_width_sum;

static uint8_t *ptr(uint8_t *ram,uint32_t a){uint32_t o=a&0x1ffffffu;return o<RAM_SIZE?ram+o:NULL;}
static void put32(uint8_t*p,uint32_t v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static unsigned be16(const uint8_t*p){return (unsigned)p[0]<<8|p[1];}
static void put16(uint8_t*p,unsigned v){p[0]=v>>8;p[1]=(uint8_t)v;}
static uint8_t *load_file(const char*p,size_t*n){FILE*f=fopen(p,"rb");if(!f)return NULL;if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}long z=ftell(f);if(z<1||fseek(f,0,SEEK_SET)){fclose(f);return NULL;}uint8_t*b=malloc((size_t)z);if(!b||fread(b,1,(size_t)z,f)!=(size_t)z){free(b);fclose(f);return NULL;}fclose(f);*n=(size_t)z;return b;}

static int check_field_header_width(const ModernGekkoModuleDesc*m,uint8_t*ram,
                                    const char*source_path,const char*folder){
    size_t source_size=0;uint8_t*source=load_file(source_path,&source_size);
    if(!source||source_size<32914u||source_size>0x100000u){free(source);return 0;}
    memcpy(ptr(ram,0x81100000u),source,source_size);free(source);
    size_t font_size=0;char font_path[4096];
    if(snprintf(font_path,sizeof font_path,"%s/uis_font/00000.bin",folder)<0)return 0;
    uint8_t*font=load_file(font_path,&font_size);if(!font||font_size>0x10000u){free(font);return 0;}
    memcpy(ptr(ram,0x81740000u),font,font_size);
    uint32_t glyphs=(uint32_t)font[20]<<24|(uint32_t)font[21]<<16|(uint32_t)font[22]<<8|font[23];
    uint32_t kern=(uint32_t)font[24]<<24|(uint32_t)font[25]<<16|(uint32_t)font[26]<<8|font[27];
    uint32_t atlas=(uint32_t)font[28]<<24|(uint32_t)font[29]<<16|(uint32_t)font[30]<<8|font[31];
    uint8_t*record=ptr(ram,FONT_RECORD);uint8_t*resource=ptr(ram,FONT_RESOURCE);
    put32(record+4,0x81740000u);put32(record+8,0x81740000u+glyphs);
    put32(record+12,0x81740000u+kern);put32(record+16,0x81740000u+atlas);
    put32(resource,FONT_RECORD);
    /* Actual group0 entry8 class8 header, font0, text "Heading". */
    nflstreet_font parsed;if(!nflstreet_font_open(font,font_size,&parsed)){free(font);return 0;}
    g_active_font=&parsed;g_glyph_calls=0;
    const uint32_t targets[]={0x81104cc0u,0x81104d60u};
    uint8_t*arg=ptr(ram,ARGS);if(!arg){free(font);return 0;}
    uint32_t words[]={0x41300000u,11u};
    for(unsigned t=0;t<sizeof targets/sizeof targets[0];++t){
      const uint32_t target=targets[t];uint8_t*o=ptr(ram,target);if(!o){free(font);return 0;}
      g_glyph_calls=0;uint32_t result_bits[2]={0,0};int32_t glyph_sums[2]={0,0};
      for(unsigned w=0;w<2;++w){
        g_glyph_width_sum=0;
        put32(arg,words[w]);put32(ptr(ram,RESULT),0xdeadbeefu);CPUState c={0};c.ram=ram;c.ram_size=RAM_SIZE;c.msr=0x2000;
        c.gpr[1]=0x81700000u;c.gpr[2]=0x803b9a40u;c.gpr[13]=R13;
        c.gpr[3]=target;c.gpr[4]=12;c.gpr[5]=0;c.gpr[6]=ARGS;c.gpr[7]=RESULT;
        c.pc=0x801d9244u;c.lr=0;c.downcount=c.cycle_budget=100000000;
        for(unsigned step=0;c.pc&&step<100000;++step){
            if(c.pc==0x801d5c24u){if(c.gpr[3]!=1){free(font);return 0;}c.gpr[3]=MANAGER;c.pc=c.lr;}
            else if(c.pc==0x801d5884u){if(c.gpr[4]!=2||c.gpr[5]!=0){fprintf(stderr,"Unexpected field header font lookup (%08x,%08x) obj=%02x/%02x\n",c.gpr[4],c.gpr[5],o[2],o[3]);free(font);return 0;}c.gpr[3]=FONT_RESOURCE;c.pc=c.lr;}
            else if(c.pc==0x80186710u){const uint8_t*g=nflstreet_font_glyph(g_active_font,c.gpr[4]&0xffu);int32_t width=g?(int8_t)g[8]:0;c.gpr[3]=(uint32_t)width;g_glyph_width_sum+=width;++g_glyph_calls;c.pc=c.lr;}
            else {m->dispatch(&c,c.pc);if(c.exception){fprintf(stderr,"property12 exception pc=%08x ex=%08x\n",c.pc,c.exception);free(font);return 0;}}
        }
        if(c.pc){fprintf(stderr,"property12 stopped pc=%08x\n",c.pc);free(font);return 0;}
        uint8_t*rp=ptr(ram,RESULT);result_bits[w]=(uint32_t)rp[0]<<24|(uint32_t)rp[1]<<16|(uint32_t)rp[2]<<8|rp[3];
        glyph_sums[w]=g_glyph_width_sum;
      }
      float actual_float,actual_word;memcpy(&actual_float,&result_bits[0],4);memcpy(&actual_word,&result_bits[1],4);
      if(actual_float!=(float)glyph_sums[0]){fprintf(stderr,"field header width mismatch target=%08x original=%g captured-glyph-sum=%d glyph-calls=%u\n",target,actual_float,glyph_sums[0],g_glyph_calls);free(font);return 0;}
      printf("Original field header property12: target=%08x font0; float-word 11.0 returns %g, raw word 11 returns %g; %u glyph-width returns supplied from parsed font asset; captured sum=%d, single-line range only.\n",target,actual_float,actual_word,g_glyph_calls,glyph_sums[0]);
    }
    free(font);return 1;
}

static int run_callback(const ModernGekkoModuleDesc*m,CPUState*c,uint8_t*ram,
                        uint32_t function,uint32_t prop,const uint32_t*args,unsigned argc){
    if(argc>8)return 0;
    uint8_t*o=ptr(ram,OBJECT),*a=ptr(ram,ARGS);if(!o||!a)return 0;
    memset(c,0,sizeof *c);c->ram=ram;c->ram_size=RAM_SIZE;c->msr=0x2000;
    c->gpr[1]=0x81700000u;c->gpr[2]=0x803b9a40u;c->gpr[13]=R13;
    c->gpr[3]=OBJECT;c->gpr[4]=prop;c->gpr[5]=0;c->gpr[6]=ARGS;c->gpr[7]=RESULT;
    for(unsigned i=0;i<argc;++i)put32(a+i*4,args[i]);
    c->pc=function;c->lr=0;c->downcount=c->cycle_budget=10000000;
    for(unsigned i=0;c->pc&&i<10000;++i){m->dispatch(c,c->pc);if(c->exception)return 0;}
    return c->pc==0&&!c->exception;
}

static int check_class0_prop0(const ModernGekkoModuleDesc*m,CPUState*c,uint8_t*ram){
    uint8_t*o=ptr(ram,OBJECT);uint32_t args[4]={0,0xc3080000u,0,0};
    memset(o,0x5a,76);put16(o,0x1234);memset(ptr(ram,ARGS),0,16);
    uint8_t*scale_word=ptr(ram,0x80263b08u);if(!scale_word)return 0;
    uint32_t scale_bits=(uint32_t)scale_word[0]<<24|(uint32_t)scale_word[1]<<16|
                        (uint32_t)scale_word[2]<<8|scale_word[3];
    float scale;memcpy(&scale,&scale_bits,4);if(!isfinite(scale)||scale==0)return 0;
    float x;memcpy(&x,&args[1],4);int32_t expected=(int32_t)(x*scale);
    uint8_t before[76];memcpy(before,o,sizeof before);
    if(!run_callback(m,c,ram,0x801d60dcu,0,args,4))return 0;
    uint8_t model[76];memcpy(model,before,sizeof model);
    put16(model+52,(uint16_t)expected);put16(model+54,0);put16(model+56,0);
    put16(model,(uint16_t)(0x1234u|0x4000u));
    if(memcmp(model,o,sizeof model)){
        for(unsigned i=0;i<76;++i)if(model[i]!=o[i]){fprintf(stderr,"class0 prop0 mismatch byte%u expected%02x original%02x scale=%g\n",i,model[i],o[i],scale);break;}return 0;}
    printf("Original 801D60DC property0 exact 76-byte case: args={0,C3080000,0,0}, DOL scale=%g, scaled shorts at +52/+54/+56, 0x4000 flag epilogue.\n",scale);
    return 1;
}

static int check_class8_prop5(const ModernGekkoModuleDesc*m,CPUState*c,uint8_t*ram){
    static const uint32_t words[]={0x00000000u,0x3f800000u,0xbf800000u,0xbf000000u};
    /* The original handler's signed-word test observes the PPC raw argument
       word; 0xbf000000 therefore returns after clearing the two status bits. */
    static const unsigned expected_low[]={1,2,0,0};
    uint8_t*o=ptr(ram,OBJECT);
    for(unsigned i=0;i<4;++i){uint32_t arg=words[i];memset(o,0x5a,40);put16(o,0x1237);
        if(!run_callback(m,c,ram,0x801d9244u,5,&arg,1))return 0;
        unsigned got=be16(o)&3u;if(got!=expected_low[i]){fprintf(stderr,"class8 prop5 raw%08x lowbits=%u expected%u\n",arg,got,expected_low[i]);return 0;}
        for(unsigned j=2;j<40;++j)if(o[j]!=0x5a)return 0;
    }
    puts("Original 801D9244 property5 checks zero, one, negative one, and negative half; raw-word status low bits are 1,2,0,0.");
    return 1;
}

int main(int argc,char**argv){
    if(argc!=5){fprintf(stderr,"Usage: field-callback-original-check.exe <original-DLL> <saved-RAM> <constructed-entry8-source> <unpacked-folder>\n");return 2;}
    HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;
    ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);
    if(!get){FreeLibrary(h);return 3;}const ModernGekkoModuleDesc*m=get();if(!m||m->cpu_state_size!=sizeof(CPUState)){FreeLibrary(h);return 3;}
    size_t n=0;uint8_t*ram=load_file(argv[2],&n);if(!ram||n!=RAM_SIZE){free(ram);FreeLibrary(h);return 4;}
    CPUState*c=calloc(1,sizeof *c);if(!c){free(ram);FreeLibrary(h);return 4;}
    int ok=check_class0_prop0(m,c,ram)&&check_class8_prop5(m,c,ram)&&
           check_field_header_width(m,ram,argv[3],argv[4]);
    if(ok)puts("Direct original callback checks passed; no filesystem or source assets were modified.");
    free(c);free(ram);FreeLibrary(h);return ok?0:5;
}
