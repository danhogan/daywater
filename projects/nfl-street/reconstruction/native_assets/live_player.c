/* Live independent inspection viewer. Original model/bank/field assets are
   cached once. Host movement, fixed camera, clip choices and hip placement
   are inspection choices, not reconstructed gameplay. Hair is rigid. */
#include "player.h"
#include "pose.h"
#include "compact_pose.h"
#include "../ball/ball_attachment.h"
#include "../ball/ball_orientation.h"
#include "../ball/ball_ownership.h"
#include "../ball/ball_flight.h"
#include "../ball/ball_frame.h"
#include "../ball/ball_lifecycle.h"
#include "archive.h"
#include "sky.h"
#include "live_movement.h"
#include "live_locomotion.h"
#include "live_contact.h"
#include "../actor_target_input/command_timing.h"
#include <windows.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define RECORD 3876u
static unsigned u16(const uint8_t *p){return (unsigned)p[0]<<8|p[1];}
static uint32_t u32(const uint8_t *p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static uint32_t le32(const uint8_t *p){return p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static void put(uint8_t *p,float f){uint32_t w;memcpy(&w,&f,4);for(unsigned i=0;i<4;++i)p[i]=(uint8_t)(w>>(i*8));}
static int span(size_t n,size_t o,size_t s){return o<=n&&s<=n-o;}
static uint8_t *readfile(const char *path,size_t *size){FILE *f=fopen(path,"rb");if(!f)return NULL;
    if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}long n=ftell(f);if(n<16||n>16*1024*1024||fseek(f,0,SEEK_SET)){fclose(f);return NULL;}
    uint8_t *b=malloc((size_t)n);if(b&&fread(b,1,(size_t)n,f)!=(size_t)n){free(b);b=NULL;}fclose(f);*size=(size_t)n;return b;
}
static int sections(const uint8_t *b,size_t n,size_t *sk,size_t *cl,size_t *ge){if(n<16||memcmp(b,"TMdl",4))return 0;
    unsigned dir=u16(b+10),count=u16(b+8);if(!span(n,dir,count*16))return 0;*sk=*cl=*ge=0;
    for(unsigned i=0;i<count;++i){const uint8_t *s=b+dir+i*16;size_t o=u32(s+4);if(!span(n,o,u32(s+8)))return 0;
        if(!memcmp(s,"Skel",4))*sk=o;if(!memcmp(s,"Clst",4))*cl=o;if(!memcmp(s,"Geom",4))*ge=o;}
    return *sk&&*cl&&*ge;
}
/* Resolve indexed loads up to the selected primitive. Their source indices
   are original Clst selectors; raw PN selectors need not be divisible by 3. */
static int slots(const uint8_t *b,size_t n,size_t geom,const uint8_t *r,int out[64]){for(unsigned i=0;i<64;++i)out[i]=-1;
    unsigned row=le32(r+8);if(row>=u32(b+geom+4))return 0;size_t d=u32(b+geom)+(size_t)row*24;if(!span(n,d,24))return 0;
    size_t at=u32(b+d),target=le32(r+12),end=at+(size_t)u16(b+d+4)*32;
    if(target<at||target>=end||!span(n,at,end-at))return 0;unsigned sz=1;size_t attrs=u32(b+geom+8)+(size_t)u16(b+d+14)*12;
    if(!span(n,attrs,b[d+16]*12))return 0;for(unsigned i=0;i<b[d+16];++i){unsigned mode=b[attrs+i*12+11]&15;if(mode<2||mode>3)return 0;sz+=mode-1;}
    while(at<target){unsigned cmd=b[at++];if(!cmd)continue;
        if(cmd==0x20||cmd==0x28){if(end-at<4)return 0;unsigned source=u16(b+at),packed=u16(b+at+2),address=packed&4095;at+=4;
            if(cmd==0x20){if((packed>>12)!=11||address%4||address/4>61)return 0;unsigned slot=address/4;
                for(unsigned i=0;i<64;++i)if(i<=slot+2&&slot<=i+2)out[i]=-1;out[slot]=(int)source;}continue;}
        if((cmd!=0x80&&cmd!=0x90&&cmd!=0x98&&cmd!=0xa0)||end-at<2)return 0;unsigned vertices=u16(b+at);at+=2;
        if((size_t)vertices*sz>end-at)return 0;at+=(size_t)vertices*sz;
    }
    if(at!=target||!span(n,target,3))return 0;
    unsigned vertices=u16(b+target+1);
    if(!span(n,target+3,(size_t)vertices*sz))return 0;
    for(unsigned i=0;i<vertices;++i){unsigned selector=b[target+3+(size_t)i*sz];if(selector>63||out[selector]<0)return 0;}
    return 1;
}
#define MAX_MODELS 8
/* These are inspection controls and timing, not reconstructed gameplay. */
typedef struct {uint8_t *b;size_t n,sk,cl,ge;unsigned category,entry,bones,clusters;int map[768];int16_t stored[768];float matrix[256][12];} component;
typedef struct {unsigned frames[1024],count;} clip;
static component models[MAX_MODELS];static unsigned model_count,draw_count,neck;
static uint8_t *profile,*bank,*skeleton;static size_t profile_n,bank_n,skeleton_n;
static nflstreet_player player;static nflstreet_player_source *sources;static int (*selectors)[64];static unsigned *model_ids;
static nflstreet_field_surface field;static nflstreet_texture image;
static float camera[12],x,y=-20,facing,phase;static unsigned keys[256],ticks,paints;static clip idle,run;
static int check,failed,direction_check;
static uint8_t animation_stream[64];static const uint8_t *run_program;static size_t run_program_n;static unsigned run_identity,animation_flags;static int animation_selection=-1;static const uint8_t *idle_program;static size_t idle_program_n;static unsigned idle_identity;
static nflstreet_live_contact contact_seed,contact;static const char *contact_selection_path,*contact_pair_path;static int contact_check;static unsigned contact_starts;
static nflstreet_live_movement movement,second_movement;
static nflstreet_live_locomotion second_locomotion;static int second_mode;
static double second_clock;static unsigned second_moving_frames,scene_checks,second_visible_frames;static float second_traveled;
static uint8_t *set_catalog;static size_t set_catalog_n;static unsigned requested_set;static int set_check;
static nflstreet_live_locomotion locomotion;static uint8_t *locomotion_bank,*locomotion_profile;static size_t locomotion_bank_n,locomotion_profile_n;
static uint8_t *movement_fixture;static size_t movement_fixture_n;
static int native_mode,movement_status,braking_fixture;static double movement_clock;static const char *output;static LARGE_INTEGER frequency,last;
static float max_step_ms;static unsigned moving_frames;static float traveled;
static void concat(const float *a,const float *b,float *c){for(unsigned r=0;r<3;++r){for(unsigned j=0;j<3;++j)c[r*4+j]=a[r*4]*b[j]+a[r*4+1]*b[4+j]+a[r*4+2]*b[8+j];c[r*4+3]=a[r*4]*b[3]+a[r*4+1]*b[7]+a[r*4+2]*b[11]+a[r*4+3];}}
static int get_clip(unsigned id,clip *c){size_t table=u32(bank+12);if(!span(bank_n,table,4)||id>=u32(bank+table)||!span(bank_n,table+4+id*4,4))return 0;
 size_t at=u32(bank+table+4+id*4)+4;for(;span(bank_n,at,2)&&c->count<1024;at+=2){unsigned w=u16(bank+at);if(w==0x8001)return c->count>1;
 if(w==0x8000)return 0;if(!(w&0x8000)){if(w>=u16(bank+4))return 0;c->frames[c->count++]=w;}else if(!(w&0x4000))return 0;}return 0;}
static int actor_open(const char *root,const char *profile_path){profile=readfile(profile_path,&profile_n);char path[4096];
 if(!profile||memcmp(profile,"NAP1",4)||profile_n!=16+(size_t)le32(profile+4)*RECORD)return 0;draw_count=le32(profile+4);if(!draw_count||draw_count>2000)return 0;
 snprintf(path,sizeof path,"%s/animdata/00103.bin",root);bank=readfile(path,&bank_n);snprintf(path,sizeof path,"%s/animdata/00095.bin",root);skeleton=readfile(path,&skeleton_n);
 if(!bank||!skeleton||memcmp(skeleton,"SKL1",4)||!get_clip(18,&idle)||!get_clip(575,&run))return 0;
 size_t program_at=u32(bank+u32(bank+12)+4+575*4);
 run_identity=(unsigned)program_at;run_program=bank+program_at;run_program_n=4+2*(run.count+1);
 if(!span(bank_n,program_at,run_program_n))return 0;
 /* This selected inspection cycle contains ordinary tags and loop only. */
 for(size_t j=4;j<run_program_n;j+=2)if(u16(run_program+j)&0x8000){if(u16(run_program+j)!=0x8001||j+2!=run_program_n)return 0;}
 program_at=u32(bank+u32(bank+12)+4+18*4);idle_identity=(unsigned)program_at;
 idle_program=bank+program_at;idle_program_n=4+2*(idle.count+2);
 if(!span(bank_n,program_at,idle_program_n))return 0;
 for(size_t j=4;j<idle_program_n;j+=2){unsigned w=u16(idle_program+j);
 if((w&0x8000)&&w!=0xc00c&&(w!=0x8001||j+2!=idle_program_n))return 0;}
 sources=calloc(draw_count,sizeof *sources);selectors=calloc(draw_count,sizeof *selectors);model_ids=calloc(draw_count,sizeof *model_ids);if(!sources||!selectors||!model_ids)return 0;
 const char *cats[]={"plamodel","plaface","plahair","plahat","plaglass","plarosterhair"};
 size_t external=u32(skeleton+12);unsigned external_count=u16(skeleton+6);if(!span(skeleton_n,external,external_count*20))return 0;
 for(unsigned i=0;i<draw_count;++i){uint8_t *r=profile+16+(size_t)i*RECORD;unsigned cat=le32(r),entry=le32(r+4),id; if(cat>=6)return 0;
 for(id=0;id<model_count;++id)if(models[id].category==cat&&models[id].entry==entry)break;
 if(id==model_count){if(model_count==MAX_MODELS)return 0;component *m=models+model_count++;m->category=cat;m->entry=entry;
 if(snprintf(path,sizeof path,"%s/%s/%05u.bin",root,cats[cat],entry)>=(int)sizeof path)return 0;m->b=readfile(path,&m->n);
 if(!m->b||!sections(m->b,m->n,&m->sk,&m->cl,&m->ge))return 0;m->bones=u32(m->b+m->sk);size_t names=u32(m->b+m->sk+4),stored=u32(m->b+m->sk+12);
 if(m->bones>256||!span(m->n,names,m->bones*16)||!span(m->n,stored,m->bones*6))return 0;
 for(unsigned j=0;j<m->bones*3;++j){m->stored[j]=(int16_t)u16(m->b+stored+j*2);m->map[j]=-1;}
 for(unsigned j=0;j<m->bones;++j)for(unsigned k=0;k<external_count;++k)if(!memcmp(m->b+names+j*16,skeleton+external+k*20+4,16))for(unsigned axis=0;axis<3;++axis)m->map[j*3+axis]=(int)(k*3+axis);
 if(!nflstreet_pose_clusters(m->b,m->n,m->stored,m->bones*3,m->matrix,&m->clusters))return 0;
 if(id==0){size_t rows=u32(m->b+m->cl);for(unsigned j=1;j<m->clusters;++j)if(!strcmp((char *)(m->b+names+m->b[rows+j*4]*16),"neckhi")&&u16(m->b+rows+j*4+2)==4096)neck=j;}
 if(m->clusters==1&&(!span(m->n,names,16)||strcmp((char *)(m->b+names),"neckhi")))return 0;
 }
 component *m=models+id;model_ids[i]=id;sources[i].bytes=m->b;sources[i].size=m->n;if(!slots(m->b,m->n,m->ge,r,selectors[i]))return 0;
 }
 if(!neck||!nflstreet_player_open(root,profile,profile_n,&player))return 0;
 if(check){size_t bytes=player.mesh.count*sizeof *player.mesh.triangles;void *original=malloc(bytes);if(!original)return 0;memcpy(original,player.mesh.triangles,bytes);
 int same=nflstreet_player_repose(&player,profile,profile_n,sources)&&!memcmp(original,player.mesh.triangles,bytes);free(original);if(!same)return 0;
 puts("Cached model reposing matches initial fixed-profile geometry exactly");}
 return 1;
}
static nflstreet_player ball_player;static uint8_t *ball_profile,*ball_asset;static size_t ball_profile_n,ball_asset_n;static nflstreet_player_source *ball_sources;
static const char *ball_profile_path;static unsigned ball_frames;static size_t ball_shaded;
static int ball_open(const char*root){
 if(!ball_profile_path)return 1;ball_profile=readfile(ball_profile_path,&ball_profile_n);
 if(!ball_profile||!nflstreet_player_open(root,ball_profile,ball_profile_n,&ball_player))return 0;
 unsigned entry=le32(ball_profile+20);char path[4096];snprintf(path,sizeof path,"%s/statmod/%05u.bin",root,entry);ball_asset=readfile(path,&ball_asset_n);
 if(!ball_asset)return 0;ball_sources=calloc(ball_player.count,sizeof *ball_sources);if(!ball_sources)return 0;
 for(unsigned i=0;i<ball_player.count;++i){const uint8_t*r=ball_profile+16+(size_t)i*RECORD;if(le32(r)!=6||le32(r+4)!=entry)return 0;ball_sources[i]=(nflstreet_player_source){ball_asset,ball_asset_n};}
 return 1;
}
static int ball_repose(const nflstreet_ball_pose*pose,const float eye[12]){
 float local[12],matrix[12];if(!nflstreet_ball_render_matrix(pose,local))return 0;concat(eye,local,matrix);
 for(unsigned i=0;i<ball_player.count;++i){uint8_t*r=ball_profile+16+(size_t)i*RECORD;
  for(unsigned k=0;k<12;++k)put(r+256+k*4,matrix[k]);for(unsigned row=0;row<3;++row)for(unsigned col=0;col<3;++col)put(r+1312+row*12+col*4,matrix[row*4+col]);}
 return nflstreet_player_repose(&ball_player,ball_profile,ball_profile_n,ball_sources);
}
static int ball_layer(nflstreet_field_surface*scene){
 if(!ball_profile)return 1;nflstreet_field_surface next={0};nflstreet_field_stats stats;
 if(!nflstreet_field_layer(scene,&ball_player.mesh,ball_player.draws,ball_player.count,&next,&stats))return 0;
 ++ball_frames;ball_shaded+=stats.shaded;nflstreet_field_surface_close(scene);*scene=next;return 1;
}
static unsigned ownership_mode,ownership_check,ownership_assignments,ownership_releases,ownership_frozen_checks,ownership_updates[2];
static int ownership_pending=0;static uint8_t ownership_record[0xf4],ownership_frozen[0xf4];
static uint32_t ownership_flags[2]={0xffffffff,0xffffffff};static const uint32_t ownership_headers[2]={0x00000101,0x00010101};
static nflstreet_ball_assignment_context ownership_context={3,0,0};static float ownership_world[12];
static unsigned airborne_mode,airborne_updates,airborne_moved,airborne_pauses;
static unsigned ball_frame_mode,ball_frame_updates,ball_frame_history_checks,ball_frame_queries;
static float ownership_float(unsigned offset){uint32_t w=u32(ownership_record+offset);float f;memcpy(&f,&w,4);return f;}
static void ownership_put(unsigned offset,float f){uint32_t w;memcpy(&w,&f,4);uint8_t*p=ownership_record+offset;p[0]=w>>24;p[1]=w>>16;p[2]=w>>8;p[3]=w;}
static int frame_ball(double dt,const nflstreet_ball_attachment_inputs*actor){
 uint8_t history[28];memcpy(history,ownership_record+12,sizeof history);
 nflstreet_ball_pose visual;for(unsigned i=0;i<3;++i)visual.position[i]=ownership_float(12+i*4);for(unsigned i=0;i<4;++i)visual.quaternion[i]=ownership_float(0x18+i*4);
 nflstreet_ball_frame_output output;
 if(!nflstreet_ball_frame(ownership_record,sizeof ownership_record,dt,0,actor,&visual,&output))return 0;
 if(memcmp(history,ownership_record+0x28,sizeof history))return 0;
 for(unsigned i=0;i<24;++i)if(ownership_record[0x7c+i])return 0;
 if(output.spatial_query!=1)return 0;
 /* Record the request; the original environment index is not reconstructed. */
 for(unsigned i=0;i<3;++i)if(!isfinite(output.spatial_query_position[i]))return 0;
 ++ball_frame_updates;++ball_frame_history_checks;++ball_frame_queries;return 1;
}
static int airborne_frame(float dt){
 if(!airborne_mode||!ownership_releases||u32(ownership_record+0xb4)||ownership_pending>=0)return 1;
 if(ownership_float(20)<=0){++airborne_pauses;return 1;}/* Host stop at plane, not original collision. */
 float previous[3];for(unsigned i=0;i<3;++i)previous[i]=ownership_float(12+i*4);
 if(ball_frame_mode){/* Explicit supplied force each frame; original caller clears it. */
  ownership_put(0x7c,0);ownership_put(0x80,0);ownership_put(0x84,-9);
  if(!frame_ball(dt,NULL))return 0;
 }else if(!nflstreet_ball_update_airborne(ownership_record,sizeof ownership_record,dt,0))return 0;
 nflstreet_ball_pose pose;for(unsigned i=0;i<3;++i)pose.position[i]=ownership_float(12+i*4);for(unsigned i=0;i<4;++i)pose.quaternion[i]=ownership_float(0x18+i*4);
 ++airborne_updates;if(memcmp(previous,pose.position,sizeof previous))++airborne_moved;
 return ball_repose(&pose,camera);
}
static int ownership_request(void){
 uint32_t handle=u32(ownership_record+0xb4);
 if(handle==ownership_headers[0])ownership_pending=1;
 else if(handle==ownership_headers[1]){if(!nflstreet_ball_release(ownership_record,sizeof ownership_record,0,0,0,0))return 0;
  if(airborne_mode){/* Supplied inspection force and no-spin context, not original launch/gravity policy. */
   memset(ownership_record+0x60,0,16);ownership_put(0x6c,1);memset(ownership_record+0x88,0,12);ownership_put(0x9c,1);ownership_put(0xe0,0);ownership_put(0xd0,0);
   ownership_put(0x7c,0);ownership_put(0x80,0);ownership_put(0x84,-9);
  }
  memcpy(ownership_frozen,ownership_record,sizeof ownership_record);++ownership_releases;ownership_pending=-1;}
 else ownership_pending=0;return 1;
}
static uint8_t *attachment_profile;static size_t attachment_profile_n;
static unsigned attachment_rotations,attachment_updates,attachment_actor;
static uint8_t attachment_records[2][2][0xf4];static unsigned attachment_initialized[2][2];
static unsigned attachment_poses,attachment_changed;static float attachment_first[2][3],attachment_min[2][3],attachment_max[2][3];
static int attachment_pose(const int16_t *raw,unsigned n,const float *eye){
 if(!attachment_profile)return 1;
 unsigned bones=u16(skeleton+6);if(bones!=29||n%3||n>bones*3)return 0;
 int16_t angles[87]={0};memcpy(angles,raw,n*sizeof *raw);
 float identity[12]={1,0,0,0,0,1,0,0,0,0,1,0},matrices[256][12],local[3];unsigned count;
 for(unsigned i=0;i<3;++i){uint32_t word=u32(attachment_profile+12+i*4);memcpy(local+i,&word,4);}
 if(!nflstreet_compact_pose(skeleton,skeleton_n,angles,87,identity,matrices,&count))return 0;
 for(unsigned side=0;side<2;++side){unsigned selector=u32(attachment_profile+4+side*4);float point[3],camera_point[3];
  if(selector>=count||!nflstreet_ball_attachment_position(identity,matrices,count,(int)selector,local,point)||
     !nflstreet_ball_attachment_position(eye,matrices,count,(int)selector,local,camera_point))return 0;
  if(attachment_profile_n>=36){int32_t rotation[3];nflstreet_ball_pose pose;float ball_rotation[4];
   for(unsigned i=0;i<3;++i)rotation[i]=(int32_t)u32(attachment_profile+24+i*4);
   if(!nflstreet_ball_attachment_pose(identity,matrices,count,(int)selector,local,rotation,&pose)||
      memcmp(pose.position,point,sizeof point)||!nflstreet_ball_attachment_rotation(pose.quaternion,ball_rotation))return 0;
   float norm=0;for(unsigned i=0;i<4;++i)norm+=ball_rotation[i]*ball_rotation[i];
   if(!isfinite(norm)||fabsf(norm-1.0f)>0.00001f)return 0;++attachment_rotations;
   if(ball_profile&&!ownership_mode&&attachment_actor==0&&side==1){memcpy(pose.quaternion,ball_rotation,sizeof ball_rotation);if(!ball_repose(&pose,eye))return 0;}
   if(attachment_profile_n==88){float root[12],scale;uint32_t w=u32(attachment_profile+36);memcpy(&scale,&w,4);
    for(unsigned i=0;i<12;++i){w=u32(attachment_profile+40+i*4);memcpy(root+i,&w,4);}
    if(ownership_mode&&ball_profile&&side==1){nflstreet_ball_attachment_inputs inputs={ownership_world,root,scale,matrices,count,(int)selector,local,rotation};nflstreet_ball_pose visual;
     if(ownership_pending==(int)attachment_actor){if(!nflstreet_ball_assign(ownership_record,sizeof ownership_record,ownership_headers[attachment_actor],ownership_flags+attachment_actor,&ownership_context,0,&inputs,&visual))return 0;ownership_pending=-1;++ownership_assignments;}
     if(u32(ownership_record+0xb4)==ownership_headers[attachment_actor]){
      if(ball_frame_mode){if(!frame_ball(1.0/60.0,&inputs))return 0;}
      else{memcpy(ownership_record+0x28,ownership_record+12,12);
       if(!nflstreet_ball_update_attached(ownership_record,sizeof ownership_record,ownership_world,root,scale,matrices,count,(int)selector,local,rotation,1.0/60.0,&visual))return 0;}
      for(unsigned i=0;i<3;++i){uint32_t word=u32(ownership_record+12+i*4);memcpy(pose.position+i,&word,4);}
      for(unsigned i=0;i<4;++i){uint32_t word=u32(ownership_record+0x18+i*4);memcpy(pose.quaternion+i,&word,4);}
      if(!ball_repose(&pose,camera))return 0;++ownership_updates[attachment_actor];
     }
    }
    uint8_t*r=attachment_records[attachment_actor][side];nflstreet_ball_pose model;
    if(!attachment_initialized[attachment_actor][side]){for(unsigned i=0;i<3;++i){memcpy(&w,point+i,4);for(unsigned j=0;j<4;++j)r[0x28+i*4+j]=(uint8_t)(w>>(24-j*8));}}
    if(!nflstreet_ball_update_attached(r,0xf4,identity,root,scale,matrices,count,(int)selector,local,rotation,1.0/60.0,&model))return 0;
    for(unsigned i=0;i<3;++i){w=u32(r+12+i*4);float current;memcpy(&current,&w,4);if(current!=point[i]||!isfinite(model.position[i]))return 0;}
    for(unsigned i=0;i<4;++i){w=u32(r+0x18+i*4);float current;memcpy(&current,&w,4);if(current!=ball_rotation[i]||!isfinite(model.quaternion[i]))return 0;}
    memcpy(r+0x28,r+12,12);attachment_initialized[attachment_actor][side]=1;++attachment_updates;
   }
  }
  for(unsigned i=0;i<3;++i){if(!attachment_poses)attachment_first[side][i]=attachment_min[side][i]=attachment_max[side][i]=point[i];
   if(fabsf(point[i]-attachment_first[side][i])>0.0001f)attachment_changed=1;
   if(point[i]<attachment_min[side][i])attachment_min[side][i]=point[i];if(point[i]>attachment_max[side][i])attachment_max[side][i]=point[i];}
 }
 ++attachment_poses;return 1;
}
static int actor_pose_raw(const int16_t *raw,unsigned n,const float *eye){
 if(!attachment_pose(raw,n,eye))return 0;
 for(unsigned id=0;id<model_count;++id){component *m=models+id;int16_t rotations[768];for(unsigned j=0;j<m->bones*3;++j)rotations[j]=m->map[j]>=0&&(unsigned)m->map[j]<n?raw[m->map[j]]:m->stored[j];
 if(!nflstreet_pose_clusters(m->b,m->n,rotations,m->bones*3,m->matrix,&m->clusters))return 0;
 if(m->clusters==1)memcpy(m->matrix[0],models[0].matrix[neck],sizeof m->matrix[0]);
 }
 for(unsigned i=0;i<draw_count;++i){uint8_t *r=profile+16+(size_t)i*RECORD;component *m=models+model_ids[i];
 for(unsigned j=0;j<64;++j)if(selectors[i][j]>=0){if((unsigned)selectors[i][j]>=m->clusters)return 0;float transformed[12];concat(eye,m->matrix[selectors[i][j]],transformed);
 for(unsigned k=0;k<12;++k)put(r+256+j*16+k*4,transformed[k]);for(unsigned row=0;row<3;++row)for(unsigned col=0;col<3;++col)put(r+1312+(j&31)*12+row*12+col*4,transformed[row*4+col]);}}
 return nflstreet_player_repose(&player,profile,profile_n,sources);
}
static int actor_pose_frames(unsigned a,unsigned b,unsigned weight,const float *eye){int16_t first[384],second[384],raw[384];unsigned n,n2;
 if(!nflstreet_pose_frame(bank,bank_n,a,first,384,&n)||!nflstreet_pose_frame(bank,bank_n,b,second,384,&n2)||n!=n2||
    !nflstreet_pose_mix(raw,first,second,n,weight))return 0;
 return actor_pose_raw(raw,n,eye);}
static int actor_pose(const clip *c,float clock,const float *eye){unsigned a=(unsigned)floorf(clock)%c->count,b=(a+1)%c->count;
 return actor_pose_frames(c->frames[a],c->frames[b],(unsigned)((clock-floorf(clock))*4096),eye);}
static int field_open(const char *archive_path,const char *profile_path){size_t an,pn;uint8_t *ab=readfile(archive_path,&an),*pb=readfile(profile_path,&pn),*owned[6]={0};nflstreet_mesh meshes[6]={0},sky={0};nflstreet_field_draw *draws=NULL;int ok=0;
 if(!ab||!pb||pn<168||memcmp(pb,"NFR2",4)||memcmp(pb+40,"NSK1",4))goto done;unsigned count=le32(pb+4);if(!count||count>20000||pn!=168+(size_t)count*208)goto done;
 float cfg[31];for(unsigned i=0;i<31;++i){uint32_t w=le32(pb+44+i*4);memcpy(cfg+i,&w,4);}if(!nflstreet_sky_mesh(cfg,&sky))goto done;
 nflstreet_archive archive;if(!nflstreet_archive_open(ab,an,&archive))goto done;for(unsigned i=0;i<6;++i){nflstreet_archive_entry e;size_t n;if(!nflstreet_archive_get(&archive,le32(pb+16+i*4),&e)||!nflstreet_archive_decode(&e,owned+i,&n)||!nflstreet_mesh_open_render(owned[i],n,meshes+i))goto done;}
 draws=calloc(count,sizeof *draws);if(!draws)goto done;int got_camera=0;for(unsigned i=0;i<count;++i){const uint8_t *r=pb+168+(size_t)i*208;nflstreet_field_draw *d=draws+i;for(unsigned j=0;j<16;++j)d->state[j]=le32(r+j*4);
 float *arrays[]={d->camera,d->projection,d->viewport,d->texmatrix};unsigned sizes[]={12,6,6,12},at=16;for(unsigned j=0;j<4;++j)for(unsigned k=0;k<sizes[j];++k){uint32_t w=le32(r+at++*4);memcpy(arrays[j]+k,&w,4);}
 if(!got_camera&&d->state[0]<6){memcpy(camera,d->camera,sizeof camera);got_camera=1;}}
 nflstreet_field_stats stats;ok=got_camera&&nflstreet_field_prepare(meshes,&sky,draws,count,le32(pb+8),le32(pb+12),&field,&stats);
 if(ok)printf("Cached original field and sky: %u draws, %zu shaded samples\n",stats.commands,stats.shaded);
 if(ok&&check){float world[12]={1,0,0,0,0,0,-1,-20,0,1,0,1.1593527793884277f},eye[12];concat(camera,world,eye);
 nflstreet_texture combined={0},overlay={0};nflstreet_field_draw *all=calloc(count+player.count,sizeof *all);
 if(!all||!actor_pose(&idle,0,eye)){free(all);ok=0;goto done;}memcpy(all,draws,count*sizeof *all);memcpy(all+count,player.draws,player.count*sizeof *all);
 ok=nflstreet_field_render_player(meshes,&sky,&player.mesh,all,count+player.count,field.image.width,field.image.height,&combined,&stats)&&
 nflstreet_field_overlay(&field,&player.mesh,player.draws,player.count,&overlay,&stats)&&!memcmp(combined.bgra,overlay.bgra,(size_t)field.image.width*field.image.height*4);
 nflstreet_texture_close(&combined);nflstreet_texture_close(&overlay);free(all);if(ok)puts("Cached field color/depth plus posed actor matches combined scene render exactly");}
 done:for(unsigned i=0;i<6;++i){nflstreet_mesh_close(meshes+i);free(owned[i]);}nflstreet_mesh_close(&sky);free(draws);free(ab);free(pb);return ok;
}
static float word_float(uint32_t w){float f;memcpy(&f,&w,4);return f;}
/* Two independent instances of the checked reference player. This does not
   assign the duplicate appearance to an original roster slot or simulate contacts. */
static int second_frame(float dt,nflstreet_field_surface *first){
 nflstreet_live_motion motion;nflstreet_live_movement_observe(&second_movement,&motion);
 second_clock+=dt;int dx=(int)keys['L']-(int)keys['J'],dy=(int)keys['I']-(int)keys['K'];
 while(second_clock>=1.0/60.0){second_clock-=1.0/60.0;float px=motion.x,py=motion.y;
  if(nflstreet_live_movement_step(&second_movement,dx,dy,&motion)<0)return 0;
  second_traveled+=hypotf(motion.x-px,motion.y-py);
 }
 if(motion.speed_per_update>0)++second_moving_frames;
 nflstreet_mesh joined=player.mesh;joined.triangles=NULL;nflstreet_field_draw *draws=NULL;
 if(check){joined.count*=2;joined.triangles=malloc(joined.count*sizeof *joined.triangles);draws=malloc(2*player.count*sizeof *draws);
  if(!joined.triangles||!draws){free(joined.triangles);free(draws);return 0;}
  memcpy(joined.triangles,player.mesh.triangles,player.mesh.count*sizeof *joined.triangles);memcpy(draws,player.draws,player.count*sizeof *draws);
 }
 float cs=cosf(motion.facing),ss=sinf(motion.facing);
 float world[12]={cs,0,ss,1.5f+motion.x,ss,0,-cs,-18+motion.y,0,1,0,1.1593527793884277f},eye[12];concat(camera,world,eye);
 nflstreet_curve_inputs input={motion.speed_per_update,word_float(second_movement.runtime.state[13]),word_float(second_movement.runtime.state[16]),word_float(second_movement.runtime.state[17]),(uint8_t)(second_movement.runtime.actor[7]>>8)};
 int16_t raw[384];unsigned components;const uint8_t*selected;size_t selected_n;
 memcpy(ownership_world,world,sizeof ownership_world);attachment_actor=1;
 int ok=nflstreet_live_locomotion_catalog_get(set_catalog,set_catalog_n,0,&selected,&selected_n)&&
  nflstreet_live_locomotion_step_set(&second_locomotion,selected,selected_n,&input,second_movement.runtime.state[6],second_movement.runtime.state[8],(double)dt*60,raw,384,&components)&&actor_pose_raw(raw,components,eye);
 nflstreet_field_surface scene={0};nflstreet_field_stats stats;
 if(ok){ok=nflstreet_field_layer(first,&player.mesh,player.draws,player.count,&scene,&stats);if(ok&&stats.shaded)++second_visible_frames;}
 if(ok&&check){
  memcpy(joined.triangles+player.mesh.count,player.mesh.triangles,player.mesh.count*sizeof *joined.triangles);memcpy(draws+player.count,player.draws,player.count*sizeof *draws);
  /* Host-only disjoint draw identifiers for the two copies in the test mesh. */
  for(size_t i=player.mesh.count;i<joined.count;++i)joined.triangles[i].source_offset^=0x80000000;
  for(unsigned i=player.count;i<2*player.count;++i)draws[i].state[1]^=0x80000000;
  nflstreet_field_surface combined={0};
  ok=nflstreet_field_layer(&field,&joined,draws,2*player.count,&combined,&stats)&&
   !memcmp(scene.image.bgra,combined.image.bgra,(size_t)scene.image.width*scene.image.height*4)&&
   !memcmp(scene.depth,combined.depth,(size_t)scene.image.width*scene.image.height*sizeof *scene.depth);
  nflstreet_field_surface_close(&combined);if(ok)++scene_checks;
 }
 free(joined.triangles);free(draws);
 if(ok)ok=ball_layer(&scene);
 if(ok){nflstreet_texture_close(&image);image=scene.image;scene.image.bgra=NULL;}
 nflstreet_field_surface_close(&scene);return ok;
}
static int contact_frame(float dt){
 int16_t frames[2][384];unsigned count;unsigned completed=contact.completions;
 if(!nflstreet_live_contact_step(&contact,(double)dt*60,frames,&count)||count!=3)return 0;
 nflstreet_field_surface scene={0};nflstreet_field_stats stats;
 float anchor_x=word_float(u32(contact.actor[0]+0x1a8)),anchor_y=word_float(u32(contact.actor[0]+0x1ac));
 for(unsigned i=0;i<2;++i){float px=word_float(u32(contact.actor[i]+0x1a8)),py=word_float(u32(contact.actor[i]+0x1ac));unsigned heading=u32(contact.actor[i]+0x1c0);
  for(unsigned j=0;j<4;++j){const uint8_t*g=contact.actor[i]+0x9bc+j*100;if(u16(g+4)==30&&u16(g+6)){const uint8_t*d=g+16;if(d[0x1c]){px=word_float(u32(d+4));py=word_float(u32(d+8));heading=u32(d+0x14);}break;}}
  float angle=(float)(heading&0xffffff)*(6.2831853071795864769f/16777216.0f)-1.5707963267948966f,cs=cosf(angle),ss=sinf(angle);
  float world[12]={cs,0,ss,x+px-anchor_x,ss,0,-cs,y+py-anchor_y,0,1,0,1.1593527793884277f},eye[12];concat(camera,world,eye);
  memcpy(ownership_world,world,sizeof ownership_world);attachment_actor=i;
  if(!actor_pose_frames((uint16_t)frames[i][0],(uint16_t)frames[i][1],(unsigned)frames[i][2],eye)){nflstreet_field_surface_close(&scene);return 0;}
  nflstreet_field_surface next={0};if(!nflstreet_field_layer(i?&scene:&field,&player.mesh,player.draws,player.count,&next,&stats)){nflstreet_field_surface_close(&scene);return 0;}
  nflstreet_field_surface_close(&scene);scene=next;
 }
 if(!ball_layer(&scene)){nflstreet_field_surface_close(&scene);return 0;}
 nflstreet_texture_close(&image);image=scene.image;scene.image.bgra=NULL;nflstreet_field_surface_close(&scene);
 if(contact.completions!=completed){movement_clock=second_clock=0;memset(keys,0,sizeof keys);}
 return 1;
}
static int frame(float dt){attachment_actor=0;if(!airborne_frame(dt))return 0;if(contact.active)return contact_frame(dt);float dx=(keys['D']||keys[VK_RIGHT])-(int)(keys['A']||keys[VK_LEFT]);float dy=(keys['W']||keys[VK_UP])-(int)(keys['S']||keys[VK_DOWN]);
 float length=sqrtf(dx*dx+dy*dy);int moving=length>0;
 if(native_mode){nflstreet_live_motion motion;nflstreet_live_movement_observe(&movement,&motion);
 movement_status=1;
 if(movement_status==1){movement_clock+=dt;while(movement_clock>=1.0/60.0){movement_clock-=1.0/60.0;
 float prior_x=motion.x,prior_y=motion.y;movement_status=nflstreet_live_movement_step(&movement,(int)dx,(int)dy,&motion);
 if(movement_status<0){movement_clock=0;break;}traveled+=hypotf(motion.x-prior_x,motion.y-prior_y);}}
 else movement_clock=0;
 x=motion.x;y=-20+motion.y;facing=motion.facing;moving=movement_status>=0&&motion.speed_per_update>0;if(movement_status>=0&&!moving)movement_status=0;if(moving)++moving_frames;
 }else {if(moving){dx/=length;dy/=length;x+=dx*5*dt;y+=dy*5*dt;traveled+=5*dt;facing=atan2f(-dx,dy);++moving_frames;}
 if(x<-10)x=-10;if(x>10)x=10;if(y<-26)y=-26;if(y>12)y=12;}
 phase+=dt*(moving?16:12);const clip *c=moving?&run:&idle;phase=fmodf(phase,(float)c->count);
 float cs=cosf(facing),ss=sinf(facing);/* model Y up -> field Z up, model -Z -> forward */
 float world[12]={cs,0,ss,x,ss,0,-cs,y,0,1,0,1.1593527793884277f},eye[12];concat(camera,world,eye);
 memcpy(ownership_world,world,sizeof ownership_world);
 if(native_mode&&locomotion_profile){int16_t raw[384];unsigned components;nflstreet_live_motion motion;nflstreet_live_movement_observe(&movement,&motion);
  /* Original curve and distinct pair/blend sign choices consume carried
     movement inputs. Upstream variant service decisions remain unfinished. */
  nflstreet_curve_inputs input={motion.speed_per_update,word_float(movement.runtime.state[13]),word_float(movement.runtime.state[16]),word_float(movement.runtime.state[17]),(uint8_t)(movement.runtime.actor[7]>>8)};
  if(set_catalog){const uint8_t*selected;size_t selected_n;
   if(!nflstreet_live_locomotion_catalog_get(set_catalog,set_catalog_n,requested_set,&selected,&selected_n)||!nflstreet_live_locomotion_step_set(&locomotion,selected,selected_n,&input,movement.runtime.state[6],movement.runtime.state[8],(double)dt*60,raw,384,&components))return 0;
  }else if(!nflstreet_live_locomotion_step_direction(&locomotion,&input,movement.runtime.state[6],movement.runtime.state[8],(double)dt*60,raw,384,&components))return 0;
  if(!actor_pose_raw(raw,components,eye))return 0;
 }else if(native_mode){uint16_t commands[64];size_t emitted;unsigned complete;
  const uint8_t *program=moving?run_program:idle_program;size_t program_n=moving?run_program_n:idle_program_n;unsigned identity=moving?run_identity:idle_identity;
  if(animation_selection!=moving){memset(animation_stream,0,sizeof animation_stream);
   if(!nflstreet_stream_setup(animation_stream,identity,program,program_n,&animation_flags,commands,&emitted,&complete)||complete)return 0;animation_selection=moving;}
  /* Host mapping: one stream unit per 60 Hz update. Selection and wall-clock
     mapping remain inspection choices, while tag/countdown logic is original. */
  if(!nflstreet_command_timing(animation_stream,program,program_n,(double)dt*60,&animation_flags,commands,&emitted,&complete)||complete)return 0;
  uint32_t bits=u32(animation_stream+20);float fraction;memcpy(&fraction,&bits,4);
  if(!isfinite(fraction)||fraction<0||fraction>1||!actor_pose_frames(u16(animation_stream+16),u16(animation_stream+18),(unsigned)(fraction*4096),eye))return 0;
 }else {animation_selection=-1;if(!actor_pose(c,phase,eye))return 0;}nflstreet_field_stats stats;if(second_mode&&native_mode){nflstreet_field_surface first={0};
 if(!nflstreet_field_layer(&field,&player.mesh,player.draws,player.count,&first,&stats))return 0;
 int ok=second_frame(dt,&first);nflstreet_field_surface_close(&first);return ok;
 }if(ball_profile){nflstreet_field_surface scene={0};int ok=nflstreet_field_layer(&field,&player.mesh,player.draws,player.count,&scene,&stats)&&ball_layer(&scene);
  if(ok){nflstreet_texture_close(&image);image=scene.image;scene.image.bgra=NULL;}nflstreet_field_surface_close(&scene);return ok;
 }nflstreet_texture_close(&image);return nflstreet_field_overlay(&field,&player.mesh,player.draws,player.count,&image,&stats);
}
static void save(const char *name){char path[4096];snprintf(path,sizeof path,"%s/%s.bmp",output,name);if(!nflstreet_texture_bmp(&image,path))failed=1;}
static LRESULT CALLBACK proc(HWND window,UINT message,WPARAM a,LPARAM b){
 if(message==WM_DESTROY){PostQuitMessage(0);return 0;}
 if(message==WM_KILLFOCUS){memset(keys,0,sizeof keys);return 0;}
 if(message==WM_KEYDOWN||message==WM_KEYUP){if(a<256)keys[a]=message==WM_KEYDOWN;if(message==WM_KEYDOWN&&a==VK_ESCAPE)DestroyWindow(window);if(message==WM_KEYDOWN&&a=='B'&&!(b&(1L<<30))&&ownership_mode){if(!ownership_request())failed=1;}if(message==WM_KEYDOWN&&a=='V'&&!(b&(1L<<30))&&set_catalog)requested_set=(requested_set+1)%u32(set_catalog+4);if(message==WM_KEYDOWN&&a=='C'&&!(b&(1L<<30))&&contact_pair_path&&!contact.active){unsigned completed=contact.completions;contact=contact_seed;contact.completions=completed;++contact_starts;memset(keys,0,sizeof keys);}if(message==WM_KEYDOWN&&a=='R'){if(ownership_mode)ownership_pending=0;contact.active=0;if(second_mode){nflstreet_live_movement_reset(&second_movement);nflstreet_live_locomotion_reset(&second_locomotion);second_clock=0;}requested_set=0;x=0;y=-20;phase=facing=0;movement_clock=0;animation_selection=-1;animation_flags=0;if(movement_fixture)nflstreet_live_movement_reset(&movement);if(locomotion_profile)nflstreet_live_locomotion_reset(&locomotion);}
 if(message==WM_KEYDOWN&&a=='M'&&!(b&(1L<<30))&&movement_fixture&&!second_mode){native_mode=!native_mode;requested_set=0;x=0;y=-20;phase=facing=0;movement_clock=0;animation_selection=-1;animation_flags=0;nflstreet_live_movement_reset(&movement);if(locomotion_profile)nflstreet_live_locomotion_reset(&locomotion);}return 0;}
 if(message==WM_TIMER){LARGE_INTEGER now;QueryPerformanceCounter(&now);float dt=(float)((double)(now.QuadPart-last.QuadPart)/frequency.QuadPart);last=now;if(dt>0.05f)dt=0.05f;
 if(ownership_check&&ticks&&ticks%(contact_check?32u:80u)==0&&ticks/(contact_check?32u:80u)<=6){if(!ownership_request())failed=1;}
 if(ownership_check&&!airborne_mode&&ownership_releases&&!u32(ownership_record+0xb4)){if(memcmp(ownership_record,ownership_frozen,sizeof ownership_record))failed=1;else ++ownership_frozen_checks;}
 if(check){if(native_mode){dt=1.0f/60;const WPARAM released[]={'W','A','S','D',VK_UP,VK_LEFT,VK_DOWN,VK_RIGHT};
 for(unsigned k=0;k<sizeof released/sizeof *released;++k)if(keys[released[k]])SendMessageA(window,WM_KEYUP,released[k],0);unsigned n=u32(movement_fixture+4);
 if(direction_check){if(ticks>=48){unsigned segment=(ticks-48)/60,step=(ticks-48)%60;
 static const int directions[8][2]={{1,0},{0,1},{-1,0},{0,-1},{1,1},{-1,1},{-1,-1},{1,-1}};
 if(segment<8&&step<8){int dx=directions[segment][0],dy=directions[segment][1];
 if(dx)SendMessageA(window,WM_KEYDOWN,segment&1?(dx>0?VK_RIGHT:VK_LEFT):(dx>0?'D':'A'),0);
 if(dy)SendMessageA(window,WM_KEYDOWN,segment&1?(dy>0?VK_UP:VK_DOWN):(dy>0?'W':'S'),0);}}}
 else if(!braking_fixture&&ticks<n){const uint8_t *r=movement_fixture+152+(size_t)ticks*164;WPARAM key=u32(r+4)==0xbf800000?'A':u32(r+4)==0x3f800000?'D':'W';SendMessageA(window,WM_KEYDOWN,key,0);}
 else if(!braking_fixture&&ticks==n){SendMessageA(window,WM_KILLFOCUS,0,0);}else if(ticks==n+1)SendMessageA(window,WM_KILLFOCUS,0,0);
 }else {dt=1.0f/30;if(ticks==0){SendMessageA(window,WM_KEYDOWN,'D',0);save("start");}if(ticks==15){save("right");SendMessageA(window,WM_KEYUP,'D',0);SendMessageA(window,WM_KEYDOWN,'W',0);}if(ticks==30){save("forward");SendMessageA(window,WM_KEYDOWN,'A',0);}if(ticks==45){save("diagonal");SendMessageA(window,WM_KILLFOCUS,0,0);}}}
 if(contact_check&&(ticks==10||ticks==12||ticks==90)){SendMessageA(window,WM_KEYDOWN,'C',0);SendMessageA(window,WM_KEYUP,'C',0);}
 if(set_check&&(ticks==10||ticks==12||ticks==24||ticks==40)){SendMessageA(window,WM_KEYDOWN,'V',0);SendMessageA(window,WM_KEYUP,'V',0);}
 if(check&&second_mode&&ticks==0){SendMessageA(window,WM_KEYDOWN,'R',0);SendMessageA(window,WM_KEYUP,'R',0);
  if(memcmp(&movement.runtime,&movement.seed,sizeof movement.runtime)||memcmp(&second_movement.runtime,&second_movement.seed,sizeof second_movement.runtime)||locomotion.initialized||second_locomotion.initialized||movement_clock||second_clock)failed=1;
 }
 if(check&&second_mode){const WPARAM released[]={'I','J','K','L'};for(unsigned i=0;i<4;++i)if(keys[released[i]])SendMessageA(window,WM_KEYUP,released[i],0);
 if(direction_check&&ticks>=48&&(ticks-48)%60<8){unsigned segment=(ticks-48)/60;static const int directions[8][2]={{-1,0},{0,-1},{1,0},{0,1},{-1,-1},{1,-1},{1,1},{-1,1}};
  if(segment<8){int dx=directions[segment][0],dy=directions[segment][1];if(dx)SendMessageA(window,WM_KEYDOWN,dx>0?'L':'J',0);if(dy)SendMessageA(window,WM_KEYDOWN,dy>0?'I':'K',0);}}
 else if(!direction_check&&ticks<8)SendMessageA(window,WM_KEYDOWN,'J',0);
 }
 LARGE_INTEGER start,end;QueryPerformanceCounter(&start);if(!frame(dt)){failed=1;DestroyWindow(window);return 0;}QueryPerformanceCounter(&end);float ms=(float)((double)(end.QuadPart-start.QuadPart)*1000/frequency.QuadPart);if(ms>max_step_ms)max_step_ms=ms;
 if(check&&!direction_check&&native_mode&&ticks<u32(movement_fixture+4)){const uint8_t *r=movement_fixture+152+(size_t)ticks*(braking_fixture?132:164);
 unsigned actor_offset=braking_fixture?0:32,state_offset=braking_fixture?48:80;
 for(unsigned j=0;j<12;++j)if(movement.runtime.actor[j]!=u32(r+actor_offset+j*4))failed=1;
 for(unsigned j=0;j<21;++j)if(movement.runtime.state[j]!=u32(r+state_offset+j*4))failed=1;
 if(movement.runtime.completed!=ticks+1)failed=1;
 nflstreet_live_motion observed;nflstreet_live_movement_observe(&movement,&observed);char name[32];snprintf(name,sizeof name,"movement-%03u",ticks);save(name);
 printf("Rendered original checkpoint %u: x=%.9g y=%.9g facing=%.9g speed=%.9g full-blocks=%s\n",ticks,x,y,facing,observed.speed_per_update,failed?"FAILED":"exact");}
 if(direction_check){if(movement_status<0)failed=1;
 if(ticks>=48&&(ticks-48)%60==7&&(!movement.runtime.state[7]||(second_mode&&!second_movement.runtime.state[7])))failed=1;if(ticks>=48&&(ticks-48)%60==59){
 if(movement.runtime.state[7]||movement.runtime.state[10]||movement.runtime.state[11])failed=1;
 if(second_mode&&(second_movement.runtime.state[7]||second_movement.runtime.state[10]||second_movement.runtime.state[11]))failed=1;
 char name[32];snprintf(name,sizeof name,"direction-%u",(ticks-48)/60);save(name);
 printf("Windows direction %u start, key-up braking and held stop: %s\n",(ticks-48)/60,failed?"FAILED":"passed");}}
 if((set_check||contact_check)&&(ticks==16||ticks==32||ticks==48||(contact_check&&(ticks==112||ticks==144)))){char name[32];snprintf(name,sizeof name,"set-%02u",ticks);save(name);}
 ++ticks;char title[256];snprintf(title,sizeof title,"NFL Street native player - %s - %s",native_mode?(movement_status<0?"movement branch unsupported; paused":movement_status==0?"stopped":(!keys['W']&&!keys['A']&&!keys['S']&&!keys['D']&&!keys[VK_UP]&&!keys[VK_LEFT]&&!keys[VK_DOWN]&&!keys[VK_RIGHT]?"reconstructed braking":"reconstructed acceleration/turning")):"inspection movement",second_mode?(contact_pair_path?"WASD/arrows player 1, IJKL player 2, C contact inspection, V set, R reset, Esc":"WASD/arrows player 1, IJKL player 2, V set, R reset, Esc"):"M mode, WASD/arrows, V animation set, R reset, Esc close");if(ownership_mode)snprintf(title,sizeof title,"NFL Street native inspection - WASD/IJKL, C contact, V set, B owner/release (%s), R reset, Esc",airborne_mode?"airborne; host plane stop":"frozen release");SetWindowTextA(window,title);InvalidateRect(window,NULL,FALSE);
 if(check&&ticks==(contact_check?200u:direction_check?528u:60u)){if(contact_check&&(contact.active||contact_starts!=2||contact.completions!=2))failed=1;if(second_mode&&(!second_moving_frames||!second_visible_frames||!second_traveled||second_locomotion.set_changes||!second_locomotion.completed_fades||second_locomotion.descriptor[0x3b]||second_locomotion.descriptor[2]!=255||second_locomotion.descriptor[3]!=255||(!contact_check&&scene_checks!=ticks+1)||second_movement.runtime.state[7]||second_movement.runtime.state[10]||second_movement.runtime.state[11]||keys['I']||keys['J']||keys['K']||keys['L']))failed=1;if(set_check&&(locomotion.set_changes<2||locomotion.pending_set||locomotion.set_keys[0]!=set_catalog[18]||locomotion.set_keys[1]!=set_catalog[19]))failed=1;if(native_mode&&!locomotion_profile&&!(animation_flags&0x40000))failed=1;if(native_mode&&locomotion_profile&&(!locomotion.updates||locomotion.changes<2||!locomotion.completed_fades||locomotion.descriptor[0x3b]||locomotion.descriptor[2]!=255||locomotion.descriptor[3]!=255))failed=1;save("stopped");if(native_mode){if(movement.runtime.state[7]||movement.runtime.state[10]||movement.runtime.state[11]||keys['W']||keys['D'])failed=1;}
 else if(fabsf(x-0.732233f)>0.001f||fabsf(y+15.732233f)>0.001f||moving_frames!=45||keys['W']||keys['A'])failed=1;DestroyWindow(window);}return 0;}
 if(message==WM_PAINT){PAINTSTRUCT ps;HDC dc=BeginPaint(window,&ps);RECT r;GetClientRect(window,&r);BITMAPINFO bi={0};bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=(LONG)image.width;bi.bmiHeader.biHeight=-(LONG)image.height;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;
 if(image.bgra&&StretchDIBits(dc,0,0,r.right,r.bottom,0,0,(int)image.width,(int)image.height,image.bgra,&bi,DIB_RGB_COLORS,SRCCOPY)>0)++paints;EndPaint(window,&ps);return 0;}
 return DefWindowProcA(window,message,a,b);
}
int main(int argc,char **argv){if(argc<6){fprintf(stderr,"Usage: live-player unpacked-root player-profile envrnmt.dat field-profile output-directory [--movement private-fixture] [--locomotion bank source-curves | --sets bank catalog] [--second-player] [--attachment-profile private-NAP1-NAP2-or-NAP3] [--ball-profile private-draw-profile] [--ownership-inspection] [--render-check]\n");return 2;}
 output=argv[5];for(int arg=6;arg<argc;++arg){if(!strcmp(argv[arg],"--render-check"))check=1;
 else if(!strcmp(argv[arg],"--set-check"))set_check=check=1;
 else if(!strcmp(argv[arg],"--direction-check"))direction_check=check=1;
 else if(!strcmp(argv[arg],"--movement")&&arg+1<argc){movement_fixture=readfile(argv[++arg],&movement_fixture_n);if(!movement_fixture||!nflstreet_live_movement_open(movement_fixture,movement_fixture_n,&movement))return 2;native_mode=1;braking_fixture=!memcmp(movement_fixture,"NBS1",4);}
 else if(!strcmp(argv[arg],"--contact-check"))contact_check=check=1;
 else if(!strcmp(argv[arg],"--contact")&&arg+2<argc){contact_selection_path=argv[++arg];contact_pair_path=argv[++arg];}
 else if(!strcmp(argv[arg],"--ownership-inspection")){ownership_mode=1;}
 else if(!strcmp(argv[arg],"--ball-frame-inspection")){ball_frame_mode=1;}
 else if(!strcmp(argv[arg],"--airborne-inspection")){ownership_mode=airborne_mode=1;}
 else if(!strcmp(argv[arg],"--airborne-check")){airborne_mode=ownership_mode=ownership_check=direction_check=check=1;}
 else if(!strcmp(argv[arg],"--ownership-check")){ownership_mode=ownership_check=direction_check=check=1;}
 else if(!strcmp(argv[arg],"--ball-profile")&&arg+1<argc){ball_profile_path=argv[++arg];}
 else if(!strcmp(argv[arg],"--attachment-profile")&&arg+1<argc){attachment_profile=readfile(argv[++arg],&attachment_profile_n);
  if(!attachment_profile||!((attachment_profile_n==24&&!memcmp(attachment_profile,"NAP1",4))||(attachment_profile_n==36&&!memcmp(attachment_profile,"NAP2",4))||(attachment_profile_n==88&&!memcmp(attachment_profile,"NAP3",4))))return 2;}
 else if(!strcmp(argv[arg],"--second-player"))second_mode=1;
 else if(!strcmp(argv[arg],"--sets")&&arg+2<argc){locomotion_bank=readfile(argv[++arg],&locomotion_bank_n);set_catalog=readfile(argv[++arg],&set_catalog_n);const uint8_t*selected;size_t selected_n;
  if(!nflstreet_live_locomotion_catalog_get(set_catalog,set_catalog_n,0,&selected,&selected_n)||!nflstreet_live_locomotion_open(&locomotion,locomotion_bank,locomotion_bank_n,selected,selected_n))return 2;
  locomotion_profile=(uint8_t*)selected;locomotion_profile_n=selected_n;}
 else if(!strcmp(argv[arg],"--locomotion")&&arg+2<argc){locomotion_bank=readfile(argv[++arg],&locomotion_bank_n);locomotion_profile=readfile(argv[++arg],&locomotion_profile_n);
  if(!nflstreet_live_locomotion_open(&locomotion,locomotion_bank,locomotion_bank_n,locomotion_profile,locomotion_profile_n))return 2;}
 else return 2;}
 if(ownership_check&&contact_check)direction_check=0;
 if(ball_frame_mode&&!airborne_mode)return 2;
 if(set_check&&!set_catalog)return 2;
 if(contact_check&&(!contact_pair_path||set_check||direction_check))return 2;
 if(contact_pair_path&&!second_mode)return 2;
 if(second_mode){if(!set_catalog||!movement_fixture)return 2;second_movement=movement;second_locomotion=locomotion;}
 if((direction_check||locomotion_profile)&&!movement_fixture)return 2;
 if(movement_fixture&&check){unsigned updates;if(!(braking_fixture?nflstreet_live_braking_check(movement_fixture,movement_fixture_n,&updates):nflstreet_live_movement_check(movement_fixture,movement_fixture_n,&updates)))return 1;printf("Semantic movement adapter matches %u full original actor/movement checkpoints; unsupported/reset guards pass\n",updates);}
 int status=1;if(ball_profile_path&&(!attachment_profile||attachment_profile_n<36))goto done;
 if(ownership_mode&&(!second_mode||!ball_profile_path||attachment_profile_n!=88))goto done;
 if(!actor_open(argv[1],argv[2])||!field_open(argv[3],argv[4])||!ball_open(argv[1]))goto done;
 if(contact_pair_path&&!nflstreet_live_contact_open(&contact_seed,contact_selection_path,contact_pair_path,bank,bank_n))goto done;
 if(!frame(0))goto done;
 HINSTANCE instance=GetModuleHandleA(NULL);WNDCLASSA cls={0};cls.lpfnWndProc=proc;cls.hInstance=instance;cls.lpszClassName="NFLStreetLivePlayer";cls.hCursor=LoadCursor(NULL,IDC_ARROW);if(!RegisterClassA(&cls))goto done;
 HWND window=CreateWindowA(cls.lpszClassName,ownership_mode?"NFL Street ownership inspection - B owner/release, release freezes ball, WASD/IJKL movement, C contact, R reset, Esc":second_mode?(contact_pair_path?"NFL Street native players - WASD/arrows player 1, IJKL player 2, C contact inspection, V set, R reset, Esc":"NFL Street native players - WASD/arrows player 1, IJKL player 2, V set, R reset, Esc"):"NFL Street native player - M switches movement mode, WASD/arrows, V animation set, R reset, Esc close",WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,1000,740,NULL,NULL,instance,NULL);if(!window)goto done;
 QueryPerformanceFrequency(&frequency);QueryPerformanceCounter(&last);ShowWindow(window,SW_SHOW);UpdateWindow(window);if(!SetTimer(window,1,33,NULL)){DestroyWindow(window);goto done;}
 MSG msg;int result;while((result=GetMessageA(&msg,NULL,0,0))>0){TranslateMessage(&msg);DispatchMessageA(&msg);}status=result==0&&!failed&&paints>0?0:1;
 if(native_mode&&locomotion_profile)printf("Native source-curve paired playback: updates=%u pair-starts=%u fade-updates=%u completed-fades=%u clips=%04x,%04x rate=%.9g (source curves; phase preserved; bounded four-channel fade)\n",locomotion.updates,locomotion.changes,locomotion.fade_updates,locomotion.completed_fades,locomotion.clips[0],locomotion.clips[1],locomotion.rate);
 if(native_mode&&locomotion_profile)printf("Original directional playback: updates=%u negative-pair-updates=%u negative-blend-updates=%u curve-swaps=%u\n",locomotion.direction_updates,locomotion.negative_pair_updates,locomotion.negative_blend_updates,locomotion.curve_changes);
 else if(native_mode)printf("Native animation streams: accepted idle C00C flag=%08x\n",animation_flags);
 if(attachment_profile){if(check&&(!attachment_poses||!attachment_changed))status=1;
  printf("Native compact attachment diagnostics: poses=%u animated-joints=%s right-x-range=%.9g left-x-range=%.9g orientation-samples=%u attached-record-updates=%u (decoded compact poses; camera-space points finite; source local offset; root/extra-bone scheduling provisional)\n",attachment_poses,attachment_changed?"changed":"static",attachment_max[0][0]-attachment_min[0][0],attachment_max[1][0]-attachment_min[1][0],attachment_rotations,attachment_updates);}
 printf("Live native viewer: ticks=%u paints=%u moving=%u x=%.6f y=%.6f traveled=%.6f max-frame-ms=%.3f check=%s\n",ticks,paints,moving_frames,x,y,traveled,max_step_ms,status?"FAILED":"passed");
 if(set_catalog)printf("Supported set playback: changes=%u keys=%02x/%02x requested=%u (inspection selection)\n",locomotion.set_changes,locomotion.set_keys[0],locomotion.set_keys[1],requested_set);
 if(contact_pair_path)printf("Native contact inspection: starts=%u completed=%u callbacks-last=%u clips=%04x/%04x (fixed verified context; return control is host scheduling)\n",contact_starts,contact.completions,contact.callbacks,contact.clips[0],contact.clips[1]);
 if(ball_frame_mode){if(ownership_check&&(!ball_frame_updates||ball_frame_history_checks!=ball_frame_updates||ball_frame_queries!=ball_frame_updates))status=1;printf("Native original ball frame:updates=%u full-history/clear checks=%u spatial-requests=%u (supplied force/association; environment query retained, launch/ground unfinished)\n",ball_frame_updates,ball_frame_history_checks,ball_frame_queries);}
 if(airborne_mode){if(ownership_check&&(!airborne_updates||!airborne_moved))status=1;printf("Native airborne inspection: updates=%u moving=%u plane-pauses=%u (supplied force/time/no-spin context; host plane stop; launch/collision policy unfinished)\n",airborne_updates,airborne_moved,airborne_pauses);}
 if(ownership_mode){if(ownership_check&&(ownership_assignments!=6||ownership_releases!=2||(!airborne_mode&&!ownership_frozen_checks)||!ownership_updates[0]||!ownership_updates[1]||u32(ownership_record+0xb4)!=ownership_headers[0]))status=1;
 printf("Native ownership inspection: assignments=%u releases=%u player-updates=%u/%u frozen-release-checks=%u owner=%08x (B cycles owners/release; %s; eligibility unfinished)\n",ownership_assignments,ownership_releases,ownership_updates[0],ownership_updates[1],ownership_frozen_checks,u32(ownership_record+0xb4),airborne_mode?"bounded airborne inspection":"release pose frozen");}
 if(ball_profile){if(check&&(!ball_frames||!ball_shaded))status=1;printf("Native football inspection: frames=%u shaded=%zu (source GameBall mesh/texture; inspection attachment; gameplay eligibility unfinished)\n",ball_frames,ball_shaded);}
 if(second_mode)printf("Second native player: moving=%u traveled=%.9g animation-updates=%u completed-fades=%u independent-scene-checks=%u visible-frames=%u (shared reference appearance; contacts unfinished)\n",second_moving_frames,second_traveled,second_locomotion.updates,second_locomotion.completed_fades,scene_checks,second_visible_frames);
 done:nflstreet_player_close(&ball_player);free(ball_profile);free(ball_asset);free(ball_sources);free(attachment_profile);for(unsigned i=0;i<model_count;++i)free(models[i].b);free(profile);free(bank);free(skeleton);free(movement_fixture);free(locomotion_bank);if(!set_catalog)free(locomotion_profile);free(set_catalog);free(sources);free(selectors);free(model_ids);nflstreet_player_close(&player);nflstreet_field_surface_close(&field);nflstreet_texture_close(&image);return status;
}
