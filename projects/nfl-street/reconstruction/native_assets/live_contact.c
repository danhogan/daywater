#include "live_contact.h"
#include "pose.h"
#include "../actor_target_input/animation_callbacks.h"
#include "../actor_target_input/contact_selection.h"
#include "../actor_target_input/pair_requests.h"
#include "../actor_target_input/command_timing.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
static int readn(FILE *f,void *p,size_t n){return fread(p,1,n,f)==n;}
static int word(FILE *f,uint32_t *v){uint8_t b[4];if(!readn(f,b,4))return 0;
    *v=(uint32_t)b[0]<<24|(uint32_t)b[1]<<16|(uint32_t)b[2]<<8|b[3];return 1;}
static int bytes(FILE *f,const uint8_t **data,size_t *size,size_t limit){uint32_t n;
    if(!word(f,&n)||n>limit)return 0;
    uint8_t *p=malloc(n?n:1);if(!p)return 0;
    if(!readn(f,p,n)){free(p);return 0;}*data=p;*size=n;return 1;}
static int blobs(FILE *f,const nflstreet_animation_blob **out,size_t *size,size_t limit,size_t max_size){uint32_t count;
    if(!word(f,&count)||count>limit)return 0;
    nflstreet_animation_blob *b=calloc(count?count:1,sizeof *b);if(!b)return 0;
    *out=b;*size=count;
    for(unsigned i=0;i<count;++i)if(!word(f,&b[i].identity)||!bytes(f,&b[i].data,&b[i].size,max_size))return 0;
    return 1;}
static int sources(FILE *f,nflstreet_animation_sources *s){uint32_t value;size_t n;
    memset(s,0,sizeof *s);
    if(!word(f,&value)||value>255)return 0;
    s->bank_key=(uint16_t)value;
    if(!bytes(f,&s->bank,&n,32)||n!=32||!bytes(f,&s->groups,&s->groups_size,8196)||
       !bytes(f,&s->program_table,&s->program_table_size,16388)||
       !word(f,&s->program_identity)||!bytes(f,&s->program,&s->program_size,4096)||
       !blobs(f,&s->metadata_assets,&s->metadata_count,64,2548)||
       !bytes(f,&s->profile,&n,16)||n!=16||!bytes(f,&s->speed_table,&s->speed_table_size,64)||
       !word(f,&value)||value>256)return 0;
    s->side_source=(int)value-1;
    if(!bytes(f,&s->transition_table,&s->transition_table_size,520)||
       !blobs(f,&s->transition_assets,&s->transition_count,64,4100)||
       !blobs(f,&s->transition_targets,&s->target_count,16384,16)||!word(f,&value)||value>4096)return 0;
    nflstreet_animation_registry_entry *r=calloc(value?value:1,sizeof *r);if(!r)return 0;
    s->registry=r;s->registry_count=value;
    for(unsigned i=0;i<value;++i){
        if(!bytes(f,&r[i].registry,&r[i].registry_size,28)||r[i].registry_size!=28||
           !bytes(f,&r[i].bank,&n,32)||(n!=0&&n!=32)||!bytes(f,&r[i].groups,&r[i].groups_size,8196))return 0;
    }
    return 1;
}
static void free_blobs(const nflstreet_animation_blob *b,size_t n){for(size_t i=0;i<n;++i)free((void *)b[i].data);free((void *)b);}
static void release(nflstreet_animation_sources *s){free((void *)s->bank);free((void *)s->groups);free((void *)s->program);free((void *)s->program_table);
    free((void *)s->profile);free((void *)s->speed_table);free((void *)s->transition_table);
    free_blobs(s->metadata_assets,s->metadata_count);free_blobs(s->transition_assets,s->transition_count);free_blobs(s->transition_targets,s->target_count);
    for(size_t i=0;i<s->registry_count;++i){free((void *)s->registry[i].registry);free((void *)s->registry[i].bank);free((void *)s->registry[i].groups);}free((void *)s->registry);}

