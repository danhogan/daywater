#include "face_postattach.h"
int nflstreet_face_resource_pair_bind(nflstreet_face_resource_pair *pair,
    const nflstreet_skeleton_resource *target,uint32_t view_base,
    nflstreet_model_postattach_allocate allocate,nflstreet_model_postattach_release release,void *opaque)
{
    if(!pair||!target||!view_base||view_base>UINT32_MAX-128u||!allocate||!release)return 0;
    nflstreet_model_postattach_segment segments[5]={
        {pair->tokens[0],pair->bytes[0],pair->sizes[0]},
        {pair->tokens[1],pair->bytes[1],pair->sizes[1]},
        {view_base,pair->views[0],sizeof pair->views[0]},
        {view_base+64u,pair->views[1],sizeof pair->views[1]},
        {target->token,target->bytes,target->size}
    };
    for(unsigned i=0;i<5;++i){
        if(!segments[i].token||!segments[i].bytes||segments[i].size>UINT32_MAX-segments[i].token)return 0;
        for(unsigned j=0;j<i;++j){
            uint64_t a=segments[i].token,b=segments[j].token;
            if(a<b+segments[j].size&&b<a+segments[i].size)return 0;
        }
    }
    for(unsigned i=0;i<2;++i)
        if(!nflstreet_model_postattach_bind(view_base+64u*i,target->token,segments,5,allocate,release,opaque))return 0;
    return 1;
}
