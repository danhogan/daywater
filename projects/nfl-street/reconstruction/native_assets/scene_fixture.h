#ifndef NFLSTREET_SCENE_FIXTURE_H
#define NFLSTREET_SCENE_FIXTURE_H
/* The saved match's scene from saved globals (venue, HUD, marks, ball and
   player contexts). The including file defines R13, ram_at, ram32 and ramf
   and includes venue_fixture.h and hud_fixture.h. */
#include "scene_frame.h"
/* The head display-list compile behind 801626E0 is a supplied service: the
   saved lists are used as compiled. */
static inline int scene_face(void *p,uint32_t actor,int *compiled){(void)p;(void)actor;*compiled=0;return 1;}
static inline void scene_fixture(uint8_t *ram,nflstreet_scene *sc,nflstreet_draw_span span,nflstreet_gx_emit emit,void *sink){
 memset(sc,0,sizeof *sc);nflstreet_player_draw_context *x=&sc->draw;
 x->span=span;x->memory=ram;x->emit=emit;x->sink=sink;x->face=scene_face;x->shared_model=0x803102b4u;x->player_records=0x802aaa60u;
 x->light_sets=0x8034fe9cu;x->default_texture=ram32(ram,R13-0x640cu);x->null_shader=ram32(ram,R13-0x5684u);
 x->setups[0]=0x801c9d0cu;x->setups[1]=0x801c9dacu;x->setups[2]=0x801c9dfcu;x->setups[3]=0x801c9d5cu;
 x->dim[0]=ramf(ram,R13-0x6850u);x->dim[1]=ramf(ram,R13-0x684cu);
 sc->venue=venue_context(ram,x);sc->hud=hud_context(ram,&sc->venue);sc->marks=marks_context(ram,&sc->venue);
 sc->shadows=ball_context(ram,&sc->hud);sc->ball=ball_model_context(ram,&sc->shadows);sc->screens=uis_context(ram,&sc->hud);
 sc->field_camera=hud_cameras[0];sc->hud_camera=HUD_CAMERA;sc->main_field=VENUE_MAIN_FIELD;sc->field_passes=VENUE_FIELD_PASSES;
 memcpy(sc->lines,venue_lines,sizeof sc->lines);memcpy(sc->rings,venue_rings,sizeof sc->rings);
 sc->name_tags=HUD_NAME_TAGS;sc->name_tag_count=HUD_NAME_TAG_COUNT;sc->actors=ram32(ram,R13-0x7bbcu);sc->actor_count=14;
 sc->screen_camera_word=R13-0x6cd8u;sc->view_slot=0x80331ec0u+0xc0u;
 sc->venue.stacks=&sc->hud.stacks; /* recompute venue matrices from the camera, as the original frame does */
 nflstreet_scene_link(sc);}
#endif
