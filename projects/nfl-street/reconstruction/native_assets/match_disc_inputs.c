#include "match_disc_inputs.h"
#include "disc_gear_catalog.h"
#include "disc_slot_materials.h"
#include "skeleton_disc.h"
#include "disc_files.h"
#include <stdlib.h>
struct nflstreet_match_disc_inputs {
    nflstreet_disc_gear_catalog gear;
    nflstreet_disc_slot_materials materials;
    nflstreet_skeleton_disc_source *skeleton_source;
    uint8_t *animation_bytes;nflstreet_archive animations;
    nflstreet_gear_records records;nflstreet_color_table colors;nflstreet_dynclut_table dynamic;
    nflstreet_match_visuals visuals;nflstreet_match_skeletons skeletons;
};
typedef struct {nflstreet_texture_flush call;void *opaque;uint32_t token,size;} notification;
typedef struct {notification *items;size_t count,capacity;} queue;
typedef struct {queue *queue;nflstreet_texture_flush call;void *opaque;} callback;
static int defer(void *opaque,uint32_t token,uint32_t size){
    callback *c=opaque;queue *q=c->queue;
    if(q->count==q->capacity){size_t capacity=q->capacity?q->capacity*2:256;
        if(capacity<q->capacity||capacity>SIZE_MAX/sizeof *q->items)return 0;
        notification *items=realloc(q->items,capacity*sizeof *items);if(!items)return 0;q->items=items;q->capacity=capacity;}
    q->items[q->count++]=(notification){c->call,c->opaque,token,size};return 1;
}
void nflstreet_match_disc_inputs_close(nflstreet_match_disc_inputs *s){if(s){
    nflstreet_match_visuals_close(&s->visuals);
    nflstreet_match_skeletons_close(&s->skeletons);
    nflstreet_dynclut_table_close(&s->dynamic);
    nflstreet_skeleton_disc_close(s->skeleton_source);
    nflstreet_disc_slot_materials_destroy(&s->materials);
    nflstreet_disc_gear_catalog_close(&s->gear);
    free(s->animation_bytes);free(s);
}}
int nflstreet_match_disc_inputs_open(const char *iso,const nflstreet_quick_match_session *session,
    const nflstreet_match_visual_input *inputs,const nflstreet_match_skeleton_input *binding,nflstreet_match_disc_inputs **out){
    if(!iso||!session||!session->stat_bytes||!inputs||!binding||!out||*out)return 0;
    for(unsigned p=0;p<14;++p)if(!inputs[p].visual.bodies||inputs[p].visual.bodies!=inputs[0].visual.bodies||
        !inputs[p].visual.dynamic_palettes||inputs[p].visual.dynamic_palettes!=inputs[0].visual.dynamic_palettes||
        !inputs[p].visual.surface.flush||binding[p].face_target>=6||binding[p].hair_target>=6)return 0;
    nflstreet_match_disc_inputs *s=calloc(1,sizeof *s);nflstreet_disc_files *disc=NULL;size_t size=0;
    queue pending={0};callback callbacks[14];int ok=0;if(!s)return 0;
    if(!nflstreet_disc_gear_catalog_open_disc(iso,&s->gear)||!nflstreet_disc_slot_materials_from_disc(iso,&s->materials)||
        !(s->skeleton_source=nflstreet_skeleton_disc_open(iso)))goto done;
    disc=nflstreet_disc_open(iso);if(!disc||!nflstreet_disc_read(disc,"animdata.dat",&s->animation_bytes,&size)||
        !nflstreet_archive_open(s->animation_bytes,size,&s->animations))goto done;
    nflstreet_team_table table;
    if(!nflstreet_team_table_open_tag(session->stat_bytes,session->stat_size,0x52414547u,&table)||
        !nflstreet_gear_records_build(&table,&s->gear.catalog,&s->records)||
        !nflstreet_color_table_build(session->stat_bytes,session->stat_size,&s->gear.catalog,&s->colors)||
        !nflstreet_dynclut_table_open(inputs[0].visual.dynamic_palettes,&s->gear.catalog,&s->dynamic))goto done;
    nflstreet_match_visual_input local[14];
    for(unsigned p=0;p<14;++p){local[p]=inputs[p];
        local[p].visual.gear=&s->records;
        local[p].visual.surface.gear=&s->gear.catalog;
        local[p].visual.surface.materials=&s->materials.catalog;
        local[p].visual.surface.colors=&s->colors;
        local[p].visual.surface.dynclut=&s->dynamic;
        callbacks[p]=(callback){&pending,inputs[p].visual.surface.flush,inputs[p].visual.surface.flush_opaque};
        local[p].visual.surface.flush=defer;local[p].visual.surface.flush_opaque=callbacks+p;
    }
    nflstreet_skeleton_startup_catalog catalog;
    if(!nflstreet_skeleton_disc_catalog(s->skeleton_source,&catalog)||
        !nflstreet_match_visuals_open(session,local,&s->visuals)||
        !nflstreet_match_skeletons_bind(&s->visuals,inputs[0].visual.bodies,&s->animations,&catalog,binding,&s->skeletons))goto done;
    /* Commit host notifications only after both constructor stages succeeded. */
    for(size_t i=0;i<pending.count;++i)if(!pending.items[i].call(pending.items[i].opaque,pending.items[i].token,pending.items[i].size))goto done;
    *out=s;s=NULL;ok=1;
done:nflstreet_disc_close(disc);free(pending.items);nflstreet_match_disc_inputs_close(s);return ok;
}
const nflstreet_match_visuals *nflstreet_match_disc_inputs_visuals(const nflstreet_match_disc_inputs *s){return s?&s->visuals:NULL;}
const nflstreet_match_skeletons *nflstreet_match_disc_inputs_skeletons(const nflstreet_match_disc_inputs *s){return s?&s->skeletons:NULL;}
const nflstreet_gear_texture_catalog *nflstreet_match_disc_inputs_gear(const nflstreet_match_disc_inputs *s){return s?&s->gear.catalog:NULL;}
const nflstreet_slot_material_catalog *nflstreet_match_disc_inputs_materials(const nflstreet_match_disc_inputs *s){return s?&s->materials.catalog:NULL;}
