#ifndef NFLSTREET_TEAM_FORMAT_H
#define NFLSTREET_TEAM_FORMAT_H
#include "script.h"
/* Owned bounded source4C formatting for literal text and %d/%i/%u/%s/%%.
   Formats, descriptor arguments and destination remain private caller bytes. */
int nflstreet_team_format(uint8_t*,size_t,uint32_t,const nflstreet_ui_service_request*);
#endif
