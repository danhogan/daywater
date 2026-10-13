#ifndef NFLSTREET_HUD_FIXTURE_H
#define NFLSTREET_HUD_FIXTURE_H
/* Check-only: the saved match's name tags and the HUD context from saved
   globals. The 14 name tags (0x24 bytes apart) are the objects the original
   frame passes to 800DADC0; their font must be an FNTG font. The including
   check defines R13, ram_at and ram32 (venue_fixture.h). */
#include "hud_draw.h"
#include "uis_draw.h"
#include "ball_draw.h"
#define HUD_NAME_TAGS 0x8091c568u
#define HUD_NAME_TAG_COUNT 14u
/* The frame's three camera nodes (801DEF50): the field, the HUD with the
   name tags, and the overlay last processed (whose view the saved stack
   holds). */
static const uint32_t hud_cameras[3]={0x806a1ca8u,0x806a1db0u,0x806a1ba0u};
#define HUD_CAMERA 0x806a1db0u
static uint32_t hud_name_tag(unsigned i){return HUD_NAME_TAGS+0x24u*i;}
static int hud_font_present(uint8_t *ram,uint32_t font){
 const uint8_t *h=ram_at(ram,ram32(ram,font+4),4);return h&&!memcmp(h,"FNTG",4);}
static int hud_present(uint8_t *ram){
 for(unsigned i=0;i<HUD_NAME_TAG_COUNT;++i)if(!hud_font_present(ram,ram32(ram,hud_name_tag(i)+0x14)))return 0;return 1;}
static nflstreet_hud_context hud_context(uint8_t *ram,const nflstreet_scenery_context *scene){
 nflstreet_hud_context h;memset(&h,0,sizeof h);h.scene=scene;
 h.tag_enable=R13-0x7240u;h.tag_black=R13-0x723fu;h.option_table=0x8030b2acu;h.option_override=R13-0x6334u;h.option_mode=R13-0x6ff0u;
 h.profile_base=ram32(ram,R13-0x6608u);h.profile_current=R13-0x6618u;h.profile_next=R13-0x6610u;
 h.text_state=0x8039c480u;h.text_boxes=0x8039c4bcu;h.text_ready=R13-0x6650u;h.error_hook=ram32(ram,R13-0x64b8u);h.error_code=R13-0x64bcu;
 h.stacks.table=ram32(ram,R13-0x65f0u);h.stacks.count=ram32(ram,R13-0x65ecu);h.stacks.current=R13-0x65e8u;h.stacks.index=R13-0x4ee8u;
 h.stacks.sine=0x8039e100u;h.stacks.angle_scale[0]=ramf(ram,0x802617a0u);h.stacks.angle_scale[1]=ramf(ram,0x802617b0u);
 h.camera_types=ram32(ram,R13-0x5780u);h.screen_camera=0x8017db70u;h.field_camera=0x800c3508u;
 h.tluts[0]=0x80331460u;h.tluts[1]=0x80331480u;h.tag_offset=ramf(ram,0x80252b68u);
 return h;}
