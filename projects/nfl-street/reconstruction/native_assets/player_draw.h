#ifndef NFLSTREET_PLAYER_DRAW_H
#define NFLSTREET_PLAYER_DRAW_H
#include <stdint.h>
/* GX SDK calls issued by the original player draw 80161E10 and scenery draw
   801EC8E4 (GNNE69 rev 0), in the order the original issues them. Arguments follow the public GX API
   order. Pointer arguments are tokens; the pointed-to values a call consumes
   (colors, light objects, texture matrices, viewport floats) are copied into
   data. Floats are IEEE single bit patterns. */
typedef enum {
 NFLSTREET_GX_SET_VTX_DESC,       /* 80201FB8 attribute, type */
 NFLSTREET_GX_CLEAR_VTX_DESC,     /* 802029A8 */
 NFLSTREET_GX_SET_VTX_ATTR_FMT,   /* 802029E0 format, attribute, count, type, fraction */
 NFLSTREET_GX_SET_ARRAY,          /* 802033CC attribute, base, stride */
 NFLSTREET_GX_SET_TEX_COORD_GEN,  /* 80203468 coordinate, function, source, matrix, normalize, post matrix */
 NFLSTREET_GX_SET_NUM_TEX_GENS,   /* 80203738 count */
 NFLSTREET_GX_SET_CULL_MODE,      /* 80204224 mode */
 NFLSTREET_GX_LOAD_LIGHT,         /* 80205158 light id; data = the 64-byte light object */
 NFLSTREET_GX_SET_CHAN_AMB_COLOR, /* 802051D4 channel, RGBA */
 NFLSTREET_GX_SET_CHAN_MAT_COLOR, /* 802052C8 channel, RGBA */
 NFLSTREET_GX_SET_NUM_CHANS,      /* 802053BC count */
 NFLSTREET_GX_SET_CHAN_CTRL,      /* 80205400 channel, enable, ambient, material, lights, diffuse, attenuation */
 NFLSTREET_GX_INIT_TEX_OBJ_TLUT,  /* 80205BCC texture object, TLUT name */
 NFLSTREET_GX_LOAD_TEX_OBJ,       /* 80205DA0 texture object, map */
 NFLSTREET_GX_LOAD_TLUT,          /* 80205E5C TLUT object, TLUT name */
 NFLSTREET_GX_SET_TEV_COLOR_IN,   /* 80206F1C stage, a, b, c, d */
 NFLSTREET_GX_SET_TEV_ALPHA_IN,   /* 80206F60 stage, a, b, c, d */
 NFLSTREET_GX_SET_TEV_COLOR_OP,   /* 80206FA4 stage, op, bias, scale, clamp, register */
 NFLSTREET_GX_SET_TEV_ALPHA_OP,   /* 8020700C stage, op, bias, scale, clamp, register */
 NFLSTREET_GX_SET_TEV_COLOR,      /* 80207074 register, RGBA */
 NFLSTREET_GX_SET_ALPHA_COMPARE,  /* 80207320 compare0, ref0, op, compare1, ref1 */
 NFLSTREET_GX_SET_TEV_ORDER,      /* 802073F8 stage, coordinate, map, channel */
 NFLSTREET_GX_SET_NUM_TEV_STAGES, /* 802075D0 count */
 NFLSTREET_GX_SET_BLEND_MODE,     /* 8020791C type, source, destination, logic op */
 NFLSTREET_GX_CALL_DISPLAY_LIST,  /* 80207C38 list, bytes */
 NFLSTREET_GX_SET_CURRENT_MTX,    /* 80207F1C matrix id */
 NFLSTREET_GX_LOAD_TEX_MTX,       /* 80207F54 matrix id, type (1 = 2x4); data = 8 floats */
 NFLSTREET_GX_SET_VIEWPORT,       /* 8020810C; data = x, y, width, height, near, far */
 NFLSTREET_GX_LOAD_POS_MTX,       /* 80207E7C matrix id; data = 12 floats (3x4) */
 NFLSTREET_GX_LOAD_NRM_MTX,       /* 80207ECC matrix id; data = 9 floats (rows' first three of a 3x4) */
 NFLSTREET_GX_SET_Z_MODE,         /* 802079C8 enable, compare, update */
 NFLSTREET_GX_SET_Z_COMP_LOC,     /* 802079FC before texturing */
 NFLSTREET_GX_SET_FOG,            /* 80207600 type; data = color, start, end, near, far */
 NFLSTREET_GX_BEGIN,              /* 80203FC0 primitive, vertex format, vertex count */
 NFLSTREET_GX_WRITE,              /* immediate data written to the GX pipe 0xCC008000: bytes (1, 2, 4), value */
 NFLSTREET_GX_SET_TEV_OP,         /* 80206E90 stage, mode (modulate, decal, blend, replace, pass color) */
 NFLSTREET_GX_OPS
} nflstreet_gx_op;
typedef struct {nflstreet_gx_op op;uint32_t arg[7];uint32_t data[16];} nflstreet_gx_call;
/* Render-engine state the original keeps at 8039E560..8039E5F0, r13-6220,
   r13-5630 and 80293A54..80293A60, as far as the player draw touches it.
   The caller supplies viewport[0..3] (camera); 80161E10 resets or writes
   everything else before reading it. Fields named model_xx are the model
   view words 801C7914 copies without the player draw reading them. */
