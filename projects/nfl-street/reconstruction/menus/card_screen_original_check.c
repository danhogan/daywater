/* Original source card creation/root activation and scheduled platform-query0 return / platform-query1 nested prompt capture.
   Font/image objects are constructed natively first as an explicit supplied baseline. */
#include <windows.h>
#include "moderngekko/module_abi.h"
#include "card_screen.h"
#include "screen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint32_t be(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void put(uint8_t*p,uint32_t v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static int run(const ModernGekkoModuleDesc*m,CPUState*c,nflstreet_card_screen*a){c->lr=0;c->exception=0;for(unsigned i=0;c->pc&&i<1000000;++i){uint32_t fn=c->pc;
 if(fn==0x80004668){c->gpr[3]=a->ui.options.query_4668;c->pc=c->lr;continue;}
 if(fn==0x800069a8){c->gpr[3]=a->query_69a8;c->pc=c->lr;continue;}
 if(fn==0x80006d6c){c->gpr[3]=a->query_6d6c;c->pc=c->lr;continue;}
 if(fn==0x80006e94){if(c->gpr[3]>2||c->gpr[4]||c->gpr[5]<0x80000000||c->gpr[5]-0x80000000>c->ram_size-17)return 0;memcpy(c->ram+c->gpr[5]-0x80000000,a->device_name,17);c->pc=c->lr;continue;}
 if(fn==0x8014e29c){if(c->gpr[3]!=13)return 0;c->gpr[3]=a->ui.profile[13];c->pc=c->lr;continue;}
 if(fn==0x8018f150){nflstreet_storage_dialog*s=&a->storage;if(s->count>=16)return 0;s->requests[s->count++]=(nflstreet_storage_request){fn,0,{0,0}};c->pc=c->lr;continue;}
 if(fn==0x8018f084||fn==0x8018f01c){nflstreet_storage_dialog*s=&a->storage;if(s->count>=16)return 0;s->requests[s->count++]=(nflstreet_storage_request){fn,1,{c->gpr[3],0}};c->pc=c->lr;continue;}
 if(fn==0x801d5aec){c->gpr[3]=0;c->pc=c->lr;continue;}
 if(fn==0x801d5c24){c->gpr[3]=0x817f0000;c->pc=c->lr;continue;}
 if(fn==0x801d5884){if(c->gpr[4]!=2||(c->gpr[5]!=0&&c->gpr[5]!=2))return 0;c->gpr[3]=c->gpr[5]?0x817f3120:0x817f3100;c->pc=c->lr;continue;}
 if(fn==0x801d4b3c){if(c->gpr[6]==0){if(a->ui.coroutines.count>=8||c->gpr[8]>16)return 0;nflstreet_ui_service_request*q=&a->ui.coroutines.frames[a->ui.coroutines.count++].request;uint8_t*p=c->ram+c->gpr[7]-0x80000000;q->kind=9;q->target=be(p);q->source=q->link=be(p+4);q->argc=c->gpr[8];for(unsigned j=0;j<q->argc;++j)q->args[j]=be(c->ram+c->gpr[9]-0x80000000+j*4);c->pc=c->lr;continue;}if(c->gpr[6]!=1||c->gpr[8]||a->request_count>=8)return 0;nflstreet_ui_service_request*q=a->requests+a->request_count++;uint8_t*p=c->ram+c->gpr[7]-0x80000000;q->kind=9;q->property=1;q->source=(c->gpr[3]&65535)<<16|(c->gpr[4]&65535);q->target=be(p);q->aux=be(p+8);c->pc=c->lr;continue;}
 if(fn==0x801d54a4||fn==0x801d5918){fprintf(stderr,"Unexpected resource I/O%x\n",fn);return 0;}
 c->downcount=-1000000000;m->dispatch(c,c->pc);if(c->exception){fprintf(stderr,"Original exception pc%x\n",c->pc);return 0;}
 }return !c->pc;}
