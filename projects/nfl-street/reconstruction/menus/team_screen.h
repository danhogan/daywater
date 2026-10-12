#ifndef NFLSTREET_TEAM_SCREEN_H
#define NFLSTREET_TEAM_SCREEN_H
#include "team_callback.h"
#include "team_classes.h"
#include "team_actor.h"
#include "team_preview.h"
#include "team_activation.h"
#include "team_player_window_callback.h"
#include "options_screen.h"
#include "team_bootstrap.h"
#include "team_player_spawn.h"
#include "team_devices.h"
#include "team_transition.h"
#include "team_confirm.h"
/* Experimental complete-source integration. Default creation/activation
   requires authentic host inputs for roster/actor request80000012. The
   supplied callback12 bridge has controlled original comparisons. Supplied resource
   effects and roster projections do not establish original I/O or rendering. */
typedef struct {
 nflstreet_ui_options_screen classes;
 nflstreet_team_classes objects;
 nflstreet_team_callback game;
 nflstreet_team_actor actors;
 nflstreet_team_activation activation;
 nflstreet_team_confirm confirmation; /* Retained visibility/map effect owner. */
 /* Explicit host-supplied checked pose/formation stage, keyed to selected team.
    Not populated from invented ratings or the failed saved-match lookup. */
 uint32_t supplied_activation_mask,supplied_activation_team[2];
 nflstreet_team_player_window player_window;
 /* Ordered PLAY query IDs and roster triples must belong to the selected team. */
 uint32_t supplied_window_mask,supplied_window_team[2];
 struct {uint32_t id,count,players[20],assets[20];} supplied_rosters[32];
 uint32_t supplied_roster_count;
 /* Optional fresh original-data host. Byte views remain caller-owned. */
 const nflstreet_team_bootstrap *bootstrap;
 const uint8_t *roster_databases[32]; size_t roster_database_sizes[32];
 nflstreet_team_label_row label_rows[2][20];
 uint32_t player_callback;
 /* Original80129AB8 lookup values, supplied by the controller/device owner. */
 uint32_t port_mapping[10],supplied_port_mapping_mask;
 uint32_t controller_sides[4],supplied_controller_sides;
 /* Owned transition object and service inputs, supplied by the screen owner.
    Requests are retained; the menu host must consume audio/broadcast effects. */
 uint8_t transition_object[NFLSTREET_TEAM_TRANSITION_OBJECT_MIN_SIZE];
 uint32_t supplied_transition,transition_clock,transition_audio,transition_context;
 uint32_t transition_count;
 nflstreet_team_transition_effects transition_requests[16];
 nflstreet_team_player_spawn_request player_spawns[64];
 uint32_t player_spawn_count;
 nflstreet_team_preview_roster selected_rosters[2];
 struct {uint32_t kind,side,archive,entry;} player_resources[1024];
 uint32_t player_resource_count;
 uint32_t supplied_static_archives;
 uint32_t supplied_common_mode; /* Original8010B60C result, supplied by host. */
} nflstreet_team_screen;
int nflstreet_team_screen_service(void*,uint8_t*,size_t,uint32_t,const nflstreet_ui_service_request*,uint32_t*);
int nflstreet_team_screen_memory(void*,uint8_t*,size_t,uint32_t,const nflstreet_ui_service_request*,uint32_t*,uint32_t,uint32_t[256]);
int nflstreet_team_screen_construct(uint8_t*,size_t,uint32_t,nflstreet_team_screen*);
int nflstreet_team_screen_start(uint8_t*,size_t,uint32_t,nflstreet_team_screen*);
/* Opt-in manager-context startup. Loaded-entry arguments go to FFFE; the
   subsequent FFFC reactivation receives argc=0, matching the manager trace.
   The legacy wrapper below retains zero arguments for FFFE as well. */
int nflstreet_team_screen_start_context(uint8_t*,size_t,uint32_t,nflstreet_team_screen*,
 uint32_t current_screen,uint32_t linked_screen,uint32_t stack_base,uint32_t load_argument,
 unsigned loaded_argc,const uint32_t *loaded_args);
int nflstreet_team_screen_advance(uint8_t*,size_t,uint32_t,nflstreet_team_screen*,uint32_t);
/* Already translated source events and one original float input word.
   Physical controller translation, repeat and device discovery stay external. */
int nflstreet_team_screen_input(uint8_t*,size_t,uint32_t,nflstreet_team_screen*,uint32_t,uint16_t,float);
/* Explicit source event VM context; the legacy input entry point retains its
   established (7,5,0x817c0000) defaults. */
int nflstreet_team_screen_input_context(uint8_t*,size_t,uint32_t,
 nflstreet_team_screen*,uint32_t,uint16_t,float,
 uint32_t current_screen,uint32_t linked_screen,uint32_t stack_base);
/* Execute one host-supplied manager focus operation transactionally. The
   caller resolves the node index and supplies the registered VM identities,
   manager-owned stack base, argument and optional source index list. This is
   separate from source event-generated kind-7 requests handled by input(). */
int nflstreet_team_screen_focus_context(uint8_t*,size_t,uint32_t,
 nflstreet_team_screen*,unsigned node,unsigned selected,uint32_t argument,
 uint32_t current_screen,uint32_t linked_screen,uint32_t stack_base,
 uint32_t list,nflstreet_ui_focus_effects*);
/* Populate the original logical port mapping and iterator projection together.
   The caller owns physical device discovery and side assignment. */
int nflstreet_team_screen_devices(nflstreet_team_screen*,const nflstreet_team_device_record*,size_t,const uint32_t[10]);
#endif
