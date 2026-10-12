#ifndef NFLSTREET_MODEL_H
#define NFLSTREET_MODEL_H
#include <stddef.h>
#include <stdint.h>
typedef struct {float position[3],uv[2];uint8_t rgba[4];} nflstreet_mesh_vertex;
typedef struct {nflstreet_mesh_vertex vertex[3];uint16_t material;uint32_t source_offset;} nflstreet_mesh_triangle;
typedef struct {
    char shader[64],texture_name[128];int texture_index,palette_index;
    const uint8_t *texture_bytes;size_t texture_size;
} nflstreet_mesh_material;
typedef struct {
    nflstreet_mesh_triangle *triangles;
    size_t count;
    unsigned draws;
    float minimum[3],maximum[3];
    char name[128];
    const uint8_t *texture_bytes;
    size_t texture_size;
    nflstreet_mesh_material *materials;
    unsigned material_count;
    unsigned ignored_color_indices;
    int inspection_opaque;
} nflstreet_mesh;
/* Read original TMdl static geometry. Original model-space coordinates.
   Unsupported matrix/skin, direct attributes and GX state commands fail.
   Does not reconstruct original materials/shaders, posing or scene state. */
int nflstreet_mesh_open(const uint8_t *,size_t,nflstreet_mesh *);
/* Inspection-only option: ignore declared-array color index overflow while
   still requiring file bounds. Never relax position/UV or skin validation.
   Counts such vertices; does not interpret their original color behavior. */
int nflstreet_mesh_open_inspection(const uint8_t *,size_t,nflstreet_mesh *);
/* Original cluster hierarchy in its stored pose, without animation input.
   Host trigonometry approximates original math. Static APIs stay strict. */
int nflstreet_mesh_open_bind_pose(const uint8_t *,size_t,nflstreet_mesh *);
/* Bounded static render path: consumes RGBA8 colors; color array count
   overflow remains file-bounded and must be verified against reference data. */
int nflstreet_mesh_open_render(const uint8_t *,size_t,nflstreet_mesh *);
void nflstreet_mesh_close(nflstreet_mesh *);
/* Private diagnostics: explicit little-endian triangle/vertex floats. */
int nflstreet_mesh_write(const nflstreet_mesh *,const char *);
#endif
