#include "scene_frame.h"
#include "draw_engine.h"
#include "matrix_stack.h"
#define rd32 nflstreet_rd32
static int draw_marks(void *user,uint32_t passes,nflstreet_render_state *s){
 nflstreet_scene *sc=user;nflstreet_draw d={&sc->draw,s};const nflstreet_matrix_stacks *k=&sc->marks.stacks;
 if(sc->venue.stacks)return nflstreet_marks_draw(&sc->marks,s); /* the pass loop has already translated the stack */
 const uint8_t *p=nflstreet_draw_at(&d,passes+4,12);if(!p)return 0;
 return nflstreet_matrix_select(&d,k)&&nflstreet_matrix_push(&d,k)&&
  nflstreet_matrix_translate(&d,k,nflstreet_as_float(rd32(p)),nflstreet_as_float(rd32(p+4)),nflstreet_as_float(rd32(p+8)))&&
  nflstreet_marks_draw(&sc->marks,s)&&nflstreet_matrix_pop(&d,k);}
static int draw_ball_shadows(void *user,nflstreet_render_state *s){nflstreet_scene *sc=user;return nflstreet_ball_shadows_draw(&sc->shadows,s);}
void nflstreet_scene_link(nflstreet_scene *sc){
 sc->venue.draw=&sc->draw;sc->hud.scene=&sc->venue;sc->marks.scene=&sc->venue;sc->shadows.hud=&sc->hud;sc->ball.shadows=&sc->shadows;
 sc->screens.hud=&sc->hud;sc->hooks.marks=draw_marks;sc->hooks.shadows=draw_ball_shadows;sc->hooks.user=sc;sc->venue.pass_hooks=&sc->hooks;}
int nflstreet_scene_venue(nflstreet_scene *sc,nflstreet_render_state *s){
 nflstreet_draw d={&sc->draw,s};uint32_t top;
 if(!nflstreet_camera_view(&sc->hud,sc->field_camera,s)||!nflstreet_matrix_top(&d,&sc->hud.stacks,&top))return 0;
 uint8_t *slot=nflstreet_draw_at(&d,sc->view_slot,0x40);const uint8_t *t=nflstreet_draw_at(&d,top,0x40);if(!slot||!t)return 0;
 memmove(slot,t,0x40);if(!nflstreet_draw_word(&d,sc->hud.stacks.current,&sc->venue.matrix_stack))return 0;
 if(!nflstreet_sky_draw(&sc->venue,s)||!nflstreet_field_passes_draw(&sc->venue,sc->field_passes,s)||!nflstreet_shadows_draw(&sc->venue,s))return 0;
 for(unsigned i=0;i<4;++i)if(!nflstreet_line_draw(&sc->venue,sc->lines[i],s))return 0;
 for(unsigned i=0;i<3;++i)if(!nflstreet_ring_draw(&sc->venue,sc->rings[i],s))return 0;
 return nflstreet_field_draw(&sc->venue,sc->main_field,s)&&nflstreet_ball_draw(&sc->ball,s);}
int nflstreet_scene_players(nflstreet_scene *sc,nflstreet_render_state *s){
 nflstreet_draw d={&sc->draw,s};
 for(uint32_t i=0;i<sc->actor_count;++i){uint32_t actor;if(!nflstreet_draw_word(&d,sc->actors+4*i,&actor)||!nflstreet_player_draw(&sc->draw,actor,s))return 0;}
 return 1;}
int nflstreet_scene_hud(nflstreet_scene *sc,nflstreet_render_state *s){
 nflstreet_draw d={&sc->draw,s};uint32_t camera;
 if(!nflstreet_camera_view(&sc->hud,sc->hud_camera,s))return 0;
 for(uint32_t i=0;i<sc->name_tag_count;++i)if(!nflstreet_name_tag_draw(&sc->hud,sc->name_tags+0x24u*i,s))return 0;
 return nflstreet_draw_word(&d,sc->screen_camera_word,&camera)&&nflstreet_hud_screens_draw(&sc->screens,camera,s);}
