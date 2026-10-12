/* Controlled joined four-channel transition oracle, not full gameplay. */
#include <windows.h>
#include "moderngekko/module_abi.h"
#include "../actor_target_input/locomotion_selection.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
static unsigned u32(const uint8_t*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static void put(uint8_t*p,unsigned v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static float real(const uint8_t*p){unsigned w=u32(p);float f;memcpy(&f,&w,4);return f;}
static void scalar(uint8_t*p,float f){unsigned w;memcpy(&w,&f,4);put(p,w);}
static uint8_t*read(const char*path,size_t*n){FILE*f=fopen(path,"rb");if(!f)return NULL;fseek(f,0,SEEK_END);long size=ftell(f);rewind(f);if(size<0){fclose(f);return NULL;}uint8_t*b=malloc(size);if(!b||fread(b,1,size,f)!=(size_t)size){free(b);b=NULL;}fclose(f);*n=size;return b;}
static int call(const ModernGekkoModuleDesc*m,CPUState*c,unsigned pc){c->pc=pc;c->lr=0;c->downcount=c->cycle_budget=100000000;c->exception=0;for(unsigned i=0;c->pc&&i<1000000;++i){m->dispatch(c,c->pc);if(c->exception)break;}if(c->pc||c->exception){fprintf(stderr,"Original call %08x failed pc=%08x ex=%08x\n",pc,c->pc,c->exception);return 0;}return 1;}
int main(int argc,char**argv){
 if(argc!=4)return 2;HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;
 ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;
 const ModernGekkoModuleDesc*m=get();if(m->cpu_state_size!=sizeof(CPUState))return 4;
 size_t rn,pn;uint8_t*ram=read(argv[2],&rn),*profile=read(argv[3],&pn);if(!ram||rn!=24*1024*1024||!profile||pn<20)return 5;
 if(u32(ram+0x24f9c8)!=0)return 6;
 nflstreet_curve_source curves[3];size_t at=12;
 for(unsigned i=0;i<3;++i){if(at+8>pn)return 7;unsigned n=u32(profile+at+4);if(n<24||n>1600||at+8+n>pn)return 7;
  curves[i]=(nflstreet_curve_source){0x817f3000+i*0x800,profile[at],real(profile+at+8+12),real(profile+at+8+16)};
  memcpy(ram+0x17f3000+i*0x800,profile+at+8,n);at+=8+n;
 }if(at!=pn)return 7;
 CPUState c={0};c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;c.gpr[1]=0x81700000;c.ram=ram;c.ram_size=rn;
 c.msr=0x2000;c.hid2=0xe0000000;c.gqr[6]=0x0f070007;
 float speeds[]={0,-0.0f,-0.2f,0.4f,0.1f,curves[1].minimum,curves[1].maximum,
  nextafterf(curves[1].minimum,-INFINITY),nextafterf(curves[1].maximum,INFINITY),
  curves[2].minimum,curves[2].maximum,nextafterf(curves[2].minimum,-INFINITY),nextafterf(curves[2].maximum,INFINITY)};
 unsigned checks=0,changed=0,retained=0,doubled=0;
 for(unsigned alias=0;alias<2;++alias){nflstreet_curve_source source[3];memcpy(source,curves,sizeof source);if(alias)source[1]=source[0];
  for(unsigned start=0;start<3;++start)for(unsigned si=0;si<sizeof speeds/sizeof *speeds;++si)
  for(unsigned comparison=0;comparison<3;++comparison)for(unsigned bounds=0;bounds<3;++bounds)for(unsigned status=0;status<3;++status){
   nflstreet_curve_inputs in={speeds[si],comparison==0?-0.1f:comparison==1?0:0.1f,0.1f,bounds==0?0.05f:bounds==1?0.1f:0.2f,status==0?0:status==1?10:255};
   nflstreet_curve_decision before={source[start].identity,source[start].bank,91,123},ours;
   if(!nflstreet_locomotion_curve_select(&in,source,&before,&ours))return 8;
   uint8_t*d=ram+0x17f5000;memset(d,0x5a,64);put(d+12,before.identity);d[0x36]=before.bank;
   for(unsigned i=0;i<3;++i){put(d+16+i*4,source[i].identity);d[0x37+i]=source[i].bank;}
   scalar(ram+0x17f61c4,in.speed);scalar(ram+0x17f61dc,in.comparison);scalar(ram+0x17f61e8,in.bound_a);scalar(ram+0x17f61ec,in.bound_b);ram[0x17f621e]=in.status;
   ram[0x17f7000]=before.duration;c.gpr[3]=0x817f6000;c.gpr[4]=0x817f5000;c.gpr[5]=0x817f7000;
   if(!call(m,&c,0x80062624))return 9;
   if(c.gpr[3]!=ours.changed||u32(d+12)!=ours.identity||d[0x36]!=ours.bank||ram[0x17f7000]!=ours.duration){
    fprintf(stderr,"curve mismatch alias=%u start=%u speed=%g compare=%g bounds=%g/%g status=%u duration=%u/%u change=%u/%u\n",alias,start,in.speed,in.comparison,in.bound_a,in.bound_b,in.status,ours.duration,ram[0x17f7000],ours.changed,c.gpr[3]);return 10;}
   ++checks;changed+=ours.changed;retained+=ours.duration==91;doubled+=ours.duration==12||ours.duration==16;
  }
 }
 nflstreet_curve_inputs invalid={NAN,0,0,0,0};nflstreet_curve_decision old={curves[0].identity,2,91,123},out=old;
 if(nflstreet_locomotion_curve_select(&invalid,curves,&old,&out)||memcmp(&out,&old,sizeof out))return 11;
 printf("%u original curve decisions exact; %u changes, %u untouched durations, %u doubled durations; alias and inclusive/exclusive bounds covered; invalid-input rollback passed\n",checks,changed,retained,doubled);
 free(ram);free(profile);FreeLibrary(h);return 0;
}
