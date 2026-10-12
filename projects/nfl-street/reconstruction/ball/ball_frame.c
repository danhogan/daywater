#include "ball_frame.h"
#include "ball_flight.h"
#include <math.h>
#include <string.h>
static uint32_t be(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
int nflstreet_ball_frame(uint8_t*r,size_t n,double dt,double plane,
 const nflstreet_ball_attachment_inputs*a,const nflstreet_ball_pose*visual,
 nflstreet_ball_frame_output*out){
 if(!r||n<244||!visual||!out||!isfinite(dt))return 0;
 for(unsigned i=0;i<3;++i)if(!isfinite(visual->position[i]))return 0;
 for(unsigned i=0;i<4;++i)if(!isfinite(visual->quaternion[i]))return 0;
 if((be(r+0xb4)!=0)!=(a!=NULL))return 0;
 uint8_t next[244];memcpy(next,r,sizeof next);
 /* Original copies all seven position/orientation words before either branch. */
 memcpy(next+0x28,next+0x0c,28);
 nflstreet_ball_frame_output result={0};result.visual_pose=*visual;
 if(a){if(!nflstreet_ball_update_attached(next,sizeof next,a->world,a->model_root,
 a->model_scale,a->bones,a->count,a->selector,a->local,a->rotation,dt,
 &result.visual_pose))return 0;}
 else if(!nflstreet_ball_update_airborne(next,sizeof next,dt,plane))return 0;
 /* Source 80252164 is positive zero; original clears force and angular inputs. */
 memset(next+0x7c,0,24);
 memcpy(result.spatial_query_position,result.visual_pose.position,12);
 result.spatial_query=1;
 memcpy(r,next,sizeof next);*out=result;return 1;
}
