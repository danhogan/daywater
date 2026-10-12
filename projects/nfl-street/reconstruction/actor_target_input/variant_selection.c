#include "variant_selection.h"
#include <string.h>
#include <math.h>
static uint8_t mapped(uint8_t v){return v==2?12:v==1?14:v==3?11:v==4?10:13;}
int nflstreet_variant_first(const nflstreet_variant_facts *f,uint8_t initial,nflstreet_variant_choice *out){
 if(!f||!out)return 0;nflstreet_variant_choice next={0,initial};
 if(f->kind_byte!=1)next.key=2;
 else if(f->status_byte>1&&f->status_byte!=2&&f->status_byte!=3&&f->status_byte!=6&&f->status_byte!=7&&f->status_byte!=31&&f->status_byte!=22&&f->status_byte!=23)next.key=1;
 else if(f->side_byte!=f->global_selector)next.key=f->selector_word==4?8:mapped(f->variant_byte);
 else {
  if(!(f->available&NFLSTREET_VARIANT_PREDICATE)||f->predicate>1)return 0;
  next.flag=(uint8_t)f->predicate;
  if(f->predicate){next.key=7;next.flag=1;}
  else if(f->callback_tag==18)next.key=3;
  else {if(!(f->available&NFLSTREET_VARIANT_SPECIAL_PREDICATE)||f->special_predicate>1)return 0;
   next.key=f->special_predicate?3:mapped(f->variant_byte);}
  if(f->status_byte==22||f->status_byte==23){
   if(!(f->available&NFLSTREET_VARIANT_SPECIAL_WORD))return 0;
   if(f->special_word==3||f->special_word==4){
    if(!(f->available&NFLSTREET_VARIANT_SPECIAL_ENABLED))return 0;
    if(!f->special_enabled)next.key=1;
   }
  }
 }
 *out=next;return 1;
}
int nflstreet_variant_resolve(const nflstreet_asset_store *store,uint16_t context,
 const nflstreet_variant_facts*f,uint8_t second,uint8_t initial,uint8_t data[64],nflstreet_variant_choice*out){
 if(!data||!out)return 0;nflstreet_variant_choice choice;if(!nflstreet_variant_first(f,initial,&choice))return 0;
 uint8_t next[64],keys[2]={choice.key,second};memcpy(next,data,64);
 if(!nflstreet_locomotion_resolve(store,context,keys,next))return 0;
 next[0x3c]=choice.key;next[0x3d]=second;memcpy(data,next,64);*out=choice;return 1;
}

