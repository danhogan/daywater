#include "ball_attachment.h"
#include <math.h>
#include <string.h>
int nflstreet_ball_attachment_position(const float world[12],const float (*clusters)[12],size_t count,int selector,const float local[3],float position[3]){
 if(!position||selector< -1)return 0;
 float out[3]={0};
 if(selector== -1){memcpy(position,out,sizeof out);return 1;}
 if(!world||!clusters||!local||(size_t)selector>=count)return 0;
 for(unsigned i=0;i<12;++i)if(!isfinite(world[i])||!isfinite(clusters[selector][i]))return 0;
 for(unsigned i=0;i<3;++i)if(!isfinite(local[i]))return 0;
 float matrix[12];const float*b=clusters[selector];
 for(unsigned row=0;row<3;++row)for(unsigned col=0;col<4;++col){
  float v=world[row*4]*b[col];v=fmaf(world[row*4+1],b[4+col],v);v=fmaf(world[row*4+2],b[8+col],v);
  /* Original paired-single multiply-add includes the affine fourth row. */
  if(col>=2)v=fmaf(col==3?1.0f:0.0f,world[row*4+3],v);
  if(!isfinite(v))return 0;matrix[row*4+col]=v;
 }
 for(unsigned row=0;row<3;++row){float v=local[1]*matrix[row*4+1];
  v=fmaf(local[0],matrix[row*4],v);v=fmaf(local[2],matrix[row*4+2],v);out[row]=matrix[row*4+3]+v;
  if(!isfinite(out[row]))return 0;
 }
 memcpy(position,out,sizeof out);return 1;
}
