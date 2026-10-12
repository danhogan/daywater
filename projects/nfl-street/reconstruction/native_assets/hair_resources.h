#ifndef NFLSTREET_HAIR_RESOURCES_H
#define NFLSTREET_HAIR_RESOURCES_H
#include "archive.h"
#include "model_view.h"
/* Three original PLYRMODEL descriptor indices. Derived headwear state remains
   explicit: selection of that state from DB/LCGP is not owned here. */
typedef struct {uint32_t indices[3];} nflstreet_hair_plan;
int nflstreet_hair_plan_make(const nflstreet_archive *roster,const char *name,
    uint16_t plht,uint16_t phar,uint16_t headwear_state,nflstreet_hair_plan *);
typedef struct {
    uint8_t *bytes[3];size_t sizes[3];uint32_t tokens[3],indices[3];
    unsigned archive_indices[3];uint8_t roster_archive[3],views[3][52];
} nflstreet_hair_resources;
/* Own decoded/relocated files and complete views. Zero indices remain absent;
   FFFF rejects; index50000 retains local routing. All slots are independent (no adjacency guess).
   Does not bind textures, skeletons, original descriptors or draw state. */
int nflstreet_hair_resources_open(const nflstreet_archive *local,
    const nflstreet_archive *roster,const nflstreet_hair_plan *,uint32_t,
    nflstreet_hair_resources *);
void nflstreet_hair_resources_close(nflstreet_hair_resources *);
#endif
