#include "match_visuals.h"
#include <stdlib.h>
#include <string.h>
typedef struct {nflstreet_texture_flush call;void *opaque;uint32_t token,size;} pending_flush;
typedef struct {pending_flush *items;size_t count,capacity;nflstreet_texture_flush call;void *opaque;} flush_queue;
static int enqueue(void *opaque,uint32_t token,uint32_t size){
 flush_queue *q=opaque;if(q->count==q->capacity){size_t capacity=q->capacity?q->capacity*2u:64u;
  if(capacity<q->capacity||capacity>SIZE_MAX/sizeof *q->items)return 0;
  pending_flush *items=realloc(q->items,capacity*sizeof *items);if(!items)return 0;q->items=items;q->capacity=capacity;}
 q->items[q->count++]=(pending_flush){q->call,q->opaque,token,size};return 1;
}
static int contains(const nflstreet_match_visual_input *i,uint32_t token,size_t size){
 uint64_t end=(uint64_t)i->namespace_base+i->namespace_size;
 return size&&token>=i->namespace_base&&(uint64_t)token+size<=end;
}
static int overlap(uint32_t a,size_t an,uint32_t b,size_t bn){return(uint64_t)a<(uint64_t)b+bn&&(uint64_t)b<(uint64_t)a+an;}
void nflstreet_match_visuals_close(nflstreet_match_visuals *v){if(v){
 for(unsigned i=0;i<NFLSTREET_MATCH_PLAYERS;++i)nflstreet_selected_player_visual_close(v->players+i);
 memset(v,0,sizeof *v);}}
int nflstreet_match_visuals_open(const nflstreet_quick_match_session *session,
 const nflstreet_match_visual_input *inputs,nflstreet_match_visuals *out){
 if(!session||!session->stat_bytes||session->roster.game.source_play.bytes!=session->stat_bytes||!inputs||!out||out->count)return 0;
 for(unsigned p=0;p<NFLSTREET_MATCH_PLAYERS;++p){const nflstreet_match_visual_input *i=inputs+p;
  if(out->players[p].geometry.base.face.bytes[0]||out->players[p].geometry.base.face.bytes[1]||
     out->players[p].geometry.hair.bytes[0]||out->players[p].geometry.hair.bytes[1]||out->players[p].geometry.hair.bytes[2]||out->players[p].surface.textures.block||out->players[p].surface.allocation_count||
     !i->namespace_base||!i->namespace_size||(uint64_t)i->namespace_base+i->namespace_size>0x100000000ull||!i->visual.bodies||!i->visual.surface.flush||
     !contains(i,i->visual.face_identity,1)||!contains(i,i->visual.hair_identity,1)||!contains(i,i->visual.surface.destination_token,1)||
     !contains(i,i->visual.surface.scratch_token,1)||!contains(i,i->visual.surface.material_token,i->visual.surface.material_capacity))return 0;
  for(unsigned q=0;q<p;++q)if(overlap(i->namespace_base,i->namespace_size,inputs[q].namespace_base,inputs[q].namespace_size))return 0;
  for(unsigned q=0;q<NFLSTREET_MATCH_PLAYERS;++q){if(!inputs[q].visual.bodies)return 0;
   for(unsigned b=0;b<9;++b)if(overlap(i->namespace_base,i->namespace_size,inputs[q].visual.bodies->tokens[b],inputs[q].visual.bodies->assets[b].size))return 0;}
 }
 nflstreet_match_visuals *v=calloc(1,sizeof *v);flush_queue queue={0};int ok=0;if(!v)return 0;
 for(unsigned p=0;p<NFLSTREET_MATCH_PLAYERS;++p){const nflstreet_match_visual_input *i=inputs+p;
  nflstreet_selected_player_visual_context c=i->visual;queue.call=c.surface.flush;queue.opaque=c.surface.flush_opaque;c.surface.flush=enqueue;c.surface.flush_opaque=&queue;
  if(!nflstreet_selected_player_visual_from_match(session->stat_bytes,session->stat_size,&session->roster.game,
      session->roster.selected_rows[p/7][p%7],i->record_side,&c,v->players+p))goto done;
  nflstreet_selected_player_visual *player=v->players+p;
  for(unsigned f=0;f<2;++f)if(!contains(i,player->geometry.base.face.tokens[f],player->geometry.base.face.sizes[f]))goto done;
  for(unsigned h=0;h<3;++h)if(player->geometry.hair.bytes[h]&&!contains(i,player->geometry.hair.tokens[h],player->geometry.hair.sizes[h]))goto done;
  if(!contains(i,player->surface.textures.token,player->surface.textures.block_size)||
     !contains(i,c.surface.scratch_token,player->surface.textures.scratch_capacity))goto done;
  ++v->count;
 }
 for(size_t n=0;n<queue.count;++n)if(!queue.items[n].call(queue.items[n].opaque,queue.items[n].token,queue.items[n].size))goto done;
 *out=*v;memset(v,0,sizeof *v);ok=1;
done:nflstreet_match_visuals_close(v);free(v);free(queue.items);return ok;
}
