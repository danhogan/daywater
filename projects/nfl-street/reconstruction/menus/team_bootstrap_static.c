#include "team_bootstrap_static.h"
#include <stdlib.h>
#include <string.h>
#define RAM_SIZE (24u*1024u*1024u)
#define BASE 0x80000000u
static uint32_t be(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static const uint8_t*at(const uint8_t*p,uint32_t a,size_t n){
 if(a<BASE||(uint64_t)a-BASE+n>RAM_SIZE)return NULL;
 return p+a-BASE;
}
static void pose(const uint8_t*p,nflstreet_team_activation_pose*out){
 for(unsigned j=0;j<3;++j)out->position[j]=be(p+j*4);
 out->rotation=be(p+12);out->visibility=be(p+16);
}
int nflstreet_team_bootstrap_static_from_dol(const uint8_t*d,size_t n,
 nflstreet_team_bootstrap_static*out){
 if(!d||!out||n<0xe4||n>16u*1024u*1024u)return 0;
 nflstreet_team_bootstrap_static s={0};s.mapped_dol=calloc(1,RAM_SIZE);
 if(!s.mapped_dol)return 0;s.mapped_size=RAM_SIZE;
 unsigned loaded=0;
 for(unsigned i=0;i<18;++i){uint32_t off=be(d+i*4),a=be(d+0x48+i*4),z=be(d+0x90+i*4);
  if(!z)continue;
  if(off>n||z>n-off||a<BASE||(uint64_t)a-BASE+z>RAM_SIZE)goto fail;
  memcpy(s.mapped_dol+a-BASE,d+off,z);++loaded;
 }
 if(!loaded)goto fail;
 for(unsigned side=0;side<2;++side){
  const uint8_t*p=at(s.mapped_dol,0x803a1e94u+side*8,7);if(!p)goto fail;
  unsigned mask=0;for(unsigned j=0;j<7;++j){if(p[j]>=7||(mask&(1u<<p[j])))goto fail;mask|=1u<<p[j];s.formation_order[side][j]=p[j];}
  p=at(s.mapped_dol,0x8028271cu+side*140,140);if(!p)goto fail;
  for(unsigned j=0;j<7;++j)pose(p+j*20,&s.poses[side][j]);
  p=at(s.mapped_dol,0x802826f4u+side*20,20);if(!p)goto fail;pose(p,&s.reset_poses[side]);
 }
 const uint8_t*p=at(s.mapped_dol,0x802859f8u,48);if(!p)goto fail;
 for(unsigned j=0;j<12;++j)s.gear_categories[j]=be(p+j*4);
 p=at(s.mapped_dol,0x80280d58u,56);if(!p)goto fail;
 for(unsigned j=0;j<14;++j){uint32_t a=be(p+j*4);const uint8_t*text=at(s.mapped_dol,a,64);if(!text)goto fail;
  const uint8_t*end=memchr(text,0,64);if(!end)goto fail;memcpy(s.role_names[j],text,(size_t)(end-text)+1);
 }
 p=at(s.mapped_dol,0x80282cfcu,14);if(!p)goto fail;memcpy(s.actor_labels,p,14);
 *out=s;return 1;
fail:free(s.mapped_dol);return 0;
}
void nflstreet_team_bootstrap_static_free(nflstreet_team_bootstrap_static*s){
 if(s){free(s->mapped_dol);memset(s,0,sizeof *s);}
}