int nflstreet_variant_request(const nflstreet_request_facts*f,unsigned*out){
 if(!f||!out||!isfinite(f->speed))return 0;unsigned result=0;
 if(f->request_state){
  if(f->selector_word==4||f->selector_word==8)result=1;
  else if((f->selector_word==1||f->selector_word==2||f->selector_word==3||f->selector_word==7)&&!f->service_argument){
   if(f->request_state==1)result=1;
   else {if(!f->target_available)return 0;
    if(f->target_resolved){if(!isfinite(f->target_distance)||f->target_distance<0)return 0;
     result=f->target_separation<=0x1fffff&&f->target_distance<4.5f;}}
  }
 }
 if(f->request_state==3&&!result&&f->speed<1.0000000116860974e-7f)result=1;
 *out=result;return 1;
}
static uint32_t separation(uint32_t a,uint32_t b){uint32_t d=(a-b)&0xffffff;return d<=0x1000000-d?d:0x1000000-d;}
static int fact(unsigned available,unsigned mask,unsigned value){return (available&mask)&&value<=1;}
int nflstreet_variant_second(const nflstreet_variant_facts*f,const nflstreet_variant_second_facts*s,uint8_t*out){
 if(!f||!s||!out||s->same_actor>1)return 0;unsigned key=2,status=f->status_byte;
 if(f->kind_byte==1){
  if(s->same_actor){
   switch(status){case 2:case 3:key=10;break;case 4:key=42;break;case 5:key=40;break;
    case 8:case 9:case 10:case 11:key=12;break;case 12:case 13:key=34;break;
    case 15:key=17;break;case 16:key=16;break;case 24:case 25:key=33;break;
    case 28:key=37;break;case 29:key=41;break;case 31:key=1;break;
    case 22:case 23:
     if(!(f->available&NFLSTREET_VARIANT_SPECIAL_WORD))return 0;
     if(f->special_word==3||f->special_word==4){if(!(s->available&NFLSTREET_SECOND_INDEX)||!s->special_table||s->table_size<256)return 0;key=s->special_table[s->special_index&255];}
     else key=1;break;
    default:
     if(!fact(s->available,NFLSTREET_SECOND_PHASE,s->phase_predicate))return 0;key=s->phase_predicate?19:1;
     /* Original 80062C28 is constant zero. */
     if(s->aux_byte==0||s->aux_byte==20||s->aux_byte==1||s->aux_byte==3){
      if(!fact(f->available,NFLSTREET_VARIANT_SPECIAL_PREDICATE,f->special_predicate))return 0;
      if(!f->special_predicate){if(!fact(s->available,NFLSTREET_SECOND_ADDITIONAL,s->additional_predicate))return 0;
       if(s->additional_predicate)key=s->side_flag?22:23;
      }else {if(!(s->available&NFLSTREET_SECOND_VECTOR)||!isfinite(s->speed))return 0;
       unsigned degrees=s->side_flag?(s->old_key==9?85:95):(s->old_key==9?95:85);
       unsigned limit=(unsigned)((float)degrees*46603.37890625f),angle=separation(s->vector_heading,0);
       if(!s->side_flag)key=angle<limit?9:6;
       else {key=angle<limit?6:9;if(s->speed<=1.0000000116860974e-7f)key=9;}
      }
     }
   }
  }else {
   unsigned active=s->request_state!=0&&s->first_flag==1;
   if(s->request_state&&s->first_flag==2){if(!fact(s->available,NFLSTREET_SECOND_REQUEST,s->request_predicate))return 0;active=s->request_predicate;}
   if(active){unsigned selector=f->selector_word;key=selector==2||selector==3||selector==7?4:selector==4||selector==8?5:3;}
   else {switch(status){case 20:key=26;break;case 17:key=24;break;case 14:key=14;break;case 18:key=25;break;case 21:key=27;break;case 26:case 27:key=36;break;
    default:if(!fact(s->available,NFLSTREET_SECOND_PHASE,s->phase_predicate))return 0;key=s->phase_predicate?20:2;}
    if(s->control_byte==9)key=2;
    if(f->callback_tag==42&&separation(s->heading,0xc00000)<0x2aaaaa){
     if(!(s->available&NFLSTREET_SECOND_FOCUS)||!isfinite(s->x)||!isfinite(s->y)||!isfinite(s->focus_x)||!isfinite(s->focus_y))return 0;
     if(s->y-s->focus_y<6.0f){key=15;if(fabsf(s->x-s->focus_x)<9.0f)key=13;}
    }
   }
  }
 }
 if(!fact(s->available,NFLSTREET_SECOND_ENDING,s->ending_predicate))return 0;
 if(s->ending_predicate){if(key==1)key=43;else if(key==2)key=18;}
 *out=(uint8_t)key;return 1;
}
int nflstreet_variant_resolve_pair(const nflstreet_asset_store*store,uint16_t context,
 const nflstreet_variant_facts*f,const nflstreet_variant_second_facts*s,uint8_t initial,
 uint8_t data[64],nflstreet_variant_choice*out,uint8_t*second){
 if(!s||!data||!out||!second)return 0;nflstreet_variant_choice choice;if(!nflstreet_variant_first(f,initial,&choice))return 0;
 nflstreet_variant_second_facts facts=*s;facts.first_flag=choice.flag;uint8_t key,next[64];
 if(!nflstreet_variant_second(f,&facts,&key))return 0;memcpy(next,data,64);
 if(!nflstreet_variant_resolve(store,context,f,key,initial,next,&choice))return 0;
 memcpy(data,next,64);*out=choice;*second=key;return 1;
}

int nflstreet_variant_phase_simple(const nflstreet_phase_facts*f,unsigned*out){
 if(!f||!out||!isfinite(f->comparison)||!isfinite(f->speed)||f->same_actor>1)return 0;
 if(f->comparison<0&&f->speed!=0){*out=0;return 1;}
 switch(f->callback_tag){case 2:case 21:case 22:case 23:case 27:case 30:case 31:case 33:case 40:case 47:case 51:case 72:case 85:case 92:return 0;}
 if(!f->same_actor||f->service_argument){*out=0;return 1;}
 if((int32_t)f->heading<0x800000){if(!f->global_flags_available)return 0;
  if(!(f->global_flags&1)){*out=1;return 1;}}
 return 0;
}
int nflstreet_variant_additional_simple(uint8_t tag,unsigned*out){
 if(!out||(tag!=1&&tag!=91))return 0;*out=0;return 1;
}
int nflstreet_variant_index(uint8_t tag,unsigned enabled,uint8_t raw,uint8_t*out){
 if(!out)return 0;*out=!enabled&&(tag==1||tag==91)?raw:32;return 1;
}
