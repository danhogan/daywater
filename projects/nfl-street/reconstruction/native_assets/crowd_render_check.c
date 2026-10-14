/* Inspection render of the saved frame with the crowd: the scene_frame.c
   order (field camera, sky, field passes with marks and ball shadows, actor
   shadows, lines, rings, main field), then the 45 crowd entries (800D2F20)
   where the original node draws them, then the ball, the 14 players and the
   HUD, each call executed by the software GX executor as it is issued. Writes
   the image with and without the crowd and a stable digest; not a pixel
   claim. The saved RAM is fixture input only. */
#include "gx_render.h"
#include "texture.h"
#include "scenery_draw.h"
#include "crowd_draw.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define RAM_BASE 0x80000000u
#define RAM_BYTES (24u*1024u*1024u)
#define R13 0x803a9a40u
#define WIDTH 640u
#define HEIGHT 448u
#define CROWD_OBJECTS 45u
#define CROWD_FIRST 0x80c4e094u
static uint32_t rd32(const uint8_t *p){return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static uint8_t *ram_at(uint8_t *ram,uint32_t a,uint32_t n){return a>=RAM_BASE&&(uint64_t)a-RAM_BASE+n<=RAM_BYTES?ram+(a-RAM_BASE):NULL;}
static uint32_t ram32(uint8_t *ram,uint32_t a){const uint8_t *p=ram_at(ram,a,4);return p?rd32(p):0;}
static float ramf(uint8_t *ram,uint32_t a){uint32_t v=ram32(ram,a);float f;memcpy(&f,&v,4);return f;}
#include "venue_fixture.h"
#include "hud_fixture.h"
#include "scene_fixture.h"
static uint32_t fnv(uint32_t h,const void *p,size_t n){const uint8_t *b=p;for(size_t i=0;i<n;++i)h=(h^b[i])*16777619u;return h;}
static uint8_t *span(void *m,uint32_t token,uint32_t size){return ram_at(m,token,size);}
typedef struct {nflstreet_gx_machine *machine;nflstreet_gx_stats *stats;unsigned count;} stream;
static int stream_call(void *sink,const nflstreet_gx_call *c){stream *x=sink;
 if(!nflstreet_gx_run(x->machine,c,1,x->stats)){fprintf(stderr,"executor rejected streamed call %u op %d args %08x %08x %08x %08x\n",x->count,
  (int)c->op,c->arg[0],c->arg[1],c->arg[2],c->arg[3]);return 0;}
 ++x->count;return 1;}
/* The crowd context from saved globals (as crowd_draw_original_check.c). */
static void crowd_context(uint8_t *ram,const nflstreet_scene *sc,nflstreet_crowd_context *c){
 memset(c,0,sizeof *c);c->scene=&sc->venue;c->palettes=sc->palettes;c->enable=R13-0x72a4u;c->view_slot=0x80331ec0u+0xc0u;c->cull_box=0x8028e2b4u;
 c->game_mode=0x802a301cu;c->sides=ram32(ram,R13-0x5ba4u);c->hide_limit=0x802a3050u;c->scratch=R13-0x6430u;
 c->hide_sign[0]=ramf(ram,0x80260f2cu);c->hide_sign[1]=ramf(ram,0x80260f30u);c->light_default[0]=ramf(ram,0x80260f04u);c->light_default[1]=ramf(ram,0x80260f54u);
 c->palette_scale[0]=ramf(ram,0x80260f1cu);c->palette_scale[1]=ramf(ram,0x80260f58u);c->normal_scale=ramf(ram,0x8026381cu);c->quat_one=ramf(ram,0x802634d8u);}
/* scene_frame.c's nflstreet_scene_venue with the crowd before the ball. */
static int venue_with_crowd(nflstreet_scene *sc,const nflstreet_crowd_context *crowd,nflstreet_render_state *s,int with_crowd){
 nflstreet_draw d={&sc->draw,s};uint32_t top;
 if(!nflstreet_camera_view(&sc->hud,sc->field_camera,s)||!nflstreet_matrix_top(&d,&sc->hud.stacks,&top))return 0;
 uint8_t *slot=nflstreet_draw_at(&d,sc->view_slot,0x40);const uint8_t *t=nflstreet_draw_at(&d,top,0x40);if(!slot||!t)return 0;
 memmove(slot,t,0x40);if(!nflstreet_draw_word(&d,sc->hud.stacks.current,&sc->venue.matrix_stack))return 0;
 if(!nflstreet_sky_draw(&sc->venue,s)||!nflstreet_field_passes_draw(&sc->venue,sc->field_passes,s)||!nflstreet_shadows_draw(&sc->venue,s))return 0;
 for(unsigned i=0;i<4;++i)if(!nflstreet_line_draw(&sc->venue,sc->lines[i],s))return 0;
 for(unsigned i=0;i<3;++i)if(!nflstreet_ring_draw(&sc->venue,sc->rings[i],s))return 0;
 if(!nflstreet_field_draw(&sc->venue,sc->main_field,s))return 0;
 if(with_crowd){uint8_t *used=nflstreet_draw_at(&d,sc->palettes->pool_used,4);if(!used)return 0;nflstreet_wr32(used,0); /* 801CA494 at frame start */
  for(unsigned i=0;i<CROWD_OBJECTS;++i)if(!nflstreet_crowd_draw(crowd,CROWD_FIRST+0x1dcu*i,s)){fprintf(stderr,"crowd entry %u rejected\n",i);return 0;}}
 return nflstreet_ball_draw(&sc->ball,s);}
static void reset_state(uint8_t *ram,nflstreet_render_state *s){memset(s,0,sizeof *s);for(unsigned i=0;i<6;++i)s->viewport[i]=ramf(ram,0x8039e584u+4*i);}
int main(int argc,char **argv){
 if(argc!=4)return 2;
 FILE *f=fopen(argv[1],"rb");if(!f)return 3;uint8_t *ram=malloc(RAM_BYTES),*pristine=malloc(RAM_BYTES);
 if(!ram||!pristine||fread(pristine,1,RAM_BYTES,f)!=RAM_BYTES){fclose(f);return 3;}fclose(f);
 uint32_t gx=ram32(pristine,R13-0x4c90u);nflstreet_gx_frame frame;memset(&frame,0,sizeof frame);
 frame.projection_type=ram32(pristine,gx+0x420);for(unsigned i=0;i<6;++i){frame.projection[i]=ramf(pristine,gx+0x424+4*i);frame.viewport[i]=ramf(pristine,gx+0x43c+4*i);}
 frame.zmode=0x17;frame.cmode0=ram32(pristine,gx+0x1d0);for(unsigned i=0;i<8;++i)frame.ksel[i]=ram32(pristine,gx+0x1b0+4*i);
 for(unsigned i=0;i<8;++i){frame.texgen_src[i]=4;frame.texgen_mtx[i]=60;}
 uint32_t *bgra=malloc(WIDTH*HEIGHT*4),*depth=malloc(WIDTH*HEIGHT*4);if(!bgra||!depth)return 3;
 for(int pass=0;pass<2;++pass){
  memcpy(ram,pristine,RAM_BYTES);for(unsigned i=0;i<WIDTH*HEIGHT;++i){bgra[i]=0xff404040u;depth[i]=0xffffffu;}
  if(!venue_present(ram)||!hud_present(ram))return 11;
  nflstreet_gx_target target={WIDTH,HEIGHT,bgra,depth};nflstreet_gx_machine *m=nflstreet_gx_open(span,ram,&frame,&target);if(!m)return 4;
  nflstreet_render_state s;reset_state(ram,&s);nflstreet_gx_stats stats;memset(&stats,0,sizeof stats);
  stream streamed={m,&stats,0};static nflstreet_scene sc;scene_fixture(ram,&sc,span,stream_call,&streamed);
  nflstreet_crowd_context crowd;crowd_context(ram,&sc,&crowd);
  if(!venue_with_crowd(&sc,&crowd,&s,pass))return 12;const unsigned venue=streamed.count;
  if(!nflstreet_scene_players(&sc,&s)||!nflstreet_scene_hud(&sc,&s))return 5;
  nflstreet_gx_close(m);
  nflstreet_texture image={WIDTH,HEIGHT,bgra};if(!nflstreet_texture_bmp(&image,argv[pass?3:2]))return 7;
  printf("%s: %u venue GX calls, %u in all; %u display lists, %u triangles (%u culled, %u clipped), %zu shaded pixels; digest %08x.\n",
   pass?"Scene with crowd":"Scene without crowd",venue,streamed.count,stats.display_lists,stats.triangles,stats.culled,stats.clipped,stats.pixels,
   fnv(2166136261u,bgra,WIDTH*HEIGHT*4));}
 free(depth);free(bgra);free(pristine);free(ram);return 0;}
