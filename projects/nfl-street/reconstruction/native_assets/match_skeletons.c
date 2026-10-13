#include "match_skeletons.h"
#include "face_postattach.h"
#include "hair_postattach.h"
#include <stdlib.h>
#include <string.h>
typedef struct temporary {struct temporary *next;uint8_t *bytes;uint32_t token;} temporary;
typedef struct {temporary *head;uint32_t base,capacity;uint64_t used;unsigned allocations,releases;int failed;} arena;
static int allocate(void *opaque,uint32_t count,uint32_t width,nflstreet_model_postattach_segment *out){
 arena *a=opaque;uint64_t size=(uint64_t)count*width,rounded=((size?size:1)+31u)&~31ull;
 if(size>UINT32_MAX||rounded>a->capacity||a->used>a->capacity-rounded){a->failed=1;return 0;}
 temporary *t=malloc(sizeof *t);if(!t){a->failed=1;return 0;}t->bytes=malloc(size? (size_t)size:1u);
 if(!t->bytes){free(t);a->failed=1;return 0;}t->token=a->base+(uint32_t)a->used;t->next=a->head;a->head=t;a->used+=rounded;++a->allocations;
 *out=(nflstreet_model_postattach_segment){t->token,t->bytes,(size_t)size};return 1;
}
static void release(void *opaque,uint32_t token){arena *a=opaque;temporary **link=&a->head;
 while(*link){temporary *t=*link;if(t->token==token){*link=t->next;free(t->bytes);free(t);++a->releases;return;}link=&t->next;}}
static void clear(arena *a){while(a->head)release(a,a->head->token);}
static int overlap(uint32_t a,size_t an,uint32_t b,size_t bn){return (uint64_t)a<(uint64_t)b+bn&&(uint64_t)b<(uint64_t)a+an;}
static int contains(const nflstreet_match_skeleton_input *i,uint32_t token,size_t size){return token>=i->namespace_base&&size&&
 (uint64_t)token+size<=(uint64_t)i->namespace_base+i->namespace_size;}
