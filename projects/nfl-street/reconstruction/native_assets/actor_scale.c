#include "actor_scale.h"
/* 800DF1EC has no callees: unsigned actor byte, pointer equality, then +7.
   Do not clamp to seven: lbz accepts the entire byte, with no range test. */
static int slot(nflstreet_draw *d,uint32_t r13,uint32_t actor,uint32_t *global,uint32_t *index){
 const uint8_t *p=nflstreet_draw_at(d,actor+0x1361u,1);uint32_t owner;
 if(!p||!nflstreet_draw_word(d,r13-0x7234u,global))return 0;
 *index=*p;
 if(!nflstreet_draw_word(d,*global+0x464u+4u**index,&owner))return 0;
 if(owner!=actor)*index+=7u;
 return 1;}
int nflstreet_actor_scale_mark(nflstreet_draw *d,uint32_t r13,uint32_t actor){
 const uint8_t *enabled=nflstreet_draw_at(d,r13-0x7238u,1);uint32_t global,index;
 if(!enabled)return 0;
 if(!*enabled)return 1;
 if(!slot(d,r13,actor,&global,&index))return 0;
 uint8_t *p=nflstreet_draw_at(d,global+0x4e3u+index,1);if(!p)return 0;
 *p=1;
 return 1;}
int nflstreet_actor_scale_vector(nflstreet_draw *d,uint32_t r13,uint32_t actor,float out[3]){
 const uint8_t *enabled=nflstreet_draw_at(d,r13-0x7238u,1);uint32_t global,index,word;
 if(!enabled||!out)return 0;
 uint32_t address=0x80252d24u;
 if(*enabled){if(!slot(d,r13,actor,&global,&index))return 0;address=global+0x49cu+4u*index;}
 if(!nflstreet_draw_word(d,address,&word))return 0;
 memcpy(out+2,&word,4);
 memcpy(out,&word,4);
 memcpy(out+1,&word,4);
 return 1;}
