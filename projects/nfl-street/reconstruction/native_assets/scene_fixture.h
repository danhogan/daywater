#ifndef NFLSTREET_SCENE_FIXTURE_H
#define NFLSTREET_SCENE_FIXTURE_H
/* The saved match's scene from saved globals (venue, HUD, marks, ball and
   player contexts). The including file defines R13, ram_at, ram32 and ramf
   and includes venue_fixture.h and hud_fixture.h. */
#include "scene_frame.h"
#include "light_rig_fixture.h"
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
 /* and rebuild the players' palettes */
 static nflstreet_actor_palette_context palettes;memset(&palettes,0,sizeof palettes);palettes.x=x;palettes.stacks=sc->hud.stacks;
 palettes.unit[0]=ramf(ram,0x803a3958u);palettes.unit[1]=ramf(ram,0x803a395cu);palettes.pool=R13-0x5680u;palettes.pool_index=R13-0x5678u;
 palettes.pool_used=R13-0x5670u;palettes.cache_on=R13-0x6200u;sc->palettes=&palettes;
 /* the light update and the crowd (crowd_draw.md: 45 entries from 80C4E094) */
 static nflstreet_light_rig_context lights;lights=light_rig_context(ram,x);sc->lights=&lights;
 nflstreet_crowd_context *c=&sc->crowd;c->enable=R13-0x72a4u;c->view_slot=0x80331ec0u+0xc0u;c->cull_box=0x8028e2b4u;
 c->game_mode=0x802a301cu;c->sides=ram32(ram,R13-0x5ba4u);c->hide_limit=0x802a3050u;c->scratch=R13-0x6430u;
 c->hide_sign[0]=ramf(ram,0x80260f2cu);c->hide_sign[1]=ramf(ram,0x80260f30u);c->light_default[0]=ramf(ram,0x80260f04u);c->light_default[1]=ramf(ram,0x80260f54u);
 c->palette_scale[0]=ramf(ram,0x80260f1cu);c->palette_scale[1]=ramf(ram,0x80260f58u);c->normal_scale=ramf(ram,0x8026381cu);c->quat_one=ramf(ram,0x802634d8u);
 sc->crowd_first=0x80c4e094u;sc->crowd_count=45;
 /* the camera slots and name-tag projection (player_markers.md) */
 static nflstreet_view_context view;memset(&view,0,sizeof view);view.x=x;view.stacks=venue_stacks(ram);view.r13=R13;view.slots=0x80331ec0u;
 sc->view=&view;sc->camera_node_word=R13-0x4eecu;
 /* the actor fans, strips and particles (effects_draw.md) and the scaled-actor byte */
 sc->effects=1;sc->r13=R13;sc->fans.byte_scale=ramf(ram,0x80260b88u);sc->fans.depth[0]=ramf(ram,0x80260b8cu);sc->fans.depth[1]=ramf(ram,0x80260b90u);
 nflstreet_strips_context *st=&sc->strips;st->actors=ram32(ram,R13-0x7bbcu);st->actor_count=R13-0x7bb8u;st->gate=R13-0x72c0u;st->scaled_gate=R13-0x7238u;
 st->history=0x80270ca4u;st->byte_scale=ramf(ram,0x80260438u);
 nflstreet_particles_context *pt=&sc->particles;pt->enable=R13-0x7304u;pt->count=R13-0x7310u;pt->list=ram32(ram,R13-0x730cu);
 pt->gate_72fc=R13-0x72fcu;pt->gate_58bc=R13-0x58bcu;pt->shader_flat=ram32(ram,R13-0x68f8u);pt->shader_textured=ram32(ram,R13-0x68fcu);pt->corners=0x802898c0u;
 pt->color_unit=ramf(ram,0x802603d8u);pt->zero=ramf(ram,0x802603dcu);pt->one=ramf(ram,0x802603e0u);pt->angle_scale=ramf(ram,0x802603e4u);
 pt->byte_scale=ramf(ram,0x802603e8u);pt->cell[0]=ramf(ram,0x80261218u);pt->cell[1]=ramf(ram,0x8026121cu);pt->fade_one=ramf(ram,0x8026323cu);
 nflstreet_scene_link(sc);}
#endif
