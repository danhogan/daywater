#include "team_devices.h"
#include <string.h>

int nflstreet_team_devices_project(const nflstreet_team_device_record *records,
 size_t record_count,const uint32_t mapping[NFLSTREET_TEAM_DEVICE_MAPPING_COUNT],
 uint32_t side_words[NFLSTREET_TEAM_DEVICE_SLOTS]){
 if((record_count&&!records)||!mapping||!side_words||
    record_count>NFLSTREET_TEAM_DEVICE_MAX_ITERATOR_RECORDS)return 0;
 uint32_t result[NFLSTREET_TEAM_DEVICE_SLOTS];
 for(unsigned i=0;i<NFLSTREET_TEAM_DEVICE_SLOTS;++i)result[i]=UINT32_MAX;
 for(size_t i=0;i<record_count;++i){
  uint32_t handle=records[i].port_handle;
  if(handle>=NFLSTREET_TEAM_DEVICE_MAPPING_COUNT)return 0;
  uint32_t slot=mapping[handle];
  if(slot==UINT32_MAX)continue;
  if(slot>=NFLSTREET_TEAM_DEVICE_SLOTS)return 0;
  uint32_t side=records[i].side;
  result[slot]=(side==0||side==1)?side:UINT32_MAX;
 }
 memcpy(side_words,result,sizeof result);return 1;
}