typedef struct {
 uint32_t chunks;         /* 8039E560 model +08: chunk table {chunks, count, arrays} */
 uint32_t palette;        /* 8039E564 matrix palette (801C79E4) */
 uint32_t model_10;       /* 8039E568 */
 uint32_t groups;         /* 8039E56C model +14: group table */
 uint32_t materials;      /* 8039E570 material instance array {count, pointers} */
 uint32_t instance;       /* 8039E574 applied material instance */
 uint32_t shader;         /* 8039E578 applied shader object */
 uint32_t blend;          /* 8039E57C blend preset (801C7DC0) */
 uint32_t overrides;      /* 8039E580 model +1C: vertex array base overrides */
 float viewport[6];       /* 8039E584 x, y, width, height, near, far */
 uint32_t effects;        /* 8039E59C chunk effect count */
 uint32_t format;         /* 8039E5C0 current vertex format id */
 uint32_t model_04,model_24,model_28,model_30,model_20; /* 8039E5C4/C8/CC/D0/E8 */
 uint32_t override;       /* 8039E5EC material-apply override callback */
 uint32_t lod;            /* 8039E5F0 chunk LOD index */
 uint32_t light_set;      /* r13-6220 light set used by the last draw */
 uint32_t light_mask;     /* r13-5630 lights loaded by the last draw */
 uint32_t matrix_cache[4];/* 80293A54..80293A60 */
} nflstreet_render_state;
/* Bounded view of one original-layout object. A token is a namespace
   identity; 0 is the null pointer. Returning NULL rejects the draw. */
typedef uint8_t *(*nflstreet_draw_span)(void *,uint32_t token,uint32_t size);
typedef int (*nflstreet_gx_emit)(void *,const nflstreet_gx_call *);
/* Original 801626E0: the head display-list compile for LOD 0 when the draw
   flags have bit 3. It is not modeled; the service reports whether the
   shared head model was compiled (1) or the actor's own model is used (0). */
typedef int (*nflstreet_face_compile)(void *,uint32_t actor,int *compiled);
typedef struct {
 nflstreet_draw_span span;void *memory;
 nflstreet_gx_emit emit;void *sink;
 nflstreet_face_compile face;void *face_context;
 uint32_t shared_model;     /* 803102B4 compiled head model view */
 uint32_t player_records;   /* 802AAA60, 0x68 bytes per record slot */
 uint32_t light_sets;       /* 8034FE9C, 0x108 bytes per light set */
 uint32_t default_texture;  /* value of r13-640C */
 uint32_t null_shader;      /* value of r13-5684 (801C8B08(0)) */
 uint32_t setups[4];        /* shader setup callbacks: NULL 801C9D0C, OnePass 801C9DAC, TwoPass 801C9DFC, Flat 801C9D5C */
 float dim[2];              /* r13-6850, r13-684C: far-depth scale without/with actor flag 3 */
} nflstreet_player_draw_context;
/* Original 80161E10(actor) for one player. It resets the render state
   (801C7648), scales the actor's light set 1 by the interpolated light color
   (801EDC60), sets the channel, alpha-compare, cull and blend presets and
   draws the twelve model instances at actor +4AC..+EFC in the original order
   through 801EC140. 80162220 draws the head: instance +59C for LOD 1 and 2,
   or for LOD 0 the head model's groups 3 and 4 (eyes, with texture matrices
   from 8016373C) and 2. Each chunk applies its material instance
   (801C7D54/801C7B58: shader setup, stage textures through 801C7AD4 and
   shader parameters), sets vertex descriptors and arrays (801CCBD8/801C7F18)
   and calls its display list. Instances with pending group selectors rebuild
   their chunk lists first (801EC0D4).

   The actor and instance fields the original writes are written through the
   span: light-color interpolation +1A64..+1A7C/+1A84, fade +1A88/+1A8C and
   rebuilt instance chunk lists. Rejects, without undoing earlier writes or
   calls, where the original would call an unmodeled callback (shader
   teardown, unknown shader setup, the 8039E5EC override, chunk effects) or
   read beyond its structures (more than 8 vertex arrays per chunk, more than
   4 lights, a chunk index past the chunk table, more than 64 rebuilt chunk
   indices, a type-3 shader parameter that reads uninitialized stack). */
int nflstreet_player_draw(const nflstreet_player_draw_context *,uint32_t actor,nflstreet_render_state *);
#endif
