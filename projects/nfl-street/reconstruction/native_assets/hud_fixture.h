#ifndef NFLSTREET_HUD_FIXTURE_H
#define NFLSTREET_HUD_FIXTURE_H
/* Check-only: the saved match's name tags and the HUD context from saved
   globals. The 14 name tags (0x24 bytes apart) are the objects the original
   frame passes to 800DADC0; their font must be an FNTG font. The including
   check defines R13, ram_at and ram32 (venue_fixture.h). */
#include "hud_draw.h"
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
 h.camera_types=ram32(ram,R13-0x5780u);h.screen_camera=0x8017db70u;
 h.tluts[0]=0x80331460u;h.tluts[1]=0x80331480u;h.tag_offset=ramf(ram,0x80252b68u);
 return h;}
#endif
