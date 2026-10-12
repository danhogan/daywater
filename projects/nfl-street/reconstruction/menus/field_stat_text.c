#include "field_stat_text.h"
#include <limits.h>
#include <stdio.h>
#include <string.h>
int nflstreet_field_stat_text(int32_t value, char output[33]) {
    if (!output || value==INT32_MIN) return 0;
    uint32_t magnitude=value<0?(uint32_t)(-value):(uint32_t)value;
    uint32_t groups[4];unsigned count=0;
    do { groups[count++]=magnitude%1000; magnitude/=1000; } while(magnitude);
    char next[33]={0};unsigned used=0;
    if(value<0)next[used++]='-';
    int n=snprintf(next+used,sizeof next-used,"%u",groups[--count]);
    if(n<0||(unsigned)n>=sizeof next-used)return 0;used+=(unsigned)n;
    while(count){n=snprintf(next+used,sizeof next-used,",%03u",groups[--count]);
        if(n<0||(unsigned)n>=sizeof next-used)return 0;used+=(unsigned)n;}
    memcpy(output,next,sizeof next);return 1;
}
