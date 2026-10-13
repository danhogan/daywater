#ifndef NFLSTREET_VENUE_FIXTURE_H
#define NFLSTREET_VENUE_FIXTURE_H
/* Check-only: the EARS venue objects of the saved match and the scenery
   context from saved globals. The objects are the ones the original scene
   traversal draws (main field 8094D31C through 80165DC4, field passes
   80A9753C through 8015B670); their models must carry the EARS names. The
   including check defines R13, ram_at, ram32 and ramf. */
#include "scenery_draw.h"
#define VENUE_MAIN_FIELD 0x8094d31cu
#define VENUE_FIELD_PASSES 0x80a9753cu
static int venue_present(uint8_t *ram){
 static const char *names[2]={"EARS_rField.ea3","EARS_FieldPass1.ea3"};
 const uint32_t models[2]={ram32(ram,VENUE_MAIN_FIELD+0x24+0x54),ram32(ram,VENUE_FIELD_PASSES+0x3d8)};
 for(unsigned i=0;i<2;++i){const uint8_t *name=ram_at(ram,ram32(ram,models[i]+0x14),32);
  if(!name||memcmp(name,names[i],strlen(names[i])+1))return 0;}
 return 1;}
static nflstreet_scenery_context venue_context(uint8_t *ram,const nflstreet_player_draw_context *x){
 nflstreet_scenery_context c;memset(&c,0,sizeof c);c.draw=x;
 c.enable=ram32(ram,R13-0x6230u);c.projection=0x80331ec0u+0x80u;c.cull_projection=0x80348ad4u;c.cull_modelview=0x80348b14u;
 c.fog=R13-0x6208u;c.fog_camera=ram32(ram,R13-0x6218u);c.fog_type=ram32(ram,R13-0x562cu);c.fog_color=ram32(ram,R13-0x620cu);
 c.fog_start=ramf(ram,R13-0x6214u);c.fog_end=ramf(ram,R13-0x6210u);c.fog_off=ramf(ram,0x80268e28u);c.depth_trim=ramf(ram,R13-0x622cu);
 c.pass_table=0x8028a004u;c.pass_offset=0x8028a090u;c.pass_alpha=ramf(ram,0x80260588u);c.pass3_alpha=ramf(ram,R13-0x58f4u);
 c.pass_color=ramf(ram,0x80260580u);c.pass_depth=ramf(ram,0x80260584u);
 c.matrix_stack=ram32(ram,R13-0x65e8u);c.sky=0x80385e8cu;c.sky_colors=0x80310a78u;c.sky_tables=0x8038668cu;
 c.actors=ram32(ram,R13-0x7bbcu);c.actor_count=R13-0x7bb8u;c.shadow_texture=ram32(ram,R13-0x58e4u);c.shadow_tlut=ram32(ram,R13-0x58e0u);
 return c;}
#endif
