#ifndef NFLSTREET_PLAYER_H
#define NFLSTREET_PLAYER_H
#include "field_render.h"
typedef struct {
    nflstreet_mesh mesh;
    nflstreet_field_draw *draws;
    unsigned count;
    nflstreet_texture *textures;
    unsigned texture_count;
} nflstreet_player;
/* NAP1 supplies a fixed captured pose/light/color selection. Original TMdl
   geometry, normals and UVs plus MMAP index planes are loaded independently.
   Does not evaluate live skeleton animation or roster/appearance selection. */
int nflstreet_player_open(const char *,const uint8_t *,size_t,nflstreet_player *);
typedef struct {const uint8_t *bytes;size_t size;} nflstreet_player_source;
/* Repose an already loaded player using cached original model bytes and
   updated NAP1 matrices/lights. Texture ownership and draw selection stay
   with the player. Sources have one entry per draw. */
int nflstreet_player_repose(nflstreet_player *,const uint8_t *,size_t,const nflstreet_player_source *);
void nflstreet_player_close(nflstreet_player *);
/* Inspection close-up of the same fixed pose; does not change asset geometry. */
int nflstreet_player_portrait(const nflstreet_player *,nflstreet_texture *);
#endif
