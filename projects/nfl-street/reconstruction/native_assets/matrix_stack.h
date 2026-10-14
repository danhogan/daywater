#ifndef NFLSTREET_MATRIX_STACK_H
#define NFLSTREET_MATRIX_STACK_H
/* The original matrix library on saved RAM (internal): stacks {used,
   depth, slots} of 4x4 slots 0x40 apart, the selected one at r13-65E8.
   Single-precision arithmetic as the original (fused multiply-adds where it
   uses fmadds/ps_madd). */
#include "draw_engine.h"
typedef struct nflstreet_matrix_stacks_s {
 uint32_t table;   /* value of r13-65F0: stacks 12 bytes apart */
 uint32_t count;   /* value of r13-65EC */
 uint32_t current; /* r13-65E8 */
 uint32_t index;   /* r13-4EE8: byte, the viewport's stack (801DF594) */
 uint32_t sine;    /* 8039E100: eight polynomial coefficients for sin(pi t) */
 float angle_scale[2]; /* 802617A0 (sine), 802617B0 (cosine): 24-bit angle to half turns */
} nflstreet_matrix_stacks;
int nflstreet_matrix_select(nflstreet_draw *,const nflstreet_matrix_stacks *);          /* 801DF594 + 801893C8 */
int nflstreet_matrix_top(nflstreet_draw *,const nflstreet_matrix_stacks *,uint32_t *);  /* 8018939C (fails without a stack) */
int nflstreet_matrix_push(nflstreet_draw *,const nflstreet_matrix_stacks *);            /* 8018941C */
int nflstreet_matrix_pop(nflstreet_draw *,const nflstreet_matrix_stacks *);             /* 8018949C */
int nflstreet_matrix_reset(nflstreet_draw *,const nflstreet_matrix_stacks *);           /* 801893EC: depth 0, identity */
int nflstreet_matrix_translate(nflstreet_draw *,const nflstreet_matrix_stacks *,float x,float y,float z); /* 80189BB0 */
int nflstreet_matrix_load(nflstreet_draw *,const nflstreet_matrix_stacks *,uint32_t matrix);   /* 8018962C: top = 16 words */
/* 801895BC: top = top x matrix (PSMTXConcat 801FFDF0; unit lanes from
   803A3958 carry the translation column). */
int nflstreet_matrix_concat(nflstreet_draw *,const nflstreet_matrix_stacks *,uint32_t matrix,const float unit[2]);
/* 801DEB54: v rotated by the top (no translation). */
int nflstreet_matrix_rotate_vector(nflstreet_draw *,const nflstreet_matrix_stacks *,const float v[3],float out[3]);
int nflstreet_matrix_scale(nflstreet_draw *,const nflstreet_matrix_stacks *,float x,float y,float z); /* 80189CEC */
int nflstreet_matrix_rotate(nflstreet_draw *,const nflstreet_matrix_stacks *,unsigned axis,uint32_t angle); /* 80189854/944/A34 */
int nflstreet_matrix_load_top(nflstreet_draw *,const nflstreet_matrix_stacks *);        /* 80189ED8 + GXLoadPosMtxImm(0) */
int nflstreet_matrix_store(nflstreet_draw *,const nflstreet_matrix_stacks *,uint32_t to); /* 80189ED8: 16 words */
/* 80188A70 / 80188AEC: sine and cosine of a 24-bit angle (0x800000 = pi). */
/* 80188EAC: 1/sqrt(x), or zero_value when the root is at most epsilon. */
float nflstreet_recip_sqrt(float x,float epsilon,float one,float zero_value);
int nflstreet_sine(nflstreet_draw *,const nflstreet_matrix_stacks *,uint32_t angle,int cosine,float *);
#endif
