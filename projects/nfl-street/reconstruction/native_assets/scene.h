#ifndef NFLSTREET_SCENE_H
#define NFLSTREET_SCENE_H
#include "archive.h"
#include "model.h"
#define NFLSTREET_SCENE_LAYERS 6
typedef struct {
    nflstreet_mesh mesh;
    uint8_t *owned[NFLSTREET_SCENE_LAYERS];
    unsigned entries[NFLSTREET_SCENE_LAYERS];
    size_t triangles[NFLSTREET_SCENE_LAYERS];
    unsigned layers;
    uint8_t *prop_owned[256];unsigned prop_owned_count;
    unsigned instances,rendered_instances,nonvisual_instances,unsupported_instances;
} nflstreet_scene;
/* Candidate scene composition: original <venue>_rField and five FieldPass
   meshes retain asset coordinates. Does not instantiate props, actors,
   particles, collision, original draw state or original scene scheduling. */
int nflstreet_scene_open(const nflstreet_archive *,const char *,nflstreet_scene *);
void nflstreet_scene_close(nflstreet_scene *);
/* Original version-105 instance records -> original HSH1 definition/model
   lookup -> supported unposed geometry. Unknown skin/render cases are counted. */
int nflstreet_scene_attach_props(nflstreet_scene *,const nflstreet_archive *,const nflstreet_archive *,const nflstreet_archive *);
#endif
