/* Full original801D0E18 +801D399C/VM + class callbacks on a supplied main
   screen. Static archive registry and resource I/O supplied; not UI startup. */
#include <windows.h>
#include "moderngekko/module_abi.h"
#include "main_menu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned be(const uint8_t*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static void put(uint8_t*p,unsigned v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static unsigned playback_calls;
static int run(const ModernGekkoModuleDesc*m,CPUState*c,nflstreet_ui_main_menu*s){
 if(!c->pc)c->pc=0x801d0e18;c->lr=0;c->exception=0;c->downcount=c->cycle_budget=100000000;
 for(unsigned i=0;c->pc&&i<1000000;++i){
  if(c->pc==0x801d5c24){if(c->gpr[3]!=1)return 0;c->gpr[3]=0x817f0000;c->pc=c->lr;}
  else if(c->pc==0x801d5884){if(c->gpr[4]!=2||c->gpr[5]!=3)return 0;c->gpr[3]=0x817f3100;c->pc=c->lr;}
  else if(c->pc==0x801d4b3c){
   if(c->gpr[6]==5){if(s->pending_count>=128||c->gpr[8]!=1)return 0;unsigned payload=c->gpr[7]-0x80000000;nflstreet_ui_service_request*q=s->pending+s->pending_count++;memset(q,0,sizeof *q);q->kind=7;q->property=(uint32_t)(int32_t)(int16_t)(be(c->ram+payload)>>16);q->target=be(c->ram+payload+8);q->argc=2;q->args[0]=be(c->ram+payload+4);q->args[1]=be(c->ram+c->gpr[9]-0x80000000);}
   else{if(c->gpr[6]!=9||s->broadcast_count>=128||c->gpr[8]>16)return 0;nflstreet_ui_service_request*q=s->broadcasts+s->broadcast_count++;memset(q,0,sizeof *q);q->kind=2;q->property=be(c->ram+c->gpr[7]-0x80000000);q->argc=c->gpr[8];for(unsigned j=0;j<q->argc;++j)q->args[j]=be(c->ram+c->gpr[9]-0x80000000+j*4);}c->pc=c->lr;}

  else if(c->pc==0x8012cb94){if(c->gpr[3]&&c->gpr[4]==0){if(s->audio_count>=128||c->gpr[5]!=1)return 0;s->audio[s->audio_count++]=be(c->ram+c->gpr[6]-0x80000000);}m->dispatch(c,c->pc);}
  else if(c->pc==0x8013d854){++playback_calls;c->pc=c->lr;}
  else if(c->pc==0x801d5918||c->pc==0x801d54a4){if(s->resource_count>=128)return 0;s->resources[s->resource_count++]=(nflstreet_ui_button_effect){c->pc==0x801d54a4,c->gpr[4],c->gpr[5]};c->pc=c->lr;}
  else if(c->pc==0x801878b0||c->pc==0x801af944||c->pc==0x801af9cc||c->pc==0x801af57c){c->pc=c->lr;}
  else m->dispatch(c,c->pc);if(c->exception)break;
 }if(c->pc||c->exception)fprintf(stderr,"Original focus stopped pc=%x exception=%x\n",c->pc,c->exception);return !c->pc&&!c->exception;
}
int main(int argc,char**argv){if(argc!=5)return 2;HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;const ModernGekkoModuleDesc*m=get();if(m->cpu_state_size!=sizeof(CPUState))return 3;
 uint8_t*ram=malloc(24*1024*1024);FILE*f=fopen(argv[2],"rb");if(!ram||!f||fread(ram,1,24*1024*1024,f)!=24*1024*1024)return 4;fclose(f);f=fopen(argv[3],"rb");if(!f)return 4;fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);if(n<32||n>1000000)return 4;uint8_t*b=malloc(n);if(!b||fread(b,1,n,f)!=(size_t)n)return 4;fclose(f);unsigned result;if(!nflstreet_ui_relocate(b,n,0x81100000,&result))return 5;
 unsigned table=be(b+4)-0x81100000;for(unsigned i=0;i<be(b);++i)put(b+be(b+table+i*20)-0x81100000+4,0);
 memcpy(ram+0x1100000,b,n);memset(ram+0x17f0000,0,0x200);put(ram+0x17f0038,1);put(ram+0x17f003c,0x817f0100);put(ram+0x17f0104,1);put(ram+0x17f0110,0x81100000);put(ram+0x17f0048,9);put(ram+0x17f004c,0x817b0380);put(ram+0x17b0380,0x801d60dc);put(ram+0x17b0384,0x8012cb94);put(ram+0x17b03a0,0x801d9244);put(ram+0x17f0074,0x817c0000);put(ram+0x17f0008,16);put(ram+0x17f000c,0x8012a414);
 memset(ram+0x17b0000,0,0x380);put(ram+0x3a9a40-0x63dc,0x817b0000);put(ram+0x17b000c,0x817b0040);put(ram+0x17b0040+3*0x24+0x14,0x817b0200);
 CPUState c={0};c.gpr[1]=0x81700000;c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;c.ram=ram;c.ram_size=24*1024*1024;c.msr=0x2000;
 f=fopen(argv[4],"rb");if(!f)return 4;fseek(f,0,SEEK_END);long fontsize=ftell(f);rewind(f);if(fontsize<32||fontsize>65536||fread(ram+0x17a0000,1,fontsize,f)!=(size_t)fontsize)return 4;fclose(f);nflstreet_font font;if(!nflstreet_font_open(ram+0x17a0000,fontsize,&font))return 5;
 put(ram+0x17f3004,0x817a0000);put(ram+0x17f3008,0x817a0000+font.glyphs);put(ram+0x17f300c,0x817a0000+font.kerning);put(ram+0x17f3010,0x817a0000+font.atlas);put(ram+0x17f3100,0x817f3000);
 if(!nflstreet_ui_main_text_bind(b,n,0x81100000))return 5;memcpy(ram+0x1100000,b,n);
 uint8_t*layoutbase=malloc(n);if(!layoutbase)return 4;memcpy(layoutbase,b,n);nflstreet_ui_main_menu layout={0},layoutoriginal={0};
 for(unsigned pass=0;pass<5;++pass){unsigned mode=(pass+1)%5;memcpy(b,layoutbase,n);memcpy(ram+0x1100000,b,n);memset(&layout,0,sizeof layout);memset(&layoutoriginal,0,sizeof layoutoriginal);layout.font=font;
 if(!nflstreet_ui_main_layout(b,n,0x81100000,&layout,mode)){fprintf(stderr,"Native layout failed mode=%u\n",mode);return 6;}
 memset(ram+0x17c0000,0,1024);put(ram+0x17c0004,mode);put(ram+0x17c0008,0x81100960);put(ram+0x3a9a40-0x6e68,0);put(ram+0x17f200c,0x817c0010);put(ram+0x17f2010,0x8110b229);
 c.gpr[3]=0x817f0000;c.gpr[4]=0x817c0000;c.gpr[5]=0x817f2000;c.gpr[6]=0x817f0100;c.gpr[7]=0x81100960;c.pc=0x801d16a0;
 if(!run(m,&c,&layoutoriginal)||memcmp(b,ram+0x1100000,n)||layout.callback_word_6e68!=be(ram+0x3a9a40-0x6e68)){for(unsigned off=0;off<(unsigned)n;++off)if(b[off]!=ram[0x1100000+off]){fprintf(stderr,"Layout difference mode=%u offset=%x native=%x original=%x\n",mode,off,b[off],ram[0x1100000+off]);break;}return 7;}
 }free(layoutbase);
 puts("Original main layout modes0..4 exact:5 complete owned files,30 labels/state updates; supplied font/class text pointers; not full activation");
 unsigned choices[]={1,4,7,10,13,17,17,13,10,7,4,1,1,7,13,4,17,10};nflstreet_ui_main_menu native=layout,original=layoutoriginal;unsigned checked=0;
 for(unsigned i=0;i<sizeof choices/sizeof *choices;++i){unsigned node=choices[i];nflstreet_ui_focus_effects effects={0};
  if(!nflstreet_ui_main_select(b,n,0x81100000,&native,node,&effects)){fprintf(stderr,"Native focus failed step=%u node=%u\n",i,node);return 6;}
  c.gpr[3]=0x817f0000;c.gpr[4]=1;c.gpr[5]=0;c.gpr[6]=be(b+table+node*20);c.gpr[7]=0;c.gpr[8]=1;c.gpr[9]=0;
  if(!run(m,&c,&original)||memcmp(b,ram+0x1100000,n)||native.entry_flags!=be(ram+0x17f0100)||native.resource_count!=original.resource_count||memcmp(native.resources,original.resources,native.resource_count*sizeof *native.resources)||native.broadcast_count!=original.broadcast_count||memcmp(native.broadcasts,original.broadcasts,native.broadcast_count*sizeof *native.broadcasts)){
   for(unsigned off=0;off<(unsigned)n;++off)if(b[off]!=ram[0x1100000+off]){fprintf(stderr,"First difference=%x native=%x original=%x\n",off,b[off],ram[0x1100000+off]);break;}fprintf(stderr,"Focus execution mismatch step=%u node=%u resources=%u/%u broadcasts=%u/%u\n",i,node,native.resource_count,original.resource_count,native.broadcast_count,original.broadcast_count);return 7;
  }++checked;
 }
 /* Source numeric direction events, then original focus with actual optional list. */
 nflstreet_ui_focus_effects initial={0};if(!nflstreet_ui_main_select(b,n,0x81100000,&native,17,&initial))return 9;
 c.gpr[3]=0x817f0000;c.gpr[4]=1;c.gpr[5]=0;c.gpr[6]=be(b+table+17*20);c.gpr[7]=0;c.gpr[8]=1;c.gpr[9]=0;if(!run(m,&c,&original))return 9;
 unsigned moves=0;unsigned direct_broadcasts=native.broadcast_count-1;
 for(unsigned i=0;i<40;++i){unsigned node=20;for(unsigned j=0;j<20;++j)if(be(b+be(b+table+j*20)-0x81100000+4)){node=j;break;}if(node==20)return 9;unsigned event=i<12?3:i<24?2:(i&1?2:3),first=original.pending_count,script,found;
  if(!nflstreet_ui_event(b,n,0x81100000,node,0,event,node,&script,&found)||!found||!nflstreet_ui_main_move(b,n,0x81100000,&native,node,event)){fprintf(stderr,"Native direction failed step=%u node=%u event=%u\n",i,node,event);return 9;}
  memset(ram+0x17c0000,0,1024);put(ram+0x17c0004,event);put(ram+0x17c0008,0x3f800000);put(ram+0x17c000c,be(b+table+node*20));put(ram+0x17f200c,0x817c0014);put(ram+0x17f2010,0x81100000+script);
  c.gpr[3]=0x817f0000;c.gpr[4]=0x817c0000;c.gpr[5]=0x817f2000;c.gpr[6]=0x817f0100;c.gpr[7]=be(b+table+node*20);c.pc=0x801d16a0;if(!run(m,&c,&original))return 9;
  for(unsigned qi=first;qi<original.pending_count;++qi){nflstreet_ui_service_request q=original.pending[qi];c.gpr[3]=0x817f0000;c.gpr[4]=1;c.gpr[5]=q.property;c.gpr[6]=q.target;c.gpr[7]=q.args[0];c.gpr[8]=1;c.gpr[9]=0;if(!run(m,&c,&original))return 9;}
  if(memcmp(b,ram+0x1100000,n)||native.audio_count!=original.audio_count||memcmp(native.audio,original.audio,native.audio_count*4)||native.pending_count!=original.pending_count||memcmp(native.pending,original.pending,native.pending_count*sizeof *native.pending)||native.broadcast_count!=original.broadcast_count||memcmp(native.broadcasts,original.broadcasts,native.broadcast_count*sizeof *native.broadcasts)||native.entry_flags!=be(ram+0x17f0100)){for(unsigned off=0;off<(unsigned)n;++off)if(b[off]!=ram[0x1100000+off]){fprintf(stderr,"Direction difference offset=%x native=%x original=%x\n",off,b[off],ram[0x1100000+off]);break;}fprintf(stderr,"audio=%u/%u first=%x/%x pending target=%x/%x prop=%x/%x args=%x,%x/%x,%x flags=%x/%x\n",native.audio_count,original.audio_count,native.audio[0],original.audio[0],native.pending[0].target,original.pending[0].target,native.pending[0].property,original.pending[0].property,native.pending[0].args[0],native.pending[0].args[1],original.pending[0].args[0],original.pending[0].args[1],native.entry_flags,be(ram+0x17f0100));fprintf(stderr,"Direction mismatch step=%u node=%u event=%u pending=%u/%u broadcasts=%u/%u\n",i,node,event,native.pending_count,original.pending_count,native.broadcast_count,original.broadcast_count);return 9;}++moves;
 }
 if(playback_calls!=original.audio_count)return 9;printf("Observed sound boundary:%u retained class1 requests/%u original playback calls; no native audio playback\n",native.audio_count,playback_calls);
 printf("Source direction scripts and indexed focus exact:%u sequential events,%u queued focus requests; supplied pressed word3F800000, not physical PAD routing\n",moves,native.pending_count);
 unsigned guards=0;uint8_t*before=malloc(n);if(!before)return 4;
 /* Failure after prior deselection/color work must roll back the whole join. */
 nflstreet_ui_main_menu guarded=native;guarded.broadcast_count=128;nflstreet_ui_main_menu old=guarded;memcpy(before,b,n);nflstreet_ui_focus_effects effects,old_effects;memset(&effects,0xa5,sizeof effects);old_effects=effects;
 if(nflstreet_ui_main_select(b,n,0x81100000,&guarded,17,&effects)||memcmp(before,b,n)||memcmp(&old,&guarded,sizeof old)||memcmp(&effects,&old_effects,sizeof effects))return 8;++guards;
 guarded=native;guarded.audio_count=128;old=guarded;memcpy(before,b,n);unsigned current=20;for(unsigned j=0;j<20;++j)if(be(b+be(b+table+j*20)-0x81100000+4)){current=j;break;}
 if(current==20||nflstreet_ui_main_move(b,n,0x81100000,&guarded,current,3)||memcmp(before,b,n)||memcmp(&old,&guarded,sizeof old))return 8;++guards;
 /* Source layout fails at its final callback, after all six label writes. */
 b[0xbb21]=0xff;memcpy(before,b,n);old=guarded;
 if(nflstreet_ui_main_layout(b,n,0x81100000,&guarded,0)||memcmp(before,b,n)||memcmp(&old,&guarded,sizeof old))return 8;++guards;free(before);
 printf("Atomic main layout/focus/direction rollback guards:%u\n",guards);
 printf("Full original focus+VM/class callbacks exact: %u sequential selections, %u resource requests, %u broadcasts; all6 choices/reselection/reverse order; supplied static archive, no physical input or full activation claim\n",checked,native.resource_count,direct_broadcasts);
 free(b);free(ram);FreeLibrary(h);return 0;
}