static unsigned be(const uint8_t*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static unsigned half(const uint8_t*p){return (unsigned)p[0]<<8|p[1];}
int nflstreet_live_contact_open(nflstreet_live_contact*out,const char*selection,const char*paired,const uint8_t*bank,size_t bank_size){
 if(!out||!selection||!paired||!bank||bank_size<24)return 0;
 nflstreet_live_contact next={0};uint8_t params[60],actors[2][0x14d4],motion[44],rng[24],generator[256],manager[256],descriptors[2][6],requests[2][16];
 uint32_t scale_bits=0,count=0;float scale;FILE*f=fopen(selection,"rb");if(!f)return 0;
 int ok=readn(f,params,60)&&readn(f,actors,sizeof actors)&&readn(f,motion,44)&&word(f,&scale_bits)&&readn(f,rng,24)&&readn(f,generator,256)&&readn(f,manager,256)&&readn(f,descriptors,12)&&word(f,&count)&&count<=64;
 nflstreet_contact_candidate *candidates=calloc(ok&&count?count:1,sizeof *candidates);if(!candidates)ok=0;
 for(unsigned i=0;ok&&i<count;++i){uint32_t choice,key;ok=word(f,&candidates[i].identity)&&word(f,&choice)&&choice<=65535&&word(f,&key)&&key<=65535&&bytes(f,&candidates[i].asset,&candidates[i].size,5128);if(ok){candidates[i].choice=choice;candidates[i].key=key;}}
 if(ok)ok=fgetc(f)==EOF;fclose(f);memcpy(&scale,&scale_bits,4);
 unsigned eligible,accepted;uint32_t identity;uint16_t choice;
 if(ok)ok=nflstreet_contact_parameters(be(params),be(params+4),actors[0],actors[1],descriptors[0],descriptors[1],manager,params,&eligible)&&eligible&&
  nflstreet_contact_select(params,actors[0],actors[1],motion,scale,candidates,count,rng,generator,&identity,&choice,&accepted)&&identity&&accepted;
 if(ok){unsigned i=0;while(i<count&&candidates[i].identity!=identity)++i;ok=i<count&&nflstreet_pair_requests(candidates[i].asset,candidates[i].size,params,choice,requests[0],requests[1]);}
 if(candidates){for(unsigned i=0;i<(ok?count:count<=64?count:0);++i)free((void*)candidates[i].asset);free(candidates);}if(!ok)return 0;
 nflstreet_animation_sources pair[2]={{0},{0}};uint8_t ignored_requests[32],desc[2];f=fopen(paired,"rb");if(!f)return 0;
 ok=fgetc(f)=='P'&&readn(f,next.actor,sizeof next.actor)&&readn(f,next.manager,256)&&readn(f,next.globals,8)&&readn(f,ignored_requests,32)&&readn(f,desc,2)&&sources(f,pair)&&sources(f,pair+1)&&fgetc(f)==EOF;
 fclose(f);if(ok)ok=!memcmp(next.actor,actors,sizeof actors)&&!memcmp(next.manager,manager,256)&&desc[0]==descriptors[0][0]&&desc[1]==descriptors[1][0]&&
  nflstreet_contact_animation_complete(next.actor[0],next.actor[1],next.manager,requests,desc,pair,next.globals);
 for(unsigned i=0;ok&&i<2;++i){unsigned clip=half(requests[i]+10),table=be(bank+12);
  if(table>bank_size-4||clip+1>=be(bank+table)||table+8+(size_t)clip*4>bank_size){ok=0;break;}
  unsigned at=be(bank+table+4+clip*4),end=be(bank+table+8+clip*4);
  if(end<=at||end>bank_size||end-at>4096||pair[i].program_size<end-at||memcmp(pair[i].program,bank+at,end-at)){ok=0;break;}
  next.clips[i]=clip;next.program_size[i]=end-at;memcpy(next.program[i],bank+at,end-at);next.flags[i]=be(next.actor[i]+12);
  unsigned slot=10;for(unsigned j=0;j<10;++j){const uint8_t*c=next.actor[i]+0x4e4+j*124;if(c[1]==2&&half(c+2)==clip&&half(c+8)==30){slot=j;break;}}
  if(slot==10){ok=0;break;}next.slots[i]=slot;memcpy(next.stream[i],next.actor[i]+0x4e4+slot*124+0x4c,64);
 }
 release(pair);release(pair+1);if(!ok)return 0;next.active=1;*out=next;return 1;
}
int nflstreet_live_contact_step(nflstreet_live_contact*m,double delta,int16_t pose[2][384],unsigned*count){
 if(!m||!pose||!count||!m->active||!isfinite(delta)||delta<0)return 0;
 nflstreet_live_contact next=*m;unsigned n=0;int16_t result[2][384]={{0},{0}};
 for(unsigned i=0;i<2;++i){if(!next.done[i]){uint16_t commands[64];size_t emitted;unsigned done;
  if(!nflstreet_command_timing(next.stream[i],next.program[i],next.program_size[i],delta,&next.flags[i],commands,&emitted,&done))return 0;
  next.callbacks+=(unsigned)emitted;next.done[i]=done;
  for(unsigned j=0;j<4;++j)next.actor[i][12+j]=(uint8_t)(next.flags[i]>>(24-j*8));
  memcpy(next.actor[i]+0x4e4+next.slots[i]*124+0x4c,next.stream[i],64);
 }
 /* Rendering decodes frames through the existing common-bank API in the viewer.
    Return previous/next frame indices and 12-bit fraction in a compact output. */
 float fraction;uint32_t bits=be(next.stream[i]+20);memcpy(&fraction,&bits,4);if(!isfinite(fraction)||fraction<0||fraction>1)return 0;
 result[i][0]=(int16_t)half(next.stream[i]+16);result[i][1]=(int16_t)half(next.stream[i]+18);result[i][2]=(int16_t)(fraction*4096);n=3;
 }
 ++next.updates;if(next.done[0]&&next.done[1]){next.active=0;++next.completions;}
 memcpy(pose,result,sizeof result);*count=n;*m=next;return 1;
}