static nflstreet_uis_context uis_context(uint8_t *ram,const nflstreet_hud_context *hud){
 nflstreet_uis_context u;memset(&u,0,sizeof u);u.hud=hud;
 u.manager=ram32(ram,R13-0x63e0u+4);u.tint=0x8034cdb8u;u.tint_start[0]=ramf(ram,0x8026396cu);u.tint_start[1]=ramf(ram,0x80263970u);
 u.color_depth=R13-0x5648u;u.color_current=R13-0x63c8u;
 u.color_limits[0]=ramf(ram,0x80263b70u);u.color_limits[1]=ramf(ram,0x80263b74u);u.color_limits[2]=ramf(ram,0x80263b78u);
 u.degrees=ramf(ram,0x80263b80u);u.zero=ramf(ram,0x80263b7cu);
 u.text_mode=R13-0x63c4u;u.draw_mode=R13-0x63c0u;u.text_mode_on=R13-0x63bcu;u.channel_cache=R13-0x565cu;
 u.color_cache[0]=R13-0x5664u;u.color_cache[1]=R13-0x5660u;u.light=0x8039e700u;u.texture=0x8039e740u;u.tlut=0x8039e780u;
 u.texture_matrix=0x80293decu;u.palette_cache[0]=R13-0x564cu;u.palette_cache[1]=R13-0x5650u;u.palette_cache[2]=R13-0x5658u;
 u.palette_cache[3]=R13-0x5654u;u.mark=R13-0x566cu;u.resources_up=R13-0x6538u;u.notify=ram32(ram,R13-0x63d8u);u.refresh=R13-0x63a8u;
 u.color_unit=ramf(ram,0x80263ba8u);u.text_limit=ramf(ram,0x80263bb8u);u.lod=ramf(ram,0x80263b98u);
 u.texobj.lod_scale=ramf(ram,R13-0x4bf0u);u.texobj.bias_min=ramf(ram,R13-0x4be0u);u.texobj.bias_cap=ramf(ram,R13-0x4bdcu);
 u.texobj.bias_max=ramf(ram,R13-0x4bd8u);u.texobj.bias_scale=ramf(ram,R13-0x4bd4u);u.texobj.lod_min=ramf(ram,R13-0x4bd0u);
 u.texobj.lod_max=ramf(ram,R13-0x4bccu);memcpy(u.texobj.filters,ram_at(ram,R13-0x6090u,6),6);
 u.image_class=0x801d60dcu;u.text_class=0x801d9244u;u.quiet_classes[0]=0x8012cb94u;u.quiet_classes[1]=0x8012c5d8u;u.transform=0x801d8398u;
 u.camera=R13-0x6cd8u;u.screens[0]=R13-0x6c84u;u.screens[1]=R13-0x6c94u;u.screens[2]=R13-0x6c88u;u.screens[3]=R13-0x6c8cu;
 u.screen_flags=R13-0x6c98u;u.pending_on=R13-0x6cf0u;u.pending=0x8030834cu;u.refresh_hook=0x80381c40u;u.refresh_target=0x800044a4u;
 u.after_second=R13-0x4f80u;
 return u;}
static inline nflstreet_ball_context ball_context(uint8_t *ram,const nflstreet_hud_context *hud){
 nflstreet_ball_context c;memset(&c,0,sizeof c);c.hud=hud;
 c.enabled=R13-0x58d8u;c.count=R13-0x6834u;c.entries=0x80310898u;c.texture=0x80310a58u;c.lights=0x8034fd8cu;
 c.unit[0]=ramf(ram,0x803a3958u);c.unit[1]=ramf(ram,0x803a395cu);c.axis[0]=ramf(ram,0x80268de0u);c.axis[1]=ramf(ram,0x80268de4u);
 static const uint32_t k[5]={0x80260d88u,0x80260d8cu,0x80260d90u,0x80260d6cu,0x80260d70u};for(unsigned i=0;i<5;++i)c.consts[i]=ramf(ram,k[i]);
 for(unsigned i=0;i<3;++i)c.recip[i]=ramf(ram,0x802617d8u+4*i);
 return c;}
/* The ball the original frame draws (8015CB60). */
#define HUD_BALL 0x80deee24u
static inline nflstreet_ball_model_context ball_model_context(uint8_t *ram,const nflstreet_ball_context *shadows){
 nflstreet_ball_model_context c;memset(&c,0,sizeof c);c.shadows=shadows;c.ball=HUD_BALL;c.box=0x8028ca20u;c.lod=0x803897d8u;c.slots=0x80331ec0u;
 c.bounds_init[0]=ramf(ram,0x8026389cu);c.bounds_init[1]=ramf(ram,0x802638a0u);
 static const uint32_t k[6]={0x80260614u,0x80260618u,0x80260628u,0x8026062cu,0x80260630u,0x80260634u};for(unsigned i=0;i<6;++i)c.k[i]=ramf(ram,k[i]);
 c.refresh_hook=0x80381c40u;c.refresh_target=0x800044a4u;
 return c;}
#endif
