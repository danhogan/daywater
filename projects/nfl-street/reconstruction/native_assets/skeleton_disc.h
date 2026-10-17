#ifndef NFLSTREET_SKELETON_DISC_H
#define NFLSTREET_SKELETON_DISC_H
#include "skeleton_startup.h"
typedef struct nflstreet_skeleton_disc_source nflstreet_skeleton_disc_source;
/* Host preparation for pinned GNNE69 rev0. Copies initialized DOL constants and
   owns animdata.dat read through disc_files. No generated NSC1/runtime snapshot. */
nflstreet_skeleton_disc_source *nflstreet_skeleton_disc_open(const char *iso);
void nflstreet_skeleton_disc_close(nflstreet_skeleton_disc_source *);
/* Borrowed pointers stay valid until source close. Output untouched on failure. */
int nflstreet_skeleton_disc_catalog(const nflstreet_skeleton_disc_source *,nflstreet_skeleton_startup_catalog *);
/* Forward to existing verified six-resource consumer. Returned resources are
   independent owned copies, remain valid after source close; caller closes them
   with nflstreet_skeleton_startup_close. Output must be zero-initialized/closed;
   nonempty output is rejected to avoid overwriting owned resources.
   Failure leaves output untouched. */
int nflstreet_skeleton_disc_startup(const nflstreet_skeleton_disc_source *,uint32_t identity,nflstreet_skeleton_startup *);
#endif