static int compare(uint8_t*b,uint8_t*ram,const nflstreet_card_screen*n,const nflstreet_card_screen*a){
 if(memcmp(b,ram+0x1100000,28838)){for(unsigned i=0;i<28838;++i)if(b[i]!=ram[0x1100000+i]){fprintf(stderr,"File mismatch%x:%x/%x\n",i,b[i],ram[0x1100000+i]);break;}return 0;}
 if(n->ui.coroutines.count!=a->ui.coroutines.count||n->ui.coroutines.count!=be(ram+0x17f0060))return 0;
 if(n->ui.coroutines.count){const nflstreet_ui_continuation*f=n->ui.coroutines.frames;const uint8_t*rec=ram+0x17d0000;if(memcmp(&f->request,&a->ui.coroutines.frames[0].request,sizeof f->request)||f->vm.pc!=be(rec+16)||f->vm.sp*4+0x817c0400!=be(rec+12)||f->vm.root!=be(rec+20)||be(rec+24)!=0x817f0100||be(rec+28)!=0x817c0400||be(rec+36)!=((f->request.target<<16)|(f->request.target>>16))){fprintf(stderr,"Card frozen frame differs PC%x/%x SP%x/%x root%x/%x loaded%x stack%x dest%x/%x requestcmp%d\n",f->vm.pc,be(rec+16),f->vm.sp*4+0x817c0000,be(rec+12),f->vm.root,be(rec+20),be(rec+24),be(rec+28),f->request.target,be(rec+36),memcmp(&f->request,&a->ui.coroutines.frames[0].request,sizeof f->request));return 0;}for(unsigned j=0;j<f->vm.sp;++j)if(f->vm.words[j]!=be(ram+0x17c0400+j*4)){fprintf(stderr,"Card frozen stack differs%u\n",j);return 0;}
 if(n->operation!=be(ram+0x346500)||n->device[0]!=be(ram+0x3a9a40-0x7fa4)||n->device[1]!=be(ram+0x3a9a40-0x7fa0)||strcmp(n->selected_name,(char*)ram+0x346528))return 0;}
 unsigned r=0x3a9a40;if(n->word_5a14!=be(ram+r-0x5a14)||n->byte_6f77!=ram[r-0x6f77]||n->storage.word_5a10!=be(ram+r-0x5a10)||n->storage.word_573c!=be(ram+r-0x573c)||n->storage.count!=a->storage.count||memcmp(n->storage.requests,a->storage.requests,sizeof n->storage.requests)||n->request_count!=a->request_count||memcmp(n->requests,a->requests,sizeof n->requests)){fprintf(stderr,"Fields/effects mismatch;returns%u/%u ext%u/%u\n",n->request_count,a->request_count,n->storage.count,a->storage.count);return 0;}
 for(unsigned j=0;j<5;++j)if(n->storage.callbacks[j]!=be(ram+r-0x6ee8+j*4))return 0;
 if(n->ui.shared.entry_flags!=be(ram+0x17f0100)||n->ui.shared.rates.count!=be(ram+0x17f0054)||n->ui.shared.rates.flags!=be(ram+0x17f0004))return 0;
 for(unsigned i=0;i<n->ui.shared.rates.count;++i)for(unsigned j=0;j<13;++j){uint32_t v=n->ui.shared.rates.records[i].words[j];if(j==7)v=0x817f0100;if(v!=be(ram+0x17e0000+i*52+j*4)){fprintf(stderr,"Rate differs%u:%u\n",i,j);return 0;}}
 return 1;
}
int main(int argc,char**argv){if(argc!=5)return 2;HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;const ModernGekkoModuleDesc*m=get();if(m->cpu_state_size!=sizeof(CPUState))return 3;
 uint8_t*ram=malloc(24*1024*1024),*seed=malloc(24*1024*1024),*source=malloc(28838),*b=malloc(28838);FILE*f=fopen(argv[2],"rb");if(!ram||!seed||!source||!b||!f||fread(seed,1,24*1024*1024,f)!=24*1024*1024)return 4;fclose(f);f=fopen(argv[3],"rb");if(!f||fread(source,1,28838,f)!=28838)return 4;fclose(f);uint32_t relocated;if(!nflstreet_ui_relocate(source,28838,0x81100000,&relocated))return 4;
 nflstreet_card_screen initial={0};initial.ui.shared.rates.clock=16;initial.word_5a14=0x11223344;initial.storage.word_5a10=0xffffffff;initial.storage.word_573c=0xffffffff;
 for(unsigned j=0;j<5;++j)initial.storage.source_callbacks[j]=be(seed+0x2829d0+j*4);
 for(unsigned e=0;e<=2;e+=2){char path[4096];snprintf(path,sizeof path,"%s/%05u.bin",argv[4],e);f=fopen(path,"rb");if(!f)return 4;fseek(f,0,SEEK_END);long z=ftell(f);rewind(f);unsigned at=e?0x1790000:0x17a0000;if(z<32||z>65536||fread(seed+at,1,z,f)!=(size_t)z)return 4;fclose(f);nflstreet_font font;if(!nflstreet_font_open(seed+at,z,&font))return 4;initial.ui.fonts[e]=font;unsigned rec=e?0x17f3020:0x17f3000,asset=e?0x17f3120:0x17f3100;put(seed+rec+4,0x80000000+at);put(seed+rec+8,0x80000000+at+font.glyphs);put(seed+rec+12,0x80000000+at+font.kerning);put(seed+rec+16,0x80000000+at+font.atlas);put(seed+asset,0x80000000+rec);}
 if(!nflstreet_card_screen_construct(source,28838,0x81100000,&initial))return 5;
 for(unsigned active=0;active<2;++active)for(unsigned arg=0;arg<3;++arg)for(unsigned result=0;result<(active?3:1);++result){memcpy(ram,seed,24*1024*1024);memcpy(b,source,28838);memcpy(ram+0x1100000,source,28838);nflstreet_card_screen native=initial,actual={0};native.ui.coroutines.capacity=4;native.ui.options.query_4668=active;native.query_69a8=native.query_6d6c=1;strcpy(native.device_name,"OPTIONS");actual.ui.options.query_4668=active;actual.query_69a8=actual.query_6d6c=1;strcpy(actual.device_name,"OPTIONS");
 memset(ram+0x17f0000,0,0x3000);/* Font asset records were just cleared; restore their backing records. */
 for(unsigned e=0;e<=2;e+=2){unsigned rec=e?0x17f3020:0x17f3000,asset=e?0x17f3120:0x17f3100;memcpy(ram+rec,seed+rec,20);memcpy(ram+asset,seed+asset,4);}
 put(ram+0x17f005c,4);put(ram+0x17f0064,0x817d0000);memset(ram+0x17d0000,0,320);put(ram+0x17f0008,16);put(ram+0x17f000c,0x8012a414);put(ram+0x17f0104,0x60004);put(ram+0x17f0108,0x50000);put(ram+0x17f0110,0x81100000);put(ram+0x17f0048,9);put(ram+0x17f004c,0x817b0000);memset(ram+0x17b0000,0,36);put(ram+0x17b0000,0x801d60dc);put(ram+0x17b0020,0x801d9244);put(ram+0x17f0038,1);put(ram+0x17f003c,0x817f0100);put(ram+0x17f0058,0x817e0000);memset(ram+0x17e0000,0,64*52);put(ram+0x17f0074,0x817c0000);put(ram+0x17f0088,0x817c0400);ram[0x3a9a40-0x6f78]=ram[0x3a9a40-0x7fcc]=ram[0x3a9a40-0x6f77]=0;put(ram+0x3a9a40-0x6f7c,0);put(ram+0x3a9a40-0x5a14,initial.word_5a14);put(ram+0x3a9a40-0x5a10,0xffffffff);put(ram+0x3a9a40-0x573c,0xffffffff);memset(ram+0x3a9a40-0x6ee8,0,20);
 CPUState c={0};c.ram=ram;c.ram_size=24*1024*1024;c.msr=0x2000;c.gpr[1]=0x81700000;c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;
 uint32_t args[2]={arg,0};unsigned scripts=0;if(!nflstreet_card_screen_start(b,28838,0x81100000,&native,args,&scripts)||scripts!=2)return 6;
 for(unsigned pass=0;pass<2;++pass){uint32_t root=pass?0x811000c0:0x81100174,pc=pass?0x81106b94:0x81100c54;uint32_t words[256]={0};words[1]=pass?0xfffe:0;if(pass){words[2]=arg;words[3]=0;words[4]=root;}else words[2]=root;unsigned sp=pass?6:4;for(unsigned j=0;j<256;++j)put(ram+0x17c0000+j*4,words[j]);put(ram+0x17f200c,0x817c0000+sp*4);put(ram+0x17f2010,pc);c.pc=0x801d16a0;c.gpr[3]=0x817f0000;c.gpr[4]=0x817c0000;c.gpr[5]=0x817f2000;c.gpr[6]=0x817f0100;c.gpr[7]=root;if(!run(m,&c,&actual))return 7;}
 if(!compare(b,ram,&native,&actual))return 8;printf("Created card supplied arg%u:byte6F77=%u\n",arg,native.byte_6f77);
 for(unsigned frame=0;frame<160;++frame){if(!nflstreet_card_screen_advance(b,28838,0x81100000,&native,16))return 9;c.pc=0x801cf610;c.gpr[3]=0x817f0000;c.gpr[4]=16;if(!run(m,&c,&actual)||!compare(b,ram,&native,&actual)){fprintf(stderr,"Card frame%u arg%u\n",frame,arg);return 10;}}
 if(active){if(native.request_count||native.ui.coroutines.count!=1)return 11;uint32_t dest=native.ui.coroutines.frames[0].request.target;unsigned completed=0;
 if(!nflstreet_card_screen_complete(b,28838,0x81100000,&native,dest,result,&completed)||!completed)return 12;
 actual.ui.coroutines.count=0;c.pc=0x801d0310;c.gpr[3]=0x817f0000;c.gpr[4]=dest>>16;c.gpr[5]=dest&65535;c.gpr[6]=result;
 if(!run(m,&c,&actual)||!compare(b,ram,&native,&actual))return 13;
 printf("Original active card args%u,0:2 scripts,160 updates;prompt target%x;raw result%u resumption exact;polling continuation remains unsupported;supplied platform/device/name,no prompt rendering or I/O claim\n",arg,dest,result);continue;}
 if(native.request_count!=1)return 11;printf("Original card args%u,0:2 source scripts,160 scheduler updates exact;return raw word%x destination%x;full28838 bytes/records/fields/retained effects;supplied platform query0\n",arg,native.requests[0].aux,native.requests[0].target);
 }
 FreeLibrary(h);return 0;
}
