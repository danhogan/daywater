/* NFL Street live prototype: the verified native scene draws (scene_frame.c)
   running every frame on Aurora (window, D3D12/Vulkan through WebGPU) from a
   saved match RAM image. State the draws advance (field-line alpha pulse,
   mark alpha ramp, matrix stacks, the HUD) carries from frame to frame; the
   field camera follows the controller or keyboard. */
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
#include "gx_aurora.h"
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
/* The field camera's data (+04 position, +14/+18/+1C angles) moved by the
   stick: x/y pan, C-stick orbit about z and pitch. */
static void steer(uint8_t *ram,uint32_t orbit){
 uint8_t *d=ram_at(ram,ram32(ram,hud_cameras[0]+0x40),0x20);if(!d)return;
 if(orbit)wr32(d+0x1c,(rd32(d+0x1c)+orbit)&0xffffffu); /* demo: turn about z by this 24-bit angle per frame */
 PADStatus pads[4];PADRead(pads);const PADStatus *p=&pads[0];if(p->err!=PAD_ERR_NONE)return;
 float x,y;uint32_t w;w=rd32(d+4);memcpy(&x,&w,4);w=rd32(d+8);memcpy(&y,&w,4);
 x+=(float)p->stickX*0.003f;y+=(float)p->stickY*0.003f;memcpy(&w,&x,4);wr32(d+4,w);memcpy(&w,&y,4);wr32(d+8,w);
 wr32(d+0x1c,(rd32(d+0x1c)+(uint32_t)(int32_t)(-p->substickX*40))&0xffffffu);
 wr32(d+0x14,(rd32(d+0x14)+(uint32_t)(int32_t)(p->substickY*20))&0xffffffu);}
static float word_at(uint8_t *ram,uint32_t a){return ramf(ram,a);}
/* The view the saved frame was drawn with: the stored main-field matrix
   without the venue translation (80165DC4: view x T(object +4)). */
static void drawn_field_view(uint8_t *ram,float v[12]){
 float p[3];for(unsigned i=0;i<12;++i)v[i]=word_at(ram,VENUE_MAIN_FIELD+0x24+8+4*i);for(unsigned i=0;i<3;++i)p[i]=word_at(ram,VENUE_MAIN_FIELD+4+4*i);
 for(unsigned r=0;r<3;++r)v[4*r+3]-=v[4*r]*p[0]+v[4*r+1]*p[1]+v[4*r+2]*p[2];}
/* D = current view (the viewport stack's top) x inverse(drawn view). */
static int view_delta(uint8_t *ram,const float drawn[12],float d[12]){
 const uint32_t stack=ram32(ram,R13-0x65e8u),top=ram32(ram,stack+8)+0x40u*ram32(ram,stack+4);float v[12],inv[12];
 for(unsigned i=0;i<12;++i)v[i]=word_at(ram,top+4*i);
 const float *m=drawn;const float det=m[0]*(m[5]*m[10]-m[6]*m[9])-m[1]*(m[4]*m[10]-m[6]*m[8])+m[2]*(m[4]*m[9]-m[5]*m[8]);
 if(!(det!=0.0f))return 0;const float k=1.0f/det;
 inv[0]=(m[5]*m[10]-m[6]*m[9])*k;inv[1]=(m[2]*m[9]-m[1]*m[10])*k;inv[2]=(m[1]*m[6]-m[2]*m[5])*k;
 inv[4]=(m[6]*m[8]-m[4]*m[10])*k;inv[5]=(m[0]*m[10]-m[2]*m[8])*k;inv[6]=(m[2]*m[4]-m[0]*m[6])*k;
 inv[8]=(m[4]*m[9]-m[5]*m[8])*k;inv[9]=(m[1]*m[8]-m[0]*m[9])*k;inv[10]=(m[0]*m[5]-m[1]*m[4])*k;
 for(unsigned r=0;r<3;++r)inv[4*r+3]=-(inv[4*r]*m[3]+inv[4*r+1]*m[7]+inv[4*r+2]*m[11]);
 for(unsigned i=0;i<3;++i)for(unsigned j=0;j<4;++j)d[4*i+j]=v[4*i]*inv[j]+v[4*i+1]*inv[4+j]+v[4*i+2]*inv[8+j]+(j==3?v[4*i+3]:0.0f);
 return 1;}
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
 const uint32_t gx=ram32(ram,R13-0x4c90u);float drawn_view[12];drawn_field_view(ram,drawn_view);unsigned frames=0;int exiting=0,failed=0;
 while(!exiting){
  const AuroraEvent *e=aurora_update();
  for(;e&&e->type!=AURORA_NONE;++e)if(e->type==AURORA_EXIT)exiting=1;
  if(exiting||!aurora_begin_frame())continue;
  GXSetCopyClear((GXColor){64,64,64,255},GX_MAX_Z24);
  steer(ram,orbit);
  nflstreet_render_state s;memset(&s,0,sizeof s);for(unsigned i=0;i<6;++i)s.viewport[i]=ramf(ram,0x8039e584u+4*i);
  nflstreet_gx_aurora_new_frame(bridge);
  int ok=!failed&&nflstreet_gx_aurora_begin_frame(bridge,gx)&&(!(parts&1)||nflstreet_scene_venue(&scene,&s));
  /* The players still carry the drawn frame's palettes: re-express them for
     the current camera (top of the viewport's stack after the venue). */
  float delta[12];if(ok&&view_delta(ram,drawn_view,delta))nflstreet_gx_aurora_set_view_delta(bridge,delta);
  ok=ok&&(!(parts&2)||nflstreet_scene_players(&scene,&s));nflstreet_gx_aurora_set_view_delta(bridge,NULL);
  ok=ok&&(!(parts&4)||nflstreet_scene_hud(&scene,&s));
  if(!failed&&!ok){failed=1;
   if(log_file){fprintf(log_file,"frame %u: scene draw stopped after %u GX calls\n",frames,nflstreet_gx_aurora_calls(bridge));fflush(log_file);}}
  nflstreet_gx_aurora_flush(bridge);
  aurora_end_frame();
  if(++frames%120==0&&log_file){const AuroraStats *st=aurora_get_stats();
   fprintf(log_file,"frame %u, %u GX calls so far; draws %u (merged %u), pipelines queued %u created %u, %.0f fps\n",frames,
    nflstreet_gx_aurora_calls(bridge),st->drawCallCount,st->mergedDrawCallCount,st->queuedPipelines,st->createdPipelines,aurora_get_fps());fflush(log_file);}}
 nflstreet_gx_aurora_close(bridge);aurora_shutdown();free(ram);return 0;}
