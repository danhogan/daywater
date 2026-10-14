/* NFL Street live prototype: the verified native scene draws (scene_frame.c)
   running every frame on Aurora (window, D3D12/Vulkan through WebGPU) from a
   saved match RAM image. State the draws advance (field-line alpha pulse,
   mark alpha ramp, matrix stacks, the HUD) carries from frame to frame; the
   controlled player moves with the main stick through the verified movement
   runtime and is placed and skinned natively; the C-stick turns the camera. */
#include <aurora/aurora.h>
#include <aurora/event.h>
#include <aurora/main.h>
#include <aurora/gfx.h>
#include <dolphin/gx.h>
#include <dolphin/pad.h>
#include <dolphin/vi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "gx_aurora.h"
#include "actor_world.h"
#include "live_movement.h"
#include "live_pose.h"
#include "actor_skeleton.h"
#include "select_rings.h"
#define RAM_BASE 0x80000000u
#define RAM_BYTES (24u*1024u*1024u)
#define R13 0x803a9a40u
static uint32_t rd32(const uint8_t *p){return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void wr32(uint8_t *p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
static uint8_t *ram_at(uint8_t *ram,uint32_t a,uint32_t n){return a>=RAM_BASE&&(uint64_t)a-RAM_BASE+n<=RAM_BYTES?ram+(a-RAM_BASE):NULL;}
static uint32_t ram32(uint8_t *ram,uint32_t a){const uint8_t *p=ram_at(ram,a,4);return p?rd32(p):0;}
static float ramf(uint8_t *ram,uint32_t a){uint32_t v=ram32(ram,a);float f;memcpy(&f,&v,4);return f;}
#include "venue_fixture.h"
#include "hud_fixture.h"
#include "scene_fixture.h"
static uint8_t *span(void *m,uint32_t token,uint32_t size){return ram_at(m,token,size);}
static FILE *log_file;
static void log_callback(AuroraLogLevel level,const char *module,const char *message,unsigned int len){
 (void)len;static const char *names[]={"DEBUG","INFO","WARNING","ERROR","FATAL"};
 FILE *out=log_file?log_file:stderr;fprintf(out,"[%s %s] %s\n",(unsigned)level<5?names[level]:"?",module,message);fflush(out);
 if(level==LOG_FATAL)abort();}
static void note(const char *what){if(log_file){fprintf(log_file,"%s\n",what);fflush(log_file);}}
/* The field camera's data (+04 position, +14/+18/+1C angles): the C-stick
   orbits about z and pitches; the camera position follows the controlled
   player by its movement (a host choice: the game's camera logic is not
   reconstructed). */
static void steer(uint8_t *ram,uint32_t orbit,const PADStatus *p,float dx,float dy){
 uint8_t *d=ram_at(ram,ram32(ram,hud_cameras[0]+0x40),0x20);if(!d)return;
 if(orbit)wr32(d+0x1c,(rd32(d+0x1c)+orbit)&0xffffffu); /* demo: turn about z by this 24-bit angle per frame */
 float x,y;uint32_t w;w=rd32(d+4);memcpy(&x,&w,4);w=rd32(d+8);memcpy(&y,&w,4);
 x+=dx;y+=dy;memcpy(&w,&x,4);wr32(d+4,w);memcpy(&w,&y,4);wr32(d+8,w);
 if(p->err!=PAD_ERR_NONE)return;
 wr32(d+0x1c,(rd32(d+0x1c)+(uint32_t)(int32_t)(-p->substickX*40))&0xffffffu);
 wr32(d+0x14,(rd32(d+0x14)+(uint32_t)(int32_t)(p->substickY*20))&0xffffffu);}
/* The controlled player: the visual actor standing nearest the player-select
   ring, and its gameplay actor (the record whose +04 names the visual and
   whose +1A8 position equals the visual's +04). A host lookup; the game's
   controller-to-actor assignment is not reconstructed. */
static uint32_t controlled_actor(uint8_t *ram){
 const uint32_t table=ram32(ram,R13-0x7bbcu);uint32_t visual=0;float best=1e30f;
 for(unsigned i=0;i<14;++i){const uint32_t v=ram32(ram,table+4*i);const float x=ramf(ram,v+4)-ramf(ram,venue_rings[0]+4),y=ramf(ram,v+8)-ramf(ram,venue_rings[0]+8);
  if(x*x+y*y<best){best=x*x+y*y;visual=v;}}
 for(uint32_t o=0;o+0x1000<=RAM_BYTES;o+=4){const uint8_t *g=ram+o;
  if(rd32(g+4)==visual&&rd32(g+0x1a8)==ram32(ram,visual+4)&&rd32(g+0x1ac)==ram32(ram,visual+8))return RAM_BASE+o;}
 return 0;}
static int16_t ram16(uint8_t *ram,uint32_t a){const uint8_t *p=ram_at(ram,a,2);return p?(int16_t)(uint16_t)(p[0]<<8|p[1]):0;}
static int seed_movement(uint8_t *ram,uint32_t g,nflstreet_live_movement *m){
 uint32_t actor[12],state[21];for(unsigned i=0;i<12;++i)actor[i]=ram32(ram,g+0x200+4*i);for(unsigned i=0;i<21;++i)state[i]=ram32(ram,g+0x1a8+4*i);
 if(!nflstreet_live_movement_init(m,ram32(ram,g),ram16(ram,g+0xbb4),ram16(ram,g+0xbbc),ram32(ram,g+0x408),actor,state))return 0;
 /* Every update starts with the original's reset (80036AF8, then the
    acceleration reset); the runtime skips it only for a first step resumed
    from a recording. The saved player is standing (no prior acceleration),
    so the first step needs it too. */
 m->seed.completed=m->runtime.completed=1;return 1;}
/* The stick as a cardinal or diagonal request (the movement runtime's input). */
static void stick(const PADStatus *p,int *dx,int *dy){
 *dx=*dy=0;if(p->err!=PAD_ERR_NONE)return;*dx=p->stickX>40?1:p->stickX<-40?-1:0;*dy=p->stickY>40?1:p->stickY<-40?-1:0;}
static uint8_t *read_file(const char *path,size_t *size){
 FILE *f=path?fopen(path,"rb"):NULL;if(!f)return NULL;fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);uint8_t *b=n>0?malloc((size_t)n):NULL;
 if(!b||fread(b,1,(size_t)n,f)!=(size_t)n){free(b);fclose(f);return NULL;}fclose(f);*size=(size_t)n;return b;}
static double seconds(void){struct timespec t;timespec_get(&t,TIME_UTC);return (double)t.tv_sec+t.tv_nsec*1e-9;}
int main(int argc,char *argv[]){
 log_file=fopen(argc>2?argv[2]:"nflstreet_live.log","w");
 const char *image=argc>1?argv[1]:"position-neutral-15.mem1";
 FILE *f=fopen(image,"rb");uint8_t *ram=malloc(RAM_BYTES);
 if(!f||!ram||fread(ram,1,RAM_BYTES,f)!=RAM_BYTES){note("cannot read the RAM image");return 2;}fclose(f);
 if(!venue_present(ram)||!hud_present(ram)){note("not the saved EARS match");return 3;}
 const AuroraConfig config={.appName="NFL Street live",.logCallback=log_callback,.logLevel=(getenv("NFL_LIVE_DEBUG")?LOG_DEBUG:LOG_INFO),.windowWidth=640,.windowHeight=448};
 AuroraInfo info=aurora_initialize(argc,argv,&config);(void)info;
 if(getenv("NFL_LIVE_TRACE"))nflstreet_gx_aurora_trace=fopen(getenv("NFL_LIVE_TRACE"),"w");
 static uint8_t fifo[256*1024];GXInit(fifo,sizeof fifo); /* the SDK's default GX state, as the game does at boot */
 /* The game's 640x448 EFB (its saved viewport): NTSC interlaced with 448 lines. */
 static GXRenderModeObj mode;mode=GXNtsc480IntDf;mode.efbHeight=448;mode.xfbHeight=448;VIConfigure(&mode);
 GXSetScissor(0,0,mode.fbWidth,mode.efbHeight);
 PADInit();
 nflstreet_gx_aurora *bridge=nflstreet_gx_aurora_open(ram,RAM_BYTES);static nflstreet_scene scene;
 scene_fixture(ram,&scene,span,nflstreet_gx_aurora_emit,bridge);
 /* Optional third argument: the parts to draw (1 venue, 2 players, 4 HUD). */
 const unsigned parts=argc>3?(unsigned)strtoul(argv[3],NULL,0):7u;
 /* NFL_LIVE_ORBIT: per-frame turn (24-bit angle; 1 means 0x2000, about 10 degrees a second at 60 fps). */
 uint32_t orbit=getenv("NFL_LIVE_ORBIT")?(uint32_t)strtoul(getenv("NFL_LIVE_ORBIT"),NULL,0):0;if(orbit==1)orbit=0x2000;
 /* The controlled player (NFL_LIVE_SCRIPT=1 drives it right, forward, left for a capture). */
 const uint32_t player=controlled_actor(ram);nflstreet_live_movement movement;int rejected=0;const int scripted=getenv("NFL_LIVE_SCRIPT")!=NULL;
 if(player&&!seed_movement(ram,player,&movement)){note("cannot seed the controlled player's movement");return 4;}
 if(log_file){fprintf(log_file,"controlled player %08x (visual %08x)\n",player,player?ram32(ram,player+4):0);fflush(log_file);}
 /* Animation: NFL_LIVE_ANIMDATA = animdata/00111.bin (bank 7) and
    NFL_LIVE_SETS = an NSC1 set catalog (live_pose.h); without them the
    player keeps its saved stance. */
 size_t bank_n=0,sets_n=0;uint8_t *bank=read_file(getenv("NFL_LIVE_ANIMDATA"),&bank_n),*sets=read_file(getenv("NFL_LIVE_SETS"),&sets_n);
 static nflstreet_live_pose pose;const int animated=player&&bank&&sets&&nflstreet_live_pose_open(&pose,bank,bank_n,sets,sets_n);
 if(log_file){fprintf(log_file,"animation %s\n",animated?"on":"off");fflush(log_file);}
 const nflstreet_rings_context rings={&scene.draw,R13};
 const double start=seconds();double clock_time=start;
 const uint32_t gx=ram32(ram,R13-0x4c90u);unsigned frames=0;int exiting=0,failed=0;
 while(!exiting){
  const AuroraEvent *e=aurora_update();
  for(;e&&e->type!=AURORA_NONE;++e)if(e->type==AURORA_EXIT)exiting=1;
  if(exiting||!aurora_begin_frame())continue;
  GXSetCopyClear((GXColor){64,64,64,255},GX_MAX_Z24);
  /* Fixed 60 Hz updates of the select rings, the controlled player's verified movement
     (input, acceleration, turning, kinematics), written back to its
     gameplay actor (+200 input, +1A8 movement block), then the native
     placement (800D4A6C/800D4B0C) of its visual. */
  PADStatus pads[4];PADRead(pads);float moved[2]={0,0};
  if(player){int dx,dy;stick(&pads[0],&dx,&dy);if(scripted){const double t=seconds()-start;dx=t<1.5?1:t<3.0?0:t<4.5?-1:0;dy=t>=1.5&&t<3.0?1:0;}
   for(const double now=seconds();clock_time+1.0/60.0<=now;clock_time+=1.0/60.0){
    /* The rings follow the position the previous update left (800E4A24),
       so they trail the player by one update, as in the saved frame. */
    if(!nflstreet_rings_update(&rings))failed=1;
    const float x0=ramf(ram,player+0x1a8),y0=ramf(ram,player+0x1ac);nflstreet_live_motion mo;
    const int r=nflstreet_live_movement_step(&movement,dx,dy,&mo);
    if(r<0){if(!rejected&&log_file){fprintf(log_file,"frame %u: movement request %d,%d takes an unreconstructed branch; held\n",frames,dx,dy);fflush(log_file);}rejected=1;}
    else{rejected=0;for(unsigned i=0;i<12;++i)wr32(ram_at(ram,player+0x200+4*i,4),movement.runtime.actor[i]);
     for(unsigned i=0;i<21;++i)wr32(ram_at(ram,player+0x1a8+4*i,4),movement.runtime.state[i]);
     moved[0]+=ramf(ram,player+0x1a8)-x0;moved[1]+=ramf(ram,player+0x1ac)-y0;}
    /* Then the animation update: pose (8015E9C8, 8015EC30, 80053404 from
       the host clip choice) and the compact skeleton (800D51B4/8015F08C). */
    if(animated&&(!nflstreet_live_pose_step(&pose,span,ram,player,0,1.0)||!nflstreet_actor_skeleton_frame(scene.palettes,ram32(ram,player+4))))failed=1;}
   nflstreet_render_state unused;memset(&unused,0,sizeof unused);nflstreet_draw d={&scene.draw,&unused};
   if(!nflstreet_matrix_select(&d,&scene.hud.stacks)||!nflstreet_actor_place(&d,&scene.hud.stacks,player,R13-0x7078u))failed=1;}
  steer(ram,orbit,&pads[0],moved[0],moved[1]);
  nflstreet_render_state s;memset(&s,0,sizeof s);for(unsigned i=0;i<6;++i)s.viewport[i]=ramf(ram,0x8039e584u+4*i);
  nflstreet_gx_aurora_new_frame(bridge);
  /* The venue rebuilds the light sets from the camera (801ED3A0) and the
     players their palettes (80162760); without the venue the players draw
     with the stored light sets. */
  int ok=!failed&&nflstreet_gx_aurora_begin_frame(bridge,gx)&&nflstreet_scene_start(&scene,&s)&&(!(parts&1)||nflstreet_scene_venue(&scene,&s));
  ok=ok&&(!(parts&2)||nflstreet_scene_players(&scene,&s));
  ok=ok&&(!(parts&4)||nflstreet_scene_hud(&scene,&s));
  if(!failed&&!ok){failed=1;
   if(log_file){fprintf(log_file,"frame %u: scene draw stopped after %u GX calls\n",frames,nflstreet_gx_aurora_calls(bridge));fflush(log_file);}}
  nflstreet_gx_aurora_flush(bridge);
  aurora_end_frame();
  if(++frames%120==0&&log_file){const AuroraStats *st=aurora_get_stats();
   fprintf(log_file,"frame %u, %u GX calls so far; draws %u (merged %u), pipelines queued %u created %u, %.0f fps\n",frames,
    nflstreet_gx_aurora_calls(bridge),st->drawCallCount,st->mergedDrawCallCount,st->queuedPipelines,st->createdPipelines,aurora_get_fps());fflush(log_file);}}
 nflstreet_gx_aurora_close(bridge);aurora_shutdown();free(bank);free(sets);free(ram);return 0;}
