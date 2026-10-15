/* Inspection render of the saved frame through scene_frame.c's order, once
   as scene_frame.c draws it (the sky alone) and once with the field camera
   node's entry 1 (800E398C: projection change, sky, clouds, restore) in the
   sky's place, as the proposed integration does (venue_with_clouds below
   is nflstreet_scene_venue with that one call replaced). Each call runs in
   the software GX executor as it is issued; GXSetProjectionv
   (NFLSTREET_GX_SET_PROJECTION_V, not in gx_render.c's op table) is applied
   here to the executor's frame projection, which is why gx_render.c is
   included rather than linked. ALPHA0's channel control, which the clouds
   rely on but do not set, persists from the previous frame on hardware: each
   pass draws the frame twice on one executor and keeps the second. Writes both images and stable digests; not a
   pixel claim. The saved RAM is fixture input only. */
#include "gx_render.c"
#include "texture.h"
#include "scenery_draw.h"
#include "cloud_draw.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define RAM_BASE 0x80000000u
#define RAM_BYTES (24u*1024u*1024u)
#define R13 0x803a9a40u
#define WIDTH 640u
#define HEIGHT 448u
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
 if(c->op==NFLSTREET_GX_SET_PROJECTION_V){nflstreet_gx_frame *f=&x->machine->frame;f->projection_type=(uint32_t)as_float(c->data[0]);
  for(unsigned i=0;i<6;++i)f->projection[i]=as_float(c->data[1+i]);++x->count;return 1;}
 if(!nflstreet_gx_run(x->machine,c,1,x->stats)){fprintf(stderr,"executor rejected streamed call %u op %d args %08x %08x %08x %08x\n",x->count,
  (int)c->op,c->arg[0],c->arg[1],c->arg[2],c->arg[3]);return 0;}
 ++x->count;return 1;}
/* nflstreet_scene_venue (scene_frame.c) with nflstreet_sky_draw replaced by nflstreet_sky_node_draw. */
static int venue_with_clouds(nflstreet_scene *sc,const nflstreet_cloud_context *clouds,nflstreet_render_state *s){
 nflstreet_draw d={&sc->draw,s};uint32_t top;
 if(!nflstreet_camera_view(&sc->hud,sc->field_camera,s)||!nflstreet_matrix_top(&d,&sc->hud.stacks,&top))return 0;
 uint8_t *slot=nflstreet_draw_at(&d,sc->view_slot,0x40);const uint8_t *t=nflstreet_draw_at(&d,top,0x40);if(!slot||!t)return 0;
 if(sc->view){if(!nflstreet_camera_slots(sc->view))return 0;}else memmove(slot,t,0x40);
 if(!nflstreet_draw_word(&d,sc->hud.stacks.current,&sc->venue.matrix_stack))return 0;
 if(sc->lights&&!nflstreet_light_rig_update(sc->lights,s))return 0;
 if(!nflstreet_sky_node_draw(clouds,s)||!nflstreet_field_passes_draw(&sc->venue,sc->field_passes,s)||!nflstreet_shadows_draw(&sc->venue,s))return 0;
 for(unsigned i=0;i<4;++i)if(!nflstreet_line_draw(&sc->venue,sc->lines[i],s))return 0;
 for(unsigned i=0;i<3;++i)if(!nflstreet_ring_draw(&sc->venue,sc->rings[i],s))return 0;
 if(!nflstreet_field_draw(&sc->venue,sc->main_field,s)||(sc->crowd_count&&!sc->palettes))return 0;
 for(uint32_t i=0;i<sc->crowd_count;++i)if(!nflstreet_crowd_draw(&sc->crowd,sc->crowd_first+0x1dcu*i,s))return 0;
 return nflstreet_ball_draw(&sc->ball,s);}
static void cloud_context(uint8_t *ram,nflstreet_scene *sc,nflstreet_cloud_context *c){
 memset(c,0,sizeof *c);c->scene=&sc->venue;c->stacks=&sc->hud.stacks;c->enable=R13-0x71e3u;c->sky_enable=R13-0x71e4u;
 c->gx_state=ram32(ram,R13-0x4c90u);c->texture=0x802aa7a4u;c->records=0x80385c8cu;c->count=R13-0x7284u;c->texcoords=0x8028cc80u;
 c->period=0x802a3088u;c->overlay=0x802a3068u;c->palette=0x8028cbc0u;c->sky_record=0x8030feb4u;c->projection_value=ramf(ram,0x802536ccu);
 c->blend_cap=ramf(ram,0x802606d8u);c->blend_half=ramf(ram,0x802606dcu);c->blend_floor=ramf(ram,0x802606e0u);c->color_alpha=ramf(ram,0x802606e4u);
 c->one=ramf(ram,0x802606f8u);c->zero=ramf(ram,0x802606fcu);c->half=ramf(ram,0x80260700u);c->byte_scale=ramf(ram,0x80260704u);}
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
  nflstreet_gx_target target={WIDTH,HEIGHT,bgra,depth};nflstreet_gx_machine *m=nflstreet_gx_open(span,ram,&frame,&target);if(!m)return 4;
  nflstreet_gx_stats stats;stream streamed;unsigned venue=0;
  /* GX state persists across frames and the clouds set only COLOR0's channel control, so ALPHA0 is the previous
     frame's: the same frame is drawn twice on one executor from the saved RAM, and the second one is kept. */
  for(int warm=1;warm>=0;--warm){
   memcpy(ram,pristine,RAM_BYTES);for(unsigned i=0;i<WIDTH*HEIGHT;++i){bgra[i]=0xff404040u;depth[i]=0xffffffu;}
   if(!venue_present(ram)||!hud_present(ram))return 11;
   nflstreet_render_state s;reset_state(ram,&s);memset(&stats,0,sizeof stats);
   streamed=(stream){m,&stats,0};static nflstreet_scene sc;scene_fixture(ram,&sc,span,stream_call,&streamed);
   nflstreet_cloud_context clouds;cloud_context(ram,&sc,&clouds);
   if(!nflstreet_scene_start(&sc,&s)||!(pass?venue_with_clouds(&sc,&clouds,&s):nflstreet_scene_venue(&sc,&s)))return 12;venue=streamed.count;
   if(!nflstreet_scene_players(&sc,&s)||!nflstreet_scene_hud(&sc,&s))return 5;}
  nflstreet_gx_close(m);
  nflstreet_texture image={WIDTH,HEIGHT,bgra};if(!nflstreet_texture_bmp(&image,argv[pass?3:2]))return 7;
  printf("%s: %u venue GX calls and writes, %u in all; %u display lists, %u primitives, %u triangles (%u culled, %u clipped), %zu shaded pixels; digest %08x.\n",
   pass?"Scene with the sky node":"Scene with the sky alone",venue,streamed.count,stats.display_lists,stats.primitives,stats.triangles,stats.culled,stats.clipped,stats.pixels,
   fnv(2166136261u,bgra,WIDTH*HEIGHT*4));}
 free(depth);free(bgra);free(pristine);free(ram);return 0;}
