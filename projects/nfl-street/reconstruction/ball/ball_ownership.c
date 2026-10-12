#include "ball_ownership.h"
#include "ball_lifecycle.h"
#include <string.h>
static void put(uint8_t*p,uint32_t v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
int nflstreet_ball_assign(uint8_t*r,size_t n,uint32_t header,uint32_t*flags,nflstreet_ball_assignment_context*context,uint32_t elapsed,const nflstreet_ball_attachment_inputs*in,nflstreet_ball_pose*out){
 if(!r||n<0xf4||(header&255)!=1||!flags||!context||!in||!out)return 0;
 uint8_t next[0xf4];memcpy(next,r,sizeof next);nflstreet_ball_assignment_context ctx=*context;nflstreet_ball_pose model;
 put(next+0xcc,0);put(next+0xc8,0);put(next+0xb4,header);put(next+0xb8,header);
 if(!nflstreet_ball_transition(next,sizeof next,1,0,elapsed))return 0;
 ctx.cached_value=ctx.value;if(ctx.value==3)ctx.actor_word=header;put(next+0xd0,0);
 if(!nflstreet_ball_update_attached(next,sizeof next,in->world,in->model_root,in->model_scale,in->bones,in->count,in->selector,in->local,in->rotation,0,&model))return 0;
 memset(next+0x54,0,12);memcpy(r,next,sizeof next);*flags&=~0x08000000u;*context=ctx;*out=model;return 1;
}
