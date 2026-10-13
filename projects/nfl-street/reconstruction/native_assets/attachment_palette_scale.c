#include "attachment_palette_scale.h"
#include <math.h>
#include <string.h>
int nflstreet_attachment_palette_scale(const uint8_t *rows,size_t size,unsigned count,
 nflstreet_attachment_scale_slot slot,unsigned lod,float scale,float scratch[256][12],float staged[256][12]){
 if(!rows||!count||count>255||size<(size_t)count*4||slot>NFLSTREET_SCALE_SLOT_698||
    (int)slot<0||!isfinite(scale)||!scratch||!staged||scratch==staged)return 0;
 float a[256][12],b[256][12];memcpy(a,scratch,count*48u);memcpy(b,staged,count*48u);
 for(unsigned i=0;i<count;++i)for(unsigned w=0;w<12;++w)
  if(!isfinite(a[i][w])||!isfinite(b[i][w]))return 0;
 if(lod<2)for(unsigned i=0;i<count;++i){int selected;
  if(slot==NFLSTREET_SCALE_SLOT_5A8)selected=i>0&&rows[i*4]==13;
  else if(count==1)selected=1;
  else if(slot==NFLSTREET_SCALE_SLOT_698)selected=rows[i*4]==0;
  else selected=i>0&&rows[i*4]==13;
  if(selected){float *m=slot==NFLSTREET_SCALE_SLOT_788?b[i]:a[i];
   for(unsigned w=0;w<12;++w)if(w!=3&&w!=7&&w!=11){m[w]*=scale;if(!isfinite(m[w]))return 0;}
  }
 }
 /* The caller always copies scratch to the staged destination after scaling.
    The slot788 writes are therefore overwritten when the areas are disjoint. */
 if(slot!=NFLSTREET_SCALE_SLOT_5A8||lod<2)memcpy(b,a,count*48u);memcpy(scratch,a,count*48u);memcpy(staged,b,count*48u);return 1;
}
