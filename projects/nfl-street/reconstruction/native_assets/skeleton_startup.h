#ifndef NFLSTREET_SKELETON_STARTUP_H
#define NFLSTREET_SKELETON_STARTUP_H
#include "skeleton_resource.h"
typedef struct {
 const uint8_t *base_positions,*hand_positions,*hair_positions;
 uint32_t scale_word;
} nflstreet_skeleton_startup_catalog;
/* Borrow a private extracted NSC1 data file; no original game bytes embedded
   in code and no DOL needed by the native runtime. */
int nflstreet_skeleton_startup_catalog_open(const uint8_t *,size_t,nflstreet_skeleton_startup_catalog *);
typedef struct {
 nflstreet_skeleton_resource resources[6];
 uint32_t base_vectors[6];
} nflstreet_skeleton_startup;
/* Own original80041108 relocation, position copies, scale writes and paired
   skeleton mirroring. All six resources are owned until close. Output is
   unchanged on failure; identity ranges must not overlap other caller storage. */
int nflstreet_skeleton_startup_open(const nflstreet_archive *,const nflstreet_skeleton_startup_catalog *,
 uint32_t identity_base,nflstreet_skeleton_startup *);
void nflstreet_skeleton_startup_close(nflstreet_skeleton_startup *);
#endif
