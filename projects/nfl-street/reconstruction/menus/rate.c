#include "rate.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
static uint32_t be(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void put(uint8_t*p,uint32_t v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static float fl(uint32_t w){float f;memcpy(&f,&w,4);return f;}
static uint32_t word(float f){uint32_t w;memcpy(&w,&f,4);return w;}
unsigned nflstreet_ui_property_offset(uint32_t p){
 static const unsigned low[]={0x20,0x24,0x28,8,12,16,0x2c,0x30,0x34,0x14,0x18,0x1c};
 static const unsigned high[]={0x58,0x4c,0x50,0x54,0x48,0x3c,0x40,0x44};
 if(p>=0x10&&p<=0x1b)return low[p-0x10];if(p>=0x20&&p<=0x27)return high[p-0x20];return 0x48;
}
static uint8_t*property(uint8_t*b,size_t n,uint32_t base,uint32_t obj,uint32_t p){
 unsigned off=nflstreet_ui_property_offset(p);if(obj<base||obj-base>n||off>n-(obj-base)||4>n-(obj-base)-off)return NULL;return b+(obj-base)+off;
}
static void compact(nflstreet_ui_rates*s){
 for(unsigned i=s->count;i-- >0;){uint32_t*r=s->records[i].words;if(r[5]==1){--s->count;memmove(s->records+i,s->records+i+1,(s->count-i)*sizeof *s->records);}else if(r[5]==0)r[5]=2;}
}
int nflstreet_ui_rate_add(nflstreet_ui_rates*s,const uint8_t*b,size_t n,uint32_t base,uint32_t screen,uint32_t root,uint32_t id,uint32_t obj,uint32_t p,float target,uint32_t duration,uint32_t completion,uint32_t tick){
 if(!s||!b||s->count>64||!s->clock||!duration||!isfinite(target))return 0;
 uint8_t*at=property((uint8_t*)b,n,base,obj,p);if(!at)return 0;float current=fl(be(at));if(!isfinite(current))return 0;
 unsigned i;for(i=0;i<s->count;++i)if(s->records[i].words[0]==id&&s->records[i].words[6]==root)break;if(i==64)return 0;
 float delta=(float)((double)target-(double)current);float divisor=(float)duration/(float)s->clock;float rate=delta/divisor;if(!isfinite(rate))return 0;
 uint32_t words[13]={id,tick,0,0,s->clock,0,root,screen,p,word(target),word(rate),completion,obj};
 memcpy(s->records[i].words,words,sizeof words);if(i==s->count)++s->count;return 1;
}
static int update(nflstreet_ui_rates*s,uint8_t*b,size_t n,uint32_t base,uint32_t screen,uint32_t elapsed,uint32_t fallback,nflstreet_ui_rate_script call,void*ctx){
 compact(s);unsigned count=s->count;s->flags|=4;unsigned budget=100000;
 for(unsigned i=0;i<count;++i){uint32_t*r=s->records[i].words;if(r[5]!=2||!r[4])continue;if(r[7]!=screen)return 0;
  /* Original interval comparison is signed; elapsed accumulators wrap. */
  uint32_t remaining=r[2]+elapsed-r[3];r[2]+=elapsed;if((int32_t)r[4]>(int32_t)remaining)continue;r[3]+=r[4];remaining-=r[4];
  do{if(!budget--)return 0;float multiplier=1;
   if(r[1]){uint32_t args[3]={r[0],r[3],r[4]};if(!call||!call(ctx,s,b,n,base,r[7],r[6],r[1],r[8]?2:3,args,&multiplier))return 0;}
   do{if(!budget--)return 0;
    if(r[8]){uint8_t*at=property(b,n,base,r[12],r[8]);if(!at)return 0;double factor=multiplier<0?1:(double)multiplier;
     double rate=(double)fl(r[10]);float target=fl(r[9]);float value=(float)(rate*factor+(double)fl(be(at)));
     if((rate>0&&value<target)||(rate<0&&target<value))put(at,word(value));
     else{put(at,r[9]);r[5]=1;uint32_t completion=r[11]?r[11]:fallback;if(completion){uint32_t arg=r[0];float ignored=0;if(!call||!call(ctx,s,b,n,base,r[7],r[6],completion,1,&arg,&ignored))return 0;}}
    }
    if((int32_t)remaining<(int32_t)r[4])goto next;remaining-=r[4];r[3]+=r[4];multiplier=1;
   }while(!r[1]);
  }while(r[5]==2);
 next:;
 }
 s->flags&=~4u;compact(s);return 1;
}
int nflstreet_ui_rate_update(nflstreet_ui_rates*s,uint8_t*b,size_t n,uint32_t base,uint32_t screen,uint32_t elapsed,uint32_t fallback,nflstreet_ui_rate_script call,void*ctx){
 if(!s||!b||s->count>64||!n||n>8*1024*1024||!base||n>UINT32_MAX-base)return 0;
 uint8_t*copy=malloc(n);if(!copy)return 0;memcpy(copy,b,n);nflstreet_ui_rates state=*s;
 int ok=update(&state,copy,n,base,screen,elapsed,fallback,call,ctx);if(ok){memcpy(b,copy,n);*s=state;}free(copy);return ok;
}
int nflstreet_ui_rate_cancel(nflstreet_ui_rates*s,uint32_t screen){if(!s||s->count>64)return 0;s->flags|=4;for(unsigned i=0;i<s->count;++i)if(s->records[i].words[7]==screen)s->records[i].words[5]=1;s->flags&=~4u;compact(s);return 1;}
int nflstreet_ui_rate_cancel_identity(nflstreet_ui_rates*s,uint32_t root,uint32_t identity){
 if(!s||s->count>64)return 0;for(unsigned i=0;i<s->count;++i)if(s->records[i].words[0]==identity&&s->records[i].words[6]==root){s->records[i].words[5]=1;break;}return 1;
}
int nflstreet_ui_rate_request(nflstreet_ui_rates*s,uint8_t*b,size_t n,uint32_t base,uint32_t screen,const nflstreet_ui_service_request*q){
 if(!q||!s||s->count>64)return 0;
 if(q->kind==5)return q->argc==2&&nflstreet_ui_rate_cancel_identity(s,q->args[0],q->args[1]);
 if(q->kind==4){if(q->argc!=4||!q->args[1]||q->args[2]>=n||q->args[2]>UINT32_MAX-base)return 0;
  unsigned i;for(i=0;i<s->count;++i)if(s->records[i].words[0]==q->args[3]&&s->records[i].words[6]==q->args[0])break;if(i==64)return 0;
  /*801D1490 intentionally preserves words10/12 when replacing a rate. */
  uint32_t*r=s->records[i].words;r[0]=q->args[3];r[1]=base+q->args[2];r[2]=r[3]=0;r[4]=q->args[1];r[5]=0;r[6]=q->args[0];r[7]=screen;r[8]=0;r[9]=0;r[11]=0;if(i==s->count)++s->count;return 1;
 }
 if(q->kind!=3||q->class_id>6)return 0;unsigned mode=q->class_id,count=mode>5?8:mode>4?7:6;if(q->argc!=count)return 0;
 unsigned extra=count-6;uint32_t completion=mode>4?q->args[1+extra]:0,tick=mode>5?q->args[2]:0;
 if((completion&&(completion>=n||completion>UINT32_MAX-base))||(tick&&(tick>=n||tick>UINT32_MAX-base)))return 0;
 return nflstreet_ui_rate_add(s,b,n,base,screen,q->args[0],q->args[count-1],q->args[1],q->args[count-2],fl(q->args[count-3]),q->args[count-4],completion?base+completion:0,tick?base+tick:0);
}
