#include "script.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>
static uint32_t be(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void put(uint8_t*p,uint32_t v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static int range(size_t n,uint32_t base,uint32_t addr,size_t z){return addr>=base&&(size_t)(addr-base)<=n&&z<=n-(addr-base);}
static int access(uint8_t*b,size_t n,uint32_t base,uint32_t stackbase,nflstreet_ui_script*s,uint32_t addr,uint32_t*v,int write){
 if(range(n,base,addr,4)){if(write)put(b+addr-base,*v);else *v=be(b+addr-base);return 1;}
 if(range(sizeof s->words,stackbase,addr,4)&&!((addr-stackbase)&3)){unsigned i=(addr-stackbase)/4;if(write)s->words[i]=*v;else *v=s->words[i];return 1;}return 0;
}
static int local_range(size_t n,uint32_t base,uint32_t stackbase,nflstreet_ui_script*s,uint32_t addr,size_t z){
 return range(n,base,addr,z)||(range(sizeof s->words,stackbase,addr,z)&&!((addr-stackbase)&3));
}
static int byte_access(uint8_t*b,size_t n,uint32_t base,uint32_t stackbase,nflstreet_ui_script*s,uint32_t addr,uint8_t*v,int write){
 if(range(n,base,addr,1)){if(write)b[addr-base]=*v;else *v=b[addr-base];return 1;}
 if(range(sizeof s->words,stackbase,addr,1)){unsigned offset=addr-stackbase,index=offset/4,shift=24-(offset&3)*8;uint32_t mask=0xffu<<shift;if(write)s->words[index]=(s->words[index]&~mask)|(uint32_t)*v<<shift;else *v=(uint8_t)(s->words[index]>>shift);return 1;}return 0;
}
static int push(nflstreet_ui_script*s,uint32_t v){if(s->sp>=256)return 0;s->words[s->sp++]=v;return 1;}
static float fl(uint32_t w){float f;memcpy(&f,&w,4);return f;}
static uint32_t word(float f){uint32_t w;memcpy(&w,&f,4);return w;}
static int invoke(nflstreet_ui_script_service service,nflstreet_ui_script_memory_service memory,void*ctx,uint8_t*b,size_t n,uint32_t base,const nflstreet_ui_service_request*q,uint32_t*out,uint32_t stackbase,uint32_t words[256]){return memory?memory(ctx,b,n,base,q,out,stackbase,words):service?service(ctx,b,n,base,q,out):0;}
static int step(uint8_t*b,size_t n,uint32_t base,uint32_t stackbase,nflstreet_ui_script*s,nflstreet_ui_script_service service,void*ctx,nflstreet_ui_script_capture capture,nflstreet_ui_script_memory_service memory){
 if(!range(n,base,s->pc,1)||s->sp>256)return 0;unsigned op=b[s->pc++-base];uint32_t a=0,v=0;
 if(op==0x10||op==0x11||op==0x43){if(!range(n,base,s->pc,4))return 0;v=be(b+s->pc-base);s->pc+=4;
  if(op==0x43){if(v>=n||!push(s,s->pc))return 0;s->pc=base+v;return 1;}return push(s,v);
 }
 if(op==0x0e){if(s->sp<2)return 0;unsigned dest=s->words[--s->sp],src=s->words[--s->sp];if(!src||!dest)return 1;if(!range(n,base,src,12)||!range(n,base,dest,12))return 0;unsigned count=be(b+src-base+4),cap=be(b+dest-base+4);if(cap<count)count=cap;unsigned from=be(b+src-base+8),to=be(b+dest-base+8);if(!range(n,base,from,count)||!range(n,base,to,(size_t)count+1))return 0;memmove(b+to-base,b+from-base,count);b[to-base+count]=0;return 1;}
 /* Original7A/7B change ASCII case over declared capacity, then pop. */
 if(op==0x7a||op==0x7b){
  if(!s->sp)return 0;uint32_t descriptor=s->words[--s->sp];if(!descriptor)return 1;
  uint32_t cap,data;if(descriptor>UINT32_MAX-8||!access(b,n,base,stackbase,s,descriptor+4,&cap,0))return 0;
  if(!cap)return 1;if(!access(b,n,base,stackbase,s,descriptor+8,&data,0)||!local_range(n,base,stackbase,s,data,cap))return 0;
  if(data<(uint64_t)descriptor+12&&descriptor<(uint64_t)data+cap)return 0;
  for(uint32_t i=0;i<cap;++i){uint8_t c;if(!byte_access(b,n,base,stackbase,s,data+i,&c,0))return 0;
   if(op==0x7a&&c>='a'&&c<='z')c-=32;else if(op==0x7b&&c>='A'&&c<='Z')c+=32;
   if(!byte_access(b,n,base,stackbase,s,data+i,&c,1))return 0;
  }return 1;
 }
 /* Original7D pops a diagnostic string. This API supplies no debug callback. */
 if(op==0x7d){if(!s->sp)return 0;--s->sp;return 1;}
 if(op==0x0d)return push(s,base);
 if(op==0x0a||op==0x0b||op==0x76||op==0x58||op==6||op==7){
  if(!service&&!memory)return 0;nflstreet_ui_service_request q={0};int result=-1;
  if(op==0x0a){if(s->sp<4)return 0;q.kind=0;q.argc=s->words[--s->sp];q.class_id=s->words[--s->sp];q.property=s->words[--s->sp];a=s->words[--s->sp];
   if(a){if(!access(b,n,base,stackbase,s,a,&q.target,0)||q.target>UINT32_MAX-base)return 0;q.target+=base;}
   if(q.argc>16||s->sp<=q.argc)return 0;memcpy(q.args,s->words+s->sp-q.argc,q.argc*4);result=(int)(s->sp-q.argc-1);
  }else if(op==0x0b){if(s->sp<2)return 0;q.kind=1;q.argc=s->words[--s->sp];--s->sp;
   if(q.argc>16||s->sp<=q.argc||s->sp+1>256)return 0;q.property=s->words[s->sp-q.argc];memcpy(q.args,s->words+s->sp-q.argc+1,q.argc*4);result=(int)(s->sp-q.argc-1);
  }else if(op==0x76){if(s->sp<2)return 0;q.kind=2;q.argc=s->words[--s->sp];--s->sp;
   if(q.argc>15||s->sp<q.argc||s->sp+1>256)return 0;q.property=s->words[s->sp-q.argc];memcpy(q.args,s->words+s->sp-q.argc+1,q.argc*4);
  }else if(op==6||op==7){q.kind=op==6?4:5;q.argc=op==6?4:2;if(s->sp<q.argc)return 0;for(unsigned i=0;i<q.argc;++i)q.args[i]=s->words[--s->sp];
  }else{if(!range(n,base,s->pc,1))return 0;q.kind=3;q.class_id=b[s->pc++-base];if(q.class_id>6)return 0;
   unsigned count=q.class_id>5?8:q.class_id>4?7:6;if(s->sp<count)return 0;q.argc=count;for(unsigned i=0;i<count;++i)q.args[i]=s->words[--s->sp];
  }
  v=result>=0?s->words[result]:0;if(memory)return invoke(service,memory,ctx,b,n,base,&q,result>=0?&s->words[result]:&v,stackbase,s->words);if(!invoke(service,memory,ctx,b,n,base,&q,&v,stackbase,s->words))return 0;if(result>=0)s->words[result]=v;return 1;
 }
 if(op==0x70){if(!capture||!range(n,base,s->pc,1))return 0;unsigned argc=b[s->pc++-base];if(argc>16||s->sp<argc+2)return 0;
  nflstreet_ui_service_request q={0};q.kind=9;q.source=s->current_screen;q.link=s->current_screen;q.argc=argc;unsigned slot=s->sp-argc-1,v=s->words[slot];q.target=(v<<16)|(v>>16);memcpy(q.args,s->words+slot+1,argc*4);
  nflstreet_ui_script frozen=*s;frozen.sp=slot;frozen.suspension=3;int status=capture(ctx,&frozen,&q);if(status==1){*s=frozen;return 1;}if(status==2){s->sp-=argc;return 1;}return 0;
 }
 if(op==2||op==0x71||op==3||op==8){if(!service&&!memory)return 0;nflstreet_ui_service_request q={0};q.kind=9;q.source=s->current_screen;
  if(op==2||op==0x71){if(!range(n,base,s->pc,1))return 0;q.argc=b[s->pc++-base];if(q.argc>16||s->sp<q.argc+1)return 0;unsigned slot=s->sp-q.argc-1;v=s->words[slot];q.target=(v<<16)|(v>>16);q.link=s->current_screen;memcpy(q.args,s->words+slot+1,q.argc*4);s->sp=slot;
  }else{unsigned count=op==3?2:1;if(s->sp<count)return 0;if(op==3)q.aux=s->words[--s->sp];v=s->words[--s->sp];q.target=(v&65535)==65535?s->current_screen:(v<<16)|(v>>16);q.property=op==3?1:3;}
  unsigned ignored=0;return invoke(service,memory,ctx,b,n,base,&q,&ignored,stackbase,s->words);
 }
 /* Original72 queues kind8: move target loaded-screen record by signed aux.
    This is ordering, not a screen-link assignment. Payload also retains source. */
 if(op==0x72){if((!service&&!memory)||s->sp<2)return 0;nflstreet_ui_service_request q={0};q.kind=9;q.property=8;q.source=s->current_screen;q.link=s->current_screen;q.aux=s->words[--s->sp];v=s->words[--s->sp];q.target=(v<<16)|(v>>16);unsigned ignored=0;return invoke(service,memory,ctx,b,n,base,&q,&ignored,stackbase,s->words);}
 if(op==0x48){if(!service&&!memory)return 0;nflstreet_ui_service_request q={0};q.kind=8;q.property=3;q.target=s->linked_screen;unsigned ignored=0;return invoke(service,memory,ctx,b,n,base,&q,&ignored,stackbase,s->words);}
 if(op==0x47){if((!service&&!memory)||s->sp<3)return 0;unsigned direction=s->words[--s->sp],cell=s->words[--s->sp],index=s->words[--s->sp],relative=0;if(!access(b,n,base,stackbase,s,cell,&relative,0)||relative>UINT32_MAX-base)return 0;unsigned list=base+relative;if(!range(n,base,list,8))return 0;unsigned count=be(b+list-base);if((int32_t)index<0)return 0;if(index>=count)return 1;if(count>1024||!range(n,base,list+8,(size_t)count*4))return 0;unsigned target=be(b+list-base+8+index*4);if(target>UINT32_MAX-base)return 0;target+=base;if(!range(n,base,target,8))return 0;if(target==s->root&&be(b+target-base+4))return 1;nflstreet_ui_service_request q={0};q.kind=7;q.target=target;q.property=direction;q.argc=2;q.args[0]=list;q.args[1]=index;unsigned ignored=0;return invoke(service,memory,ctx,b,n,base,&q,&ignored,stackbase,s->words);}
 if(op==0x59){if((!service&&!memory)||!s->sp||!range(n,base,s->pc,7))return 0;
  unsigned off=be(b+s->pc-base);int16_t index=(int16_t)((unsigned)b[s->pc-base+4]<<8|b[s->pc-base+5]);unsigned mode=b[s->pc-base+6];s->pc+=7;
  int64_t slot=(int64_t)s->sp+index;if(slot<0||slot>=256||off>UINT32_MAX-s->words[slot])return 0;
  nflstreet_ui_service_request q={0};q.kind=6;q.target=s->words[slot]+off;q.class_id=mode;q.argc=1;q.args[0]=s->words[--s->sp];uint32_t ignored=0;return invoke(service,memory,ctx,b,n,base,&q,&ignored,stackbase,s->words);
 }
 if(op==0x4f){s->flags=0;return 1;}if(op==0x50){s->flags=UINT32_MAX;return 1;}
 if(op==0x55||op==0x56||op==0x57||op==0x6b){if(!range(n,base,s->pc,6))return 0;
  v=be(b+s->pc-base);int16_t index=(int16_t)((unsigned)b[s->pc-base+4]<<8|b[s->pc-base+5]);s->pc+=6;
  int64_t slot=(int64_t)s->sp+index;if(slot<0||slot>=256)return 0;a=s->words[slot];if(v>UINT32_MAX-a)return 0;a+=v;
  if(op==0x57||op==0x6b)return push(s,a);
  if(op==0x55){if(!access(b,n,base,stackbase,s,a,&v,0))return 0;return push(s,v);}
  if(!s->sp)return 0;v=s->words[--s->sp];return access(b,n,base,stackbase,s,a,&v,1);
 }
 /* Original5F/60 and 6E address bounded source arrays with dimensions in their header. */
 if((op>=0x5b&&op<=0x62)||op==0x6e){
  unsigned array,extra=(op==0x5f||op==0x60||op==0x6e)?0:1,write=op==0x5c||op==0x5e||op==0x60||op==0x62;
  if(!extra){if(!range(n,base,s->pc,6))return 0;unsigned off=be(b+s->pc-base);int16_t index=(int16_t)((unsigned)b[s->pc-base+4]<<8|b[s->pc-base+5]);s->pc+=6;int64_t slot=(int64_t)s->sp+index;if(slot<0||slot>=256||off>UINT32_MAX-s->words[slot])return 0;array=s->words[slot]+off;
  }else{if(!s->sp)return 0;uint32_t top=s->words[s->sp-1];
   if(op==0x5d||op==0x5e)array=top;
   else{int64_t slot=(int64_t)s->sp+(int32_t)top-1;if(slot<0||slot>=256)return 0;array=(op==0x5b||op==0x5c)?stackbase+(uint32_t)slot*4:s->words[slot];}
  }
  unsigned dims=0;if(!access(b,n,base,stackbase,s,array,&dims,0)||dims>8||s->sp<dims+extra+write||array>UINT32_MAX-(dims+1)*4)return 0;
  uint64_t offset=0,stride=1;unsigned consumed=extra+write;
  for(unsigned j=0;j<dims;++j){unsigned size=0;if(!access(b,n,base,stackbase,s,array+4+j*4,&size,0)||!size||size>INT32_MAX)return 0;unsigned value=s->words[s->sp-consumed-1-j];if((int32_t)value<0||value>=size)value=size-1;offset+=stride*value;stride*=size;if(stride>UINT32_MAX/4||offset>UINT32_MAX/4)return 0;}
  uint64_t addr=(uint64_t)array+(dims+1+offset)*4;if(addr>UINT32_MAX)return 0;
  if(op==0x6e){if(!local_range(n,base,stackbase,s,(unsigned)addr,4))return 0;s->sp-=dims;return push(s,(unsigned)addr);}
  if(write){unsigned value=s->words[s->sp-extra-1];if(!access(b,n,base,stackbase,s,(unsigned)addr,&value,1))return 0;s->sp-=dims+extra+1;return 1;}
  unsigned value=0;if(!access(b,n,base,stackbase,s,(unsigned)addr,&value,0))return 0;s->sp-=dims+extra;return push(s,value);
 }
 /* Original4A/4B index a byte descriptor: capacity at+4, byte pointer at+8. */
 if(op==0x4a){if(s->sp<3)return 0;uint32_t index=s->words[s->sp-1],descriptor=s->words[s->sp-2],value=0;
  if(descriptor){uint32_t capacity=0,data=0;if(!local_range(n,base,stackbase,s,descriptor,12)||descriptor>UINT32_MAX-8||!access(b,n,base,stackbase,s,descriptor+4,&capacity,0))return 0;
   if(index<capacity){uint8_t byte;if(!access(b,n,base,stackbase,s,descriptor+8,&data,0)||index>UINT32_MAX-data||!byte_access(b,n,base,stackbase,s,data+index,&byte,0))return 0;value=byte<0x80?byte:0xffffff00u|byte;}
  }
  s->words[s->sp-3]=value;s->sp-=2;return 1;
 }
 if(op==0x4b){if(s->sp<3)return 0;uint32_t value=s->words[s->sp-1],index=s->words[s->sp-2],descriptor=s->words[s->sp-3];
  if(descriptor){uint32_t capacity=0,data=0;if(!local_range(n,base,stackbase,s,descriptor,12)||descriptor>UINT32_MAX-8||!access(b,n,base,stackbase,s,descriptor+4,&capacity,0))return 0;
   if(index<capacity){uint8_t byte=(uint8_t)value;if(!access(b,n,base,stackbase,s,descriptor+8,&data,0)||index>UINT32_MAX-data||!byte_access(b,n,base,stackbase,s,data+index,&byte,1))return 0;}
  }
  s->sp-=3;return 1;
 }
 /* Original65 copies the last immediate-count words forward onto the stack. */
 if(op==0x4c){if((!service&&!memory)||!s->sp)return 0;nflstreet_ui_service_request q={0};q.kind=10;q.argc=s->words[--s->sp];if(q.argc>16||s->sp<q.argc+2)return 0;memcpy(q.args,s->words+s->sp-q.argc,q.argc*4);s->sp-=q.argc;q.property=s->words[--s->sp];q.target=s->words[--s->sp];unsigned ignored=0;return invoke(service,memory,ctx,b,n,base,&q,&ignored,stackbase,s->words);}
 if(op==0x63||op==0x64){
  if(!range(n,base,s->pc,4))return 0;uint32_t count=be(b+s->pc-base);s->pc+=4;
  if(op==0x64){if(count>s->sp)return 0;s->sp-=count;return 1;}
  if(count>256-s->sp||(count&&!s->sp))return 0;
  uint32_t value=s->sp?s->words[s->sp-1]:0;for(unsigned i=0;i<count;++i)s->words[s->sp+i]=value;s->sp+=count;return 1;
 }
 if(op==0x65){if(!range(n,base,s->pc,4))return 0;unsigned count=be(b+s->pc-base);s->pc+=4;if(count>s->sp||count>256-s->sp)return 0;unsigned first=s->sp-count;for(unsigned i=0;i<count;++i)s->words[s->sp+i]=s->words[first+i];s->sp+=count;return 1;}
 if(op==0x52){if(s->sp<2)return 0;a=s->words[s->sp-1];s->words[s->sp-1]=s->words[s->sp-2];s->words[s->sp-2]=a;return 1;}
 if(!s->sp)return 0;a=s->words[s->sp-1];
 if(op==0x49){if(s->sp<2)return 0;unsigned len=0;if(a){if(!range(n,base,a,12))return 0;unsigned text=be(b+a-base+8);if(!range(n,base,text,1))return 0;const uint8_t*end=memchr(b+text-base,0,n-(text-base));if(!end)return 0;len=(unsigned)(end-(b+text-base));}s->words[s->sp-2]=len;--s->sp;return 1;}
 if(op==0x6f){if(n<16)return 0;if(a>=be(b+8)){s->words[s->sp-1]=0;return 1;}uint32_t table=be(b+12);if(a>UINT32_MAX/12||table>UINT32_MAX-a*12||!range(n,base,table+a*12,12))return 0;s->words[s->sp-1]=table+a*12;return 1;}
 if(op==0x12||op==0x14){unsigned index=op==0x14?s->sp-2:s->sp-1;if(op==0x14&&s->sp<2)return 0;double f=fl(s->words[index]);if(!isfinite(f)||f<INT32_MIN||f>=2147483648.0)return 0;s->words[index]=(uint32_t)(int32_t)f;return 1;}
 if(op==0x13||op==0x15){if(op==0x15&&s->sp<2)return 0;unsigned index=op==0x15?s->sp-2:s->sp-1;s->words[index]=word((float)(int32_t)s->words[index]);return 1;}
 if(op==0x23||op==0x29||op==0x30||op==0x31){float f=fl(a);if(!isfinite(f))return 0;s->words[s->sp-1]=word(op==0x23?fabsf(f):op==0x29?-f:op==0x30?f+1:f-1);return 1;}
 if(op==0x41)return push(s,a);if(op==0x42){--s->sp;return 1;}if(op==0x44){--s->sp;s->pc=a;return !a||range(n,base,a,1);}
 if(op==0x1a||op==0x1b||op==0x69){int64_t index=(int64_t)s->sp-1+(int32_t)a;if(index<0||index>=256)return 0;
  if(op==0x69){s->words[s->sp-1]=stackbase+(unsigned)index*4;return 1;}
  if(op==0x1a){s->words[s->sp-1]=s->words[index];return 1;}if(s->sp<2)return 0;s->words[index]=s->words[s->sp-2];s->sp-=2;return 1;
 }
 if(op==0x18)return access(b,n,base,stackbase,s,a,&s->words[s->sp-1],0);
 if(op==0x1e){s->words[s->sp-1]=~a;return 1;}if(op==0x21){s->words[s->sp-1]=!a;return 1;}
 if(op==0x22){s->words[s->sp-1]=(int32_t)a<0?0-a:a;return 1;}if(op==0x24){s->words[s->sp-1]=0-a;return 1;}
 if(op==0x2e||op==0x2f){s->words[s->sp-1]=op==0x2e?a+1:a-1;return 1;}
 if(op==0x40){int64_t dest=(int64_t)s->pc+(int32_t)a;if(dest<0||dest>UINT32_MAX||!range(n,base,(uint32_t)dest,1))return 0;--s->sp;s->pc=(uint32_t)dest;return 1;}
 if(s->sp<2)return 0;v=s->words[s->sp-2];
 if((op>=0x2a&&op<=0x2d)||(op>=0x38&&op<=0x3d)){float x=fl(v),y=fl(a),epsilon=fl(0x33d6bf95);if(!isfinite(x)||!isfinite(y))return 0;
  switch(op){case 0x2a:v=word(x+y);break;case 0x2b:v=word(x-y);break;case 0x2c:v=word(x*y);break;case 0x2d:if(y==0)return 0;v=word(x/y);break;
   case 0x38:v=y<=(float)((double)x+epsilon);break;case 0x39:v=(float)((double)x-epsilon)<=y;break;case 0x3a:v=y<(float)((double)x-epsilon);break;case 0x3b:v=(float)((double)x+epsilon)<y;break;case 0x3c:v=fabs((double)(y-x))<=epsilon;break;default:v=epsilon<fabs((double)(y-x));break;}
  s->words[s->sp-2]=v;--s->sp;return 1;
 }
 if(op==0x19){s->sp-=2;return access(b,n,base,stackbase,s,a,&v,1);}
 if(op==0x3e||op==0x3f){s->sp-=2;if(op==0x3e?v!=0:v==0){int64_t dest=(int64_t)s->pc+(int32_t)a;if(dest<0||dest>UINT32_MAX||!range(n,base,(uint32_t)dest,1))return 0;s->pc=(uint32_t)dest;}return 1;}
 switch(op){case 0x1c:v&=a;break;case 0x1d:v|=a;break;case 0x1f:v=!!v&&!!a;break;case 0x20:v=!!v||!!a;break;
 case 0x25:v+=a;break;case 0x26:v-=a;break;case 0x27:v*=a;break;
 case 0x28:if(!a||(v==0x80000000&&a==UINT32_MAX))return 0;v=(uint32_t)((int32_t)v/(int32_t)a);break;
 case 0x5a:if(!a||(v==0x80000000&&a==UINT32_MAX))return 0;v=(uint32_t)((int32_t)v%(int32_t)a);break;
 case 0x32:v=(int32_t)a<=(int32_t)v;break;case 0x33:v=(int32_t)v<=(int32_t)a;break;case 0x34:v=(int32_t)a<(int32_t)v;break;case 0x35:v=(int32_t)v<(int32_t)a;break;case 0x36:v=v==a;break;case 0x37:v=v!=a;break;default:return 0;
 }s->words[s->sp-2]=v;--s->sp;return 1;
}
int nflstreet_ui_script_services_memory(uint8_t*b,size_t n,uint32_t base,uint32_t stackbase,nflstreet_ui_script*s,unsigned budget,nflstreet_ui_script_service service,void*ctx,size_t ctxsize,nflstreet_ui_script_capture capture,nflstreet_ui_script_memory_service memory){
 if(!b||!s||s->suspension||!n||n>8*1024*1024||!base||n>UINT32_MAX-base||!stackbase||sizeof s->words>UINT32_MAX-stackbase||s->sp>256||!budget||budget>100000)return 0;
 if((uint64_t)base+n>stackbase&&(uint64_t)stackbase+sizeof s->words>base)return 0;
 if(((service||memory||capture)&&(!ctx||!ctxsize))||(ctxsize&&!ctx)||ctxsize>8*1024*1024)return 0;uint8_t*copy=malloc(n);void*servicecopy=ctxsize?malloc(ctxsize):NULL;if(!copy||(ctxsize&&!servicecopy)){free(copy);free(servicecopy);return 0;}memcpy(copy,b,n);if(ctxsize)memcpy(servicecopy,ctx,ctxsize);nflstreet_ui_script state=*s;int ok=0;
 for(unsigned i=0;i<budget;++i){if(!state.pc){ok=1;break;}if(!step(copy,n,base,stackbase,&state,service,servicecopy,capture,memory))break;if(state.suspension==3){ok=1;break;}if(!state.pc){ok=1;break;}}
 if(ok){memcpy(b,copy,n);*s=state;if(ctxsize)memcpy(ctx,servicecopy,ctxsize);}free(servicecopy);free(copy);return ok;
}
int nflstreet_ui_script_services_capture(uint8_t*b,size_t n,uint32_t base,uint32_t stackbase,nflstreet_ui_script*s,unsigned budget,nflstreet_ui_script_service service,void*ctx,size_t ctxsize,nflstreet_ui_script_capture capture){return nflstreet_ui_script_services_memory(b,n,base,stackbase,s,budget,service,ctx,ctxsize,capture,NULL);}
int nflstreet_ui_script_services(uint8_t*b,size_t n,uint32_t base,uint32_t stackbase,nflstreet_ui_script*s,unsigned budget,nflstreet_ui_script_service service,void*ctx,size_t ctxsize){return nflstreet_ui_script_services_capture(b,n,base,stackbase,s,budget,service,ctx,ctxsize,NULL);}
int nflstreet_ui_script_run(uint8_t*b,size_t n,uint32_t base,uint32_t stackbase,nflstreet_ui_script*s,unsigned budget){return nflstreet_ui_script_services(b,n,base,stackbase,s,budget,NULL,NULL,0);}
