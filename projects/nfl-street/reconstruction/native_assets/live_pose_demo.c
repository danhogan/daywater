/* Standalone check of the live pose source on a saved match: the
   controlled player walks forward (stick up) through the verified movement
   runtime, then stops. Each 60 Hz update runs, in the live app's order:
   movement written back to the gameplay actor, the native pose
   (live_pose.c), placement (800D4A6C), the compact skeleton (800D51B4 /
   8015F08C) and the actor's skinning palettes (80162760). It writes a CSV
   of the pose angles, model-space ankle positions (+60 bones 3/7) and
   view-space ankle palette matrices, and fails unless the legs alternate
   while walking and the palettes follow. Usage:
   live-pose-demo <mem1> <animdata/00111.bin> <sets.bin> <out.csv> */
#include "scenery_draw.h"
#include "check_fixture.h"
#include "actor_palettes.h"
#include "actor_world.h"
#include "actor_skeleton.h"
#include "live_pose.h"
#include "live_movement.h"
#include <math.h>
#define WALK 150u
#define STOP 60u
static uint8_t *readfile(const char *path,size_t *n){FILE *f=fopen(path,"rb");if(!f)return NULL;fseek(f,0,SEEK_END);long s=ftell(f);rewind(f);
 uint8_t *b=s>0?malloc((size_t)s):NULL;if(!b||fread(b,1,(size_t)s,f)!=(size_t)s){free(b);fclose(f);return NULL;}fclose(f);*n=(size_t)s;return b;}
static uint8_t *ram_span(void *m,uint32_t a,uint32_t n){return ram_at(m,a,n);}
/* The controlled player as live_main.c finds it (host lookup). */
static uint32_t controlled(uint8_t *ram){
 const uint32_t table=ram32(ram,R13-0x7bbcu);uint32_t visual=0;float best=1e30f;
 for(unsigned i=0;i<14;++i){const uint32_t v=ram32(ram,table+4*i);const float x=ramf(ram,v+4)-ramf(ram,venue_rings[0]+4),y=ramf(ram,v+8)-ramf(ram,venue_rings[0]+8);
  if(x*x+y*y<best){best=x*x+y*y;visual=v;}}
 for(uint32_t o=0;o+0x1000<=RAM_BYTES;o+=4){const uint8_t *g=ram+o;
  if(rd32(g+4)==visual&&rd32(g+0x1a8)==ram32(ram,visual+4)&&rd32(g+0x1ac)==ram32(ram,visual+8))return RAM_BASE+o;}
 return 0;}
