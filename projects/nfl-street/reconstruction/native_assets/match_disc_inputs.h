#ifndef NFLSTREET_MATCH_DISC_INPUTS_H
#define NFLSTREET_MATCH_DISC_INPUTS_H
#include "match_skeletons.h"
/* Host composition owner; no original match-entry or allocation reconstruction.
   Retains disc-derived catalogs through every borrowed visual name/view. */
typedef struct nflstreet_match_disc_inputs nflstreet_match_disc_inputs;
/* *out must be NULL. Session, resolver, stable body pools, headwear, hair,
   texture/dynamic archives and flush opaque inputs remain borrowed and must
   outlive the owner. Derived gear/color/dynclut tables are built from session's
   STAT and the common dynamic archive; supplied versions/catalog pointers are
   replaced in local context copies. All players must share bodies and dynamic.
   Queues real flush callbacks until both constructors succeed. A real callback
   failure preserves*out but earlier real callbacks may have side effects. */
int nflstreet_match_disc_inputs_open(const char *iso,const nflstreet_quick_match_session *,
 const nflstreet_match_visual_input[14],const nflstreet_match_skeleton_input[14],nflstreet_match_disc_inputs **out);
const nflstreet_match_visuals *nflstreet_match_disc_inputs_visuals(const nflstreet_match_disc_inputs *);
const nflstreet_match_skeletons *nflstreet_match_disc_inputs_skeletons(const nflstreet_match_disc_inputs *);
const nflstreet_gear_texture_catalog *nflstreet_match_disc_inputs_gear(const nflstreet_match_disc_inputs *);
const nflstreet_slot_material_catalog *nflstreet_match_disc_inputs_materials(const nflstreet_match_disc_inputs *);
/* Close visuals, skeleton borrowers, then input sources. AcceptsNULL. Views
   and names from this object become invalid. Do not shallow-copy owner. */
void nflstreet_match_disc_inputs_close(nflstreet_match_disc_inputs *);
#endif
