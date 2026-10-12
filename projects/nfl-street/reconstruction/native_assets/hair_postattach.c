#include "hair_postattach.h"
int nflstreet_hair_resources_bind(nflstreet_hair_resources *hair,const nflstreet_skeleton_resource *target,
 uint32_t view_base,nflstreet_model_postattach_allocate allocate,nflstreet_model_postattach_release release,void *opaque){
 if(!hair||!target||!view_base||view_base>UINT32_MAX-192u||!allocate||!release)return 0;
 nflstreet_model_postattach_segment segments[7];size_t count=0;
 for(unsigned i=0;i<3;++i){
  if(hair->bytes[i])segments[count++]=(nflstreet_model_postattach_segment){hair->tokens[i],hair->bytes[i],hair->sizes[i]};
  segments[count++]=(nflstreet_model_postattach_segment){view_base+64u*i,hair->views[i],52};
 }segments[count++]=(nflstreet_model_postattach_segment){target->token,target->bytes,target->size};
 for(size_t i=0;i<count;++i){
  if(!segments[i].token||!segments[i].bytes||segments[i].size>UINT32_MAX-segments[i].token)return 0;
  for(size_t j=0;j<i;++j)if((uint64_t)segments[i].token<(uint64_t)segments[j].token+segments[j].size&&
      (uint64_t)segments[j].token<(uint64_t)segments[i].token+segments[i].size)return 0;
 }
 for(unsigned i=0;i<3;++i)
  if(!nflstreet_model_postattach_bind(view_base+64u*i,target->token,segments,count,allocate,release,opaque))return 0;
 return 1;
}
