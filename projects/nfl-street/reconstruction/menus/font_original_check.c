/* Controlled original FNTG lookup/layout/quad oracle. No screen/gameplay claim. */
#include <windows.h>
#include "moderngekko/module_abi.h"
#include "font.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned be(const uint8_t*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static void put(uint8_t*p,unsigned v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static int call(const ModernGekkoModuleDesc*m,CPUState*c,unsigned pc){c->pc=pc;c->lr=0;c->downcount=c->cycle_budget=100000000;c->exception=0;for(unsigned i=0;c->pc&&i<1000000;++i)m->dispatch(c,c->pc);return !c->pc&&!c->exception;}
/* Controlled hardware service boundary: only the cache-flush routine is
   replaced by a no-op. Palette arithmetic/TLUT initialization stays original.
   This is not full original GPU or exception-handler execution. */
static int palette_call(const ModernGekkoModuleDesc*m,CPUState*c,unsigned*flushes){
 c->pc=0x80186e58;c->lr=0;c->downcount=c->cycle_budget=100000000;c->exception=0;*flushes=0;
 for(unsigned i=0;c->pc&&i<1000000;++i){if(c->pc==0x801f0ba0){c->pc=c->lr;++*flushes;}else m->dispatch(c,c->pc);if(c->exception)break;}
 return !c->pc&&!c->exception&&*flushes==2;
}
static void cpu(CPUState*c,uint8_t*ram){memset(c,0,sizeof *c);c->gpr[1]=0x81700000;c->gpr[2]=0x803b9a40;c->gpr[13]=0x803a9a40;c->ram=ram;c->ram_size=24*1024*1024;c->msr=0x2000;}
int main(int argc,char**argv){
 if(argc!=4)return 2;HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;const ModernGekkoModuleDesc*m=get();if(m->cpu_state_size!=sizeof(CPUState))return 3;
 uint8_t*ram=malloc(24*1024*1024);FILE*f=fopen(argv[2],"rb");if(!ram||!f||fread(ram,1,24*1024*1024,f)!=24*1024*1024)return 4;fclose(f);ram[0x3a33f0]=1;
 uint16_t palettes[2][16];nflstreet_font_palettes(palettes);CPUState initial;cpu(&initial,ram);
 unsigned flushes;if(!palette_call(m,&initial,&flushes)){fprintf(stderr,"Palette init pc=%08x exception=%08x\n",initial.pc,initial.exception);return 14;}
 for(unsigned pass=0;pass<2;++pass)for(unsigned i=0;i<16;++i){const uint8_t*p=ram+0x331420+pass*32+i*2;if(((unsigned)p[0]<<8|p[1])!=palettes[pass][i])return 15;}
 unsigned lookups=0,pairs=0,measures=0,layouts=0,quads=0;
 const char*texts[]={"Press START","Quick Game","Inside Street","NFL Challenge","AVATAR To WAVE","","A\nV","A\xffZ","   ","WWWiiii..."};
 for(unsigned font=0;font<4;++font){char name[4096];snprintf(name,sizeof name,"%s/%05u.bin",argv[3],font);f=fopen(name,"rb");if(!f)return 4;fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);if(n<32||n>65536||fread(ram+0x17a0000,1,(size_t)n,f)!=(size_t)n)return 4;fclose(f);
  nflstreet_font native;if(!nflstreet_font_open(ram+0x17a0000,(size_t)n,&native)){fprintf(stderr,"Font open failed %u\n",font);return 5;}
  nflstreet_texture atlas0={0},atlas1={0};if(!nflstreet_font_atlas(&native,0,&atlas0)||!nflstreet_font_atlas(&native,1,&atlas1))return 16;nflstreet_texture_close(&atlas0);nflstreet_texture_close(&atlas1);
  memset(ram+0x17f0000,0,32);put(ram+0x17f0004,0x817a0000);put(ram+0x17f0008,0x817a0000+(unsigned)native.glyphs);put(ram+0x17f000c,native.kerning?0x817a0000+(unsigned)native.kerning:0);put(ram+0x17f0010,0x817a0000+(unsigned)native.atlas);
  for(unsigned code=0;code<512;++code){CPUState c;cpu(&c,ram);c.gpr[3]=0x817f0000;c.gpr[4]=code;if(!call(m,&c,0x80185a98))return 6;const uint8_t*g=nflstreet_font_glyph(&native,code);unsigned want=g?0x817a0000+(unsigned)(g-native.bytes):0;if(c.gpr[3]!=want){fprintf(stderr,"Lookup %u/%u\n",font,code);return 7;}++lookups;}
  unsigned count=native.kerning?be(native.bytes+native.kerning):0;
  for(unsigned i=0;i<count+32;++i){unsigned a=i<count?((unsigned)native.bytes[native.kerning+4+i*8]<<8|native.bytes[native.kerning+5+i*8]):i,b=i<count?((unsigned)native.bytes[native.kerning+6+i*8]<<8|native.bytes[native.kerning+7+i*8]):255-i;
   CPUState c;cpu(&c,ram);c.gpr[3]=0x817f0000;c.gpr[4]=a;c.gpr[5]=b;if(!call(m,&c,0x80185b18)||(int32_t)c.gpr[3]!=nflstreet_font_kerning(&native,a,b))return 8;++pairs;
  }
  for(unsigned text=0;text<10;++text)for(unsigned flags=0;flags<8;++flags){strcpy((char*)ram+0x17e0000,texts[text]);CPUState c;cpu(&c,ram);c.gpr[3]=0x817f0000;c.gpr[4]=0x817e0000;c.gpr[5]=flags;int32_t width;if(!nflstreet_font_measure(&native,texts[text],flags,&width)||!call(m,&c,0x801865fc)||(int32_t)c.gpr[3]!=width)return 9;++measures;
   for(unsigned limit=0;limit<3;++limit){double maximum=limit==0?0:limit==1?37:0.5;nflstreet_glyph_box boxes[354];unsigned used;
    if(!nflstreet_font_layout(&native,texts[text],flags,maximum,boxes,354,&used))return 10;cpu(&c,ram);c.gpr[3]=0x817f0000;c.gpr[4]=0x817e0000;c.gpr[5]=flags;c.gpr[6]=0x817c0000;c.gpr[7]=0x817f1000;c.fpr[1]=maximum;put(ram+0x17f1000,0xaaaaaaaa);
    if(!call(m,&c,0x801863a4)||be(ram+0x17f1000)!=used)return 10;
    for(unsigned j=0;j<used;++j){const uint8_t*p=ram+0x17c0000+j*20;unsigned want[5]={(unsigned)boxes[j].x,(unsigned)boxes[j].y,boxes[j].width,boxes[j].height,(unsigned)boxes[j].u<<16|boxes[j].v};for(unsigned k=0;k<5;++k)if(be(p+k*4)!=want[k]){fprintf(stderr,"Layout font=%u text=%u flags=%u limit=%u glyph=%u word=%u native=%08x original=%08x\n",font,text,flags,limit,j,k,want[k],be(p+k*4));return 11;}
     float quad[16];if(!nflstreet_font_quad(&native,boxes+j,quad))return 12;cpu(&c,ram);c.gpr[3]=0x817f0000;c.gpr[4]=0x817d0000;c.gpr[5]=0x817c0000+j*20;if(!call(m,&c,0x8018675c))return 12;
     for(unsigned vertex=0;vertex<4;++vertex)for(unsigned k=0;k<4;++k){unsigned word;memcpy(&word,quad+vertex*4+k,4);unsigned at=k<2?vertex*16+k*4:64+vertex*16+(k-2)*4;if(word!=be(ram+0x17d0000+at))return 13;}++quads;
    }++layouts;
   }
  }
 }
 printf("Original font checks exact: lookups=%u pairs=%u measurements=%u layouts=%u quads=%u; palettes=32 exact (two cache flushes supplied as no-ops); four source FNTG files, original byte-string geometry only\n",lookups,pairs,measures,layouts,quads);free(ram);FreeLibrary(h);return 0;
}
