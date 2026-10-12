#ifndef NFLSTREET_MESH_RENDER_H
#define NFLSTREET_MESH_RENDER_H
#include "model.h"
#include "texture.h"
/* Inspection camera and software rasterizer, not the original game renderer.
   Orthographic fit, depth test, flat light. No original material/TEV emulation. */
int nflstreet_mesh_render(const nflstreet_mesh *,unsigned,unsigned,float,nflstreet_texture *);
int nflstreet_mesh_render_view(const nflstreet_mesh *,unsigned,unsigned,float,float,float,nflstreet_texture *);
/* Opt-in source Y-up view for stored actor geometry; defaults stay Z-up. */
int nflstreet_mesh_render_view_y_up(const nflstreet_mesh *,unsigned,unsigned,float,float,float,nflstreet_texture *);
#endif
