#ifndef NFLSTREET_VENUE_FIXTURE_H
#define NFLSTREET_VENUE_FIXTURE_H
/* Check-only: the EARS venue objects of the saved match and the scenery
   context from saved globals. The objects are the ones the original scene
   traversal draws (main field 8094D31C through 80165DC4, field passes
   80A9753C through 8015B670); their models must carry the EARS names. The
   including check defines R13, ram_at, ram32 and ramf. */
#include "scenery_draw.h"
#include "marks_draw.h"
#define VENUE_MAIN_FIELD 0x8094d31cu
#define VENUE_FIELD_PASSES 0x80a9753cu
/* The three player-select ring objects the traversal draws (800E3F7C). */
static const uint32_t venue_rings[3]={0x80ddfcd0u,0x80de01e4u,0x80de02e8u};
/* The four field line objects (800DA074), 0x124 bytes apart. */
static const uint32_t venue_lines[4]={0x80dec17cu,0x80dec2a0u,0x80dec3c4u,0x80dec4e8u};
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
 c.game_state=ram32(ram,R13-0x7078u);c.gate_7b88=ram32(ram,R13-0x7b88u);c.gate_7f40=ram32(ram,R13-0x7f40u);c.gate_7258=R13-0x7258u;
 c.gate_count=0x80308560u;c.mode_source=ram32(ram,R13-0x7cf8u);c.mode_cache=R13-0x7cf4u;c.line_ratio=R13-0x7260u;c.line_box=0x802714d0u;
 c.ring_flags=R13-0x71dcu;c.ring_colors=0x8027bed4u;c.ring_scale=ramf(ram,0x802536ecu);
 c.actors=ram32(ram,R13-0x7bbcu);c.actor_count=R13-0x7bb8u;c.shadow_texture=ram32(ram,R13-0x58e4u);c.shadow_tlut=ram32(ram,R13-0x58e0u);
 return c;}
/* Ground marks (800D75EC) on the viewport's matrix stack. */
static inline nflstreet_marks_context marks_context(uint8_t *ram,const nflstreet_scenery_context *scene){
 nflstreet_marks_context c;memset(&c,0,sizeof c);c.scene=scene;
 c.stacks.table=ram32(ram,R13-0x65f0u);c.stacks.count=ram32(ram,R13-0x65ecu);c.stacks.current=R13-0x65e8u;c.stacks.index=R13-0x4ee8u;
 c.stacks.sine=0x8039e100u;c.stacks.angle_scale[0]=ramf(ram,0x802617a0u);c.stacks.angle_scale[1]=ramf(ram,0x802617b0u);
 c.enabled=R13-0x7280u;c.newest=R13-0x5b60u;c.lists=0x802aa7c4u;c.counts=0x80271268u;c.handlers=0x8028ccb4u;c.fog_saved=R13-0x58f0u;
 c.ramp_step=ramf(ram,0x802529c8u);c.ramp_cap=ramf(ram,0x802529ccu);c.alpha_scale=ramf(ram,0x8026072cu);
 for(unsigned i=0;i<3;++i)c.depth[i]=ramf(ram,0x80260720u+4*i);for(unsigned i=0;i<6;++i)c.quad[i]=ramf(ram,0x80260730u+4*i);
 c.type_begin[0]=0x8015fd28u;c.type_begin[1]=0x8015fd84u;c.type2[0]=0x80160104u;c.type2[1]=0x80160298u;c.type2[2]=0x80160250u;
 return c;}
#endif
