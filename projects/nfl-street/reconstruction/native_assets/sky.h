#ifndef NFLSTREET_SKY_H
#define NFLSTREET_SKY_H
#include "model.h"
/* Private original constants: radius, degrees/segment, angle multiplier,
   four elevations, eight polynomial coefficients, angle scale, 15 RGB floats.
   Eight segments and five rings follow the measured original constructor. */
int nflstreet_sky_vertices(const float [31],nflstreet_mesh_vertex [41]);
int nflstreet_sky_mesh(const float [31],nflstreet_mesh *);
#endif