static int16_t r16(uint8_t *ram,uint32_t a){const uint8_t *p=ram_at(ram,a,2);return (int16_t)(uint16_t)(p[0]<<8|p[1]);}
int main(int argc,char **argv){
 if(argc!=5)return 2;size_t bn,cn;uint8_t *ram=load_ram(argv[1]),*bank=readfile(argv[2],&bn),*catalog=readfile(argv[3],&cn);FILE *csv=fopen(argv[4],"w");
 if(!ram||!bank||!catalog||!csv)return 3;
 const uint32_t g=controlled(ram),v=g?ram32(ram,g+4):0;if(!g){fprintf(stderr,"no controlled player\n");return 4;}
 static nflstreet_live_movement movement;uint32_t actor[12],state[21];for(unsigned i=0;i<12;++i)actor[i]=ram32(ram,g+0x200+4*i);for(unsigned i=0;i<21;++i)state[i]=ram32(ram,g+0x1a8+4*i);
 if(!nflstreet_live_movement_init(&movement,ram32(ram,g),r16(ram,g+0xbb4),r16(ram,g+0xbbc),ram32(ram,g+0x408),actor,state))return 4;
 movement.seed.completed=movement.runtime.completed=1;
 static nflstreet_live_pose pose;if(!nflstreet_live_pose_open(&pose,bank,bn,catalog,cn)){fprintf(stderr,"cannot open the pose source\n");return 4;}
 nflstreet_player_draw_context x;memset(&x,0,sizeof x);x.span=ram_span;x.memory=ram;static nflstreet_render_state unused;nflstreet_draw d={&x,&unused};
 static nflstreet_actor_palette_context c;c.x=&x;c.stacks=venue_stacks(ram);c.unit[0]=ramf(ram,0x803a3958u);c.unit[1]=ramf(ram,0x803a395cu);
 c.pool=R13-0x5680u;c.pool_index=R13-0x5678u;c.pool_used=R13-0x5670u;c.cache_on=R13-0x6200u;
 /* The body palette rows of the ankles (Clst rows on bones 3 and 7 at full weight). */
 const uint32_t model=ram32(ram,v+0x4b8),clst=ram32(ram,model+0xc),rows=ram32(ram,clst),clusters=ram32(ram,clst+4);int ankle[2]={-1,-1};
 for(uint32_t i=1;i<clusters;++i){const uint8_t *r=ram_at(ram,rows+4*i,4);if(r[2]==0x10&&r[3]==0){if(r[0]==3)ankle[0]=(int)i;if(r[0]==7)ankle[1]=(int)i;}}
 if(ankle[0]<0||ankle[1]<0){fprintf(stderr,"no ankle palette rows\n");return 4;}
 fprintf(csv,"update,moving,speed,entries,clip0,clip1,rhip_x,rknee_x,lhip_x,lknee_x,root_y,heading,rankle_x,rankle_y,rankle_z,lankle_x,lankle_y,lankle_z,palette_rankle_tx,palette_rankle_ty,palette_lankle_tx,palette_lankle_ty\n");
 unsigned alternations=0,palette_changes=0;int last_sign=0;float knee_min=1e9f,knee_max=-1e9f,stride_min=1e9f,stride_max=-1e9f,previous[2]={0,0};
 for(unsigned u=0;u<WALK+STOP;++u){const int moving=u<WALK;nflstreet_live_motion mo;
  if(nflstreet_live_movement_step(&movement,0,moving?1:0,&mo)<0){fprintf(stderr,"movement branch not reconstructed at %u\n",u);return 5;}
  for(unsigned i=0;i<12;++i)wr32(ram_at(ram,g+0x200+4*i,4),movement.runtime.actor[i]);for(unsigned i=0;i<21;++i)wr32(ram_at(ram,g+0x1a8+4*i,4),movement.runtime.state[i]);
  if(!nflstreet_live_pose_step(&pose,ram_span,ram,g,0,1.0)){fprintf(stderr,"pose step failed at %u\n",u);return 6;}
  if(!nflstreet_matrix_select(&d,&c.stacks)||!nflstreet_actor_place(&d,&c.stacks,g,R13-0x7078u)){fprintf(stderr,"placement failed\n");return 6;}
  if(!nflstreet_actor_skeleton_frame(&c,v)){fprintf(stderr,"skeleton failed at %u\n",u);return 6;}
  const uint32_t record=ram32(ram,v+0x4a8);wr32(ram_at(ram,c.pool_used,4),0);
  if(!nflstreet_matrix_select(&d,&c.stacks)||!nflstreet_actor_palettes(&c,v,NULL,v+0x3ac,record+0x28,ram32(ram,record+8))){fprintf(stderr,"palettes failed at %u\n",u);return 6;}
  const uint32_t p=ram32(ram,v+0x5c),bones=ram32(ram,v+0x60),palette=ram32(ram,v+0x1008);float a[2][3],t[2][2];
  for(unsigned k=0;k<2;++k){for(unsigned w=0;w<3;++w)a[k][w]=ramf(ram,bones+0x40u*(k?7u:3u)+16*w+12);for(unsigned w=0;w<2;++w)t[k][w]=ramf(ram,palette+0x30u*(unsigned)ankle[k]+16*w+12);}
  for(unsigned k=0;k<2;++k)for(unsigned w=0;w<3;++w)if(!isfinite(a[k][w]))return 7;
  const float knee=(float)r16(ram,p+12);if(knee<knee_min)knee_min=knee;if(knee>knee_max)knee_max=knee;
  /* Model-space stride: the ankles' separation along the longer horizontal axis of their difference. */
  const float stride=a[0][2]-a[1][2];if(moving&&u>=30){if(stride<stride_min)stride_min=stride;if(stride>stride_max)stride_max=stride;
   const int sign=stride>0.05f?1:stride<-0.05f?-1:0;if(sign&&sign!=last_sign){if(last_sign)++alternations;last_sign=sign;}}
  if(u&&(t[0][0]!=previous[0]||t[1][0]!=previous[1]))++palette_changes;previous[0]=t[0][0];previous[1]=t[1][0];
  fprintf(csv,"%u,%d,%.6f,%u,%u,%u,%d,%d,%d,%d,%.6f,%08x,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f\n",u,moving,ramf(ram,g+0x1a8+28),pose.entries,
   pose.locomotion.clips[0],pose.locomotion.clips[1],r16(ram,p+6),r16(ram,p+12),r16(ram,p+30),r16(ram,p+36),ramf(ram,v+0x38),ram32(ram,v+0x4c),
   a[0][0],a[0][1],a[0][2],a[1][0],a[1][1],a[1][2],t[0][0],t[0][1],t[1][0],t[1][1]);}
 fclose(csv);
 printf("Live pose demo: controlled gameplay %08x visual %08x, %u updates walking then %u stopped; %u pose updates, %u locomotion pair starts, "
  "%u completed fades; right knee angle %.0f..%.0f; ankle stride %.3f..%.3f with %u alternations while walking; ankle palettes changed in %u updates.\n",
  g,v,WALK,STOP,pose.updates,pose.locomotion.changes,pose.locomotion.completed_fades,knee_min,knee_max,stride_min,stride_max,alternations,palette_changes);
 return alternations>=4&&knee_max-knee_min>2000.0f&&palette_changes>WALK/2?0:8;}
