#include "team_classes.h"
#include "construct.h"
#include "image_construct.h"
#include "text_construct.h"
#include "text.h"
#include "text_lines.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int acquire(nflstreet_team_classes*s,uint32_t identity){
 if(s->acquisition_count>=NFLSTREET_TEAM_CLASSES_ACQUIRE_CAPACITY)return 0;
 s->acquisitions[s->acquisition_count++]=identity;return 1;
}

static uint32_t be32(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static unsigned be16(const uint8_t*p){return (unsigned)p[0]<<8|p[1];}
static void put16(uint8_t*p,unsigned v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}

static int locate_text(uint8_t*b,size_t n,uint32_t base,uint32_t target,
                       uint8_t**object,uint32_t*descriptor_address,
                       const char**text,size_t*text_length){
 if(target<base||target-base>n||40>n-(target-base))return 0;
 uint8_t*o=b+target-base;uint32_t reference=be32(o+32);size_t descriptor_offset;
 if(reference>=base){descriptor_offset=(size_t)(reference-base);}
 else {if(reference>n-(target-base))return 0;descriptor_offset=(target-base)+reference;}
 if(descriptor_offset>n||12>n-descriptor_offset)return 0;
 uint32_t text_address=be32(b+descriptor_offset+8);size_t text_offset;
 if(text_address>=base)text_offset=(size_t)(text_address-base);
 else text_offset=text_address;
 if(text_offset>=n)return 0;
 const char*string=(const char*)(b+text_offset);const char*end=memchr(string,0,n-text_offset);
 if(!end)return 0;
 *object=o;*descriptor_address=base+(uint32_t)descriptor_offset;
 *text=string;*text_length=(size_t)(end-string);return 1;
}

static int prepare_lines(nflstreet_team_classes*s,uint8_t*b,size_t n,uint32_t base,
                         uint32_t target,unsigned font_index){
 if(s->line_plan_count>=NFLSTREET_TEAM_CLASSES_LINE_PLAN_CAPACITY)return 0;
 uint8_t*object;const char*text;size_t text_length;uint32_t descriptor;
 if(!locate_text(b,n,base,target,&object,&descriptor,&text,&text_length))return 0;
 (void)text_length;
 put16(object+32,(unsigned)(descriptor>>16));put16(object+34,(unsigned)descriptor);
 unsigned index=s->line_plan_count;uint32_t line_base=0x817d0000u+index*1024u;
 nflstreet_team_classes_line_plan*entry=&s->line_plans[index];memset(entry,0,sizeof *entry);
 entry->target=target;entry->line_base=line_base;
 if(!nflstreet_ui_text_prepare_lines(object,&s->fonts[font_index],text,line_base,&entry->plan))return 0;
 if(entry->plan.count>1)++s->line_plan_count;
 else memset(entry,0,sizeof *entry);
 return 1;
}

/* Team labels include LF without wrapping flags. The original line helper
   still allocates one range per newline; retain those ranges in owned state. */
static int prepare_newlines(nflstreet_team_classes*s,uint8_t*b,size_t n,uint32_t base,
                            uint32_t target,unsigned font_index){
 if(s->line_plan_count>=NFLSTREET_TEAM_CLASSES_LINE_PLAN_CAPACITY)return 0;
 uint8_t*object;const char*text;size_t length;uint32_t descriptor;
 if(!locate_text(b,n,base,target,&object,&descriptor,&text,&length))return 0;
 unsigned flags=be16(object);if((flags&0x100)||object[15]>1)return 0;
 if(length>354)return 0;
 unsigned index=s->line_plan_count;uint32_t line_base=0x817d0000u+index*1024u;
 nflstreet_team_classes_line_plan*entry=&s->line_plans[index];memset(entry,0,sizeof *entry);
 entry->target=target;entry->line_base=line_base;
 unsigned first=0;
 for(unsigned i=0;i<=length;++i)if(!text[i]||text[i]=='\n'){
  if(entry->plan.count>=255||first>i)return 0;
  entry->plan.lines[entry->plan.count++]=(nflstreet_ui_text_line){(uint16_t)first,(uint16_t)i};
  first=i+1;
 }
 if(entry->plan.count<=1)return 0;
 int32_t width;if(!nflstreet_font_measure(&s->fonts[font_index],text,flags,&width))return 0;
 uint8_t copy[40];memcpy(copy,object,sizeof copy);
 put16(copy+32,(unsigned)(descriptor>>16));put16(copy+34,(unsigned)descriptor);
 copy[15]=(uint8_t)entry->plan.count;put16(copy+20,(unsigned)width);
 put16(copy+36,(unsigned)(line_base>>16));put16(copy+38,(unsigned)line_base);
 if(!(flags&0x200))put16(copy+22,entry->plan.count*s->fonts[font_index].line_height);
 put16(copy,flags&0x7fff);memcpy(object,copy,sizeof copy);
 ++s->line_plan_count;return 1;
}

static int service(void*ctx,uint8_t*b,size_t n,uint32_t base,
                   const nflstreet_ui_service_request*q,uint32_t*result){
 (void)result;
 nflstreet_team_classes*s=ctx;
 if(!s||!q||s->request_count>=NFLSTREET_TEAM_CLASSES_REQUEST_CAPACITY||
    (q->class_id!=0&&q->class_id!=8)||q->property!=UINT32_MAX)return 0;
 s->requests[s->request_count++]=*q;++s->class_requests[q->class_id];
 if(q->class_id==0){
  unsigned requested=0;uint32_t identity=0;
  if(!nflstreet_ui_image_construct(b,n,base,q->target,&requested,&identity))return 0;
  if(requested){++s->image_requests;return acquire(s,identity);}
  return 1;
 }
 if(q->target<base||q->target-base>n||40>n-(q->target-base))return 0;
 unsigned font_index=b[q->target-base+2];
 if(font_index>=NFLSTREET_TEAM_CLASSES_FONT_COUNT)return 0;
 if(!s->fonts[font_index].bytes||!s->fonts[font_index].size)return 0;
 /* Original 801D9788 acquires the font before 801D8E54 looks it up. */
 unsigned archive=b[q->target-base+3];
 if(!acquire(s,(archive<<16)|font_index))return 0;
 unsigned flags=be16(b+q->target-base);
 if(flags&0x100){
  if(!prepare_lines(s,b,n,base,q->target,font_index))return 0;
 }else {
  uint8_t*object;const char*text;size_t length;uint32_t descriptor;
  if(!locate_text(b,n,base,q->target,&object,&descriptor,&text,&length))return 0;
  (void)object;(void)descriptor;
  if(memchr(text,'\n',length)){
   if(!prepare_newlines(s,b,n,base,q->target,font_index))return 0;
  }else if(!nflstreet_ui_text_construct(b,n,base,q->target,&s->fonts[font_index]))return 0;
 }
 ++s->text_requests;
 return acquire(s,0x20000u|font_index);
}

int nflstreet_team_classes_construct(uint8_t*b,size_t n,uint32_t base,
                                     nflstreet_team_classes*s){
 if(!b||!s)return 0;
 return nflstreet_ui_construct(b,n,base,0,UINT32_MAX,0x0101u,service,s,sizeof *s);
}

int nflstreet_team_classes_prepare_text(nflstreet_team_classes*s,uint8_t*b,size_t n,uint32_t base,uint32_t target){
 if(!s||!b||target<base||target-base>n||n-(target-base)<40||s->line_plan_count>=NFLSTREET_TEAM_CLASSES_LINE_PLAN_CAPACITY)return 0;
 uint8_t*o=b+target-base;unsigned font=o[2];if(font>=4||!s->fonts[font].bytes)return 0;
 uint8_t saved[40];memcpy(saved,o,40);unsigned initial=s->line_plan_count,existing=initial;
 for(unsigned i=0;i<initial;++i)if(s->line_plans[i].target==target){existing=i;break;}
 if(o[15]>1&&existing==initial)return 0;
 o[15]=0;memset(o+36,0,4);
 uint8_t*object;const char*text;size_t length;uint32_t descriptor;
 int ok=locate_text(b,n,base,target,&object,&descriptor,&text,&length);
 if(ok){if(be16(o)&0x100)ok=prepare_lines(s,b,n,base,target,font);
  else if(memchr(text,'\n',length))ok=prepare_newlines(s,b,n,base,target,font);
  else ok=nflstreet_ui_text_prepare(o,&s->fonts[font],text);
 }
 if(!ok){memcpy(o,saved,40);s->line_plan_count=initial;return 0;}
 if(existing<initial){
  if(s->line_plan_count>initial){s->line_plans[existing]=s->line_plans[initial];s->line_plans[existing].line_base=0x817d0000u+existing*1024u;uint32_t a=s->line_plans[existing].line_base;put16(o+36,a>>16);put16(o+38,a);--s->line_plan_count;}
  else{memset(&s->line_plans[existing].plan,0,sizeof s->line_plans[existing].plan);}
 }
 return 1;
}
