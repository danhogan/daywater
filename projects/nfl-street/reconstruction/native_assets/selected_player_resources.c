#include "selected_player_resources.h"
#include <string.h>
void nflstreet_selected_player_resources_close(nflstreet_selected_player_resources *resources)
{
    if(!resources)return;
    nflstreet_face_resource_pair_close(&resources->face);memset(resources,0,sizeof *resources);
}
int nflstreet_selected_player_resources_open(const nflstreet_team_player_resolver *resolver,
    const uint8_t *db,size_t size,uint32_t team,uint32_t player,
    const nflstreet_body_resource_pool *bodies,uint32_t variant,
    uint32_t face_base,nflstreet_selected_player_resources *out)
{
    if(!resolver||!bodies||!out)return 0;
    nflstreet_selected_player_resources resources={0};
    if(!nflstreet_team_player_resolve(resolver,db,size,team,player,&resources.manifest)||
       resources.manifest.head_runtime_index<0||
       !nflstreet_body_resource_pool_get(bodies,resources.manifest.tbyp,variant,&resources.body,&resources.body_view))return 0;
    uint32_t key=resources.manifest.tbyp==7?2:resources.manifest.tbyp;
    resources.body_token=bodies->tokens[key*3+variant];resources.body_variant=variant;
    if(resources.body->bytes[15]!=1||!nflstreet_face_resource_pair_open(&resolver->face,
       (unsigned)resources.manifest.head_runtime_index,face_base,&resources.face))return 0;
    for(unsigned i=0;i<2;++i)for(unsigned j=0;j<9;++j){
        uint64_t face_start=resources.face.tokens[i],body_start=bodies->tokens[j];
        uint64_t face_end=face_start+resources.face.sizes[i],body_end=body_start+bodies->assets[j].size;
        if(face_start<body_end&&body_start<face_end){nflstreet_face_resource_pair_close(&resources.face);return 0;}
    }
    *out=resources;return 1;
}