static int clone(const nflstreet_selected_player_geometry *from,nflstreet_selected_player_geometry *to){
 *to=*from;for(unsigned f=0;f<2;++f)to->base.face.bytes[f]=NULL;for(unsigned h=0;h<3;++h)to->hair.bytes[h]=NULL;
 for(unsigned f=0;f<2;++f){if(!from->base.face.bytes[f]||!from->base.face.sizes[f])return 0;
  to->base.face.bytes[f]=malloc(from->base.face.sizes[f]);if(!to->base.face.bytes[f])return 0;
  memcpy(to->base.face.bytes[f],from->base.face.bytes[f],from->base.face.sizes[f]);}
 for(unsigned h=0;h<3;++h)if(from->hair.bytes[h]){to->hair.bytes[h]=malloc(from->hair.sizes[h]?from->hair.sizes[h]:1u);
  if(!to->hair.bytes[h])return 0;memcpy(to->hair.bytes[h],from->hair.bytes[h],from->hair.sizes[h]);}
 return 1;
}
void nflstreet_match_skeletons_close(nflstreet_match_skeletons *s){if(s){for(unsigned p=0;p<14;++p)nflstreet_skeleton_startup_close(s->players+p);memset(s,0,sizeof *s);}}
int nflstreet_match_skeletons_bind(nflstreet_match_visuals *visuals,const nflstreet_body_resource_pool *bodies,
 const nflstreet_archive *animations,const nflstreet_skeleton_startup_catalog *catalog,
 const nflstreet_match_skeleton_input *inputs,nflstreet_match_skeletons *out){
 if(!visuals||visuals->count!=14||!bodies||!animations||!catalog||!inputs||!out||out->count)return 0;
 for(unsigned p=0;p<14;++p){const nflstreet_match_skeleton_input *i=inputs+p;
  for(unsigned k=0;k<6;++k)if(out->players[p].resources[k].bytes)return 0;
  if(!i->namespace_base||!i->namespace_size||(uint64_t)i->namespace_base+i->namespace_size>UINT32_MAX||i->face_target>=6||i->hair_target>=6||
     !contains(i,i->skeleton_token,1)||!contains(i,i->face_view_token,128)||!contains(i,i->hair_view_token,192)||
     !contains(i,i->temporary_token,i->temporary_capacity)||overlap(i->face_view_token,128,i->hair_view_token,192)||
     overlap(i->face_view_token,128,i->temporary_token,i->temporary_capacity)||overlap(i->hair_view_token,192,i->temporary_token,i->temporary_capacity))return 0;
  for(unsigned q=0;q<p;++q)if(overlap(i->namespace_base,i->namespace_size,inputs[q].namespace_base,inputs[q].namespace_size))return 0;
  for(unsigned b=0;b<9;++b)if(overlap(i->namespace_base,i->namespace_size,bodies->tokens[b],bodies->assets[b].size))return 0;
  for(unsigned q=0;q<14;++q){const nflstreet_selected_player_visual *v=visuals->players+q;
   if(!v->geometry.base.body||overlap(i->namespace_base,i->namespace_size,v->geometry.base.body_token,v->geometry.base.body->size))return 0;
   for(unsigned f=0;f<2;++f)if(overlap(i->namespace_base,i->namespace_size,v->geometry.base.face.tokens[f],v->geometry.base.face.sizes[f]))return 0;
   for(unsigned h=0;h<3;++h)if(v->geometry.hair.bytes[h]&&overlap(i->namespace_base,i->namespace_size,v->geometry.hair.tokens[h],v->geometry.hair.sizes[h]))return 0;
   if(overlap(i->namespace_base,i->namespace_size,v->surface.textures.token,v->surface.textures.block_size))return 0;
   for(unsigned a=0;a<v->surface.allocation_count;++a)if(overlap(i->namespace_base,i->namespace_size,v->surface.allocations[a].token,((size_t)v->surface.allocations[a].size+31u)&~(size_t)31u))return 0;
  }
 }
 nflstreet_match_skeletons *s=calloc(1,sizeof *s);nflstreet_selected_player_geometry *geometry=calloc(14,sizeof *geometry);int ok=0;
 if(!s||!geometry)goto done;
 for(unsigned p=0;p<14;++p){const nflstreet_match_skeleton_input *i=inputs+p;arena a={0};a.base=i->temporary_token;a.capacity=i->temporary_capacity;
  if(!clone(&visuals->players[p].geometry,geometry+p)||!nflstreet_skeleton_startup_open(animations,catalog,i->skeleton_token,s->players+p))goto done;
  for(unsigned k=0;k<6;++k){const nflstreet_skeleton_resource *r=s->players[p].resources+k;
   if(!contains(i,r->token,r->size)||overlap(r->token,r->size,i->face_view_token,128)||overlap(r->token,r->size,i->hair_view_token,192)||
      overlap(r->token,r->size,i->temporary_token,i->temporary_capacity))goto done;}
  int bound=nflstreet_face_resource_pair_bind(&geometry[p].base.face,s->players[p].resources+i->face_target,i->face_view_token,allocate,release,&a)&&
    nflstreet_hair_resources_bind(&geometry[p].hair,s->players[p].resources+i->hair_target,i->hair_view_token,allocate,release,&a);
  if(a.head||a.failed)bound=0;clear(&a);s->temporary_allocations+=a.allocations;s->temporary_releases+=a.releases;
  if(!bound)goto done;s->face_view_tokens[p]=i->face_view_token;s->hair_view_tokens[p]=i->hair_view_token;++s->count;
 }
 for(unsigned p=0;p<14;++p){nflstreet_selected_player_geometry_close(&visuals->players[p].geometry);visuals->players[p].geometry=geometry[p];memset(geometry+p,0,sizeof *geometry);}
 *out=*s;memset(s,0,sizeof *s);ok=1;
done:if(geometry){for(unsigned p=0;p<14;++p)nflstreet_selected_player_geometry_close(geometry+p);free(geometry);}
 if(s){nflstreet_match_skeletons_close(s);free(s);}return ok;
}
