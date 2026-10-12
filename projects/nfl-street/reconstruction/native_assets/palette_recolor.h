#ifndef NFLSTREET_PALETTE_RECOLOR_H
#define NFLSTREET_PALETTE_RECOLOR_H
#include "gear_texture.h"
#include "texture_apply.h"
#define NFLSTREET_COLOR_TABLE_BYTES 4096u
#define NFLSTREET_COLOR_STATE_BYTES 24u
/* Original 8030A898 color table built by 80148A88 from the template DB:
   CPAL row id n fills entry n, CPHR row id n fills entry (n+0x80)&0xFF. Each
   16-byte entry holds four RGB levels at +0/+4/+8/+C (level order PRIMARY,
   SHADOW, MIDTONE, HIGHLIGHT); the fourth byte of each level stays zero.
   Missing fields reject; the original's stale-item behavior is not modeled. */
typedef struct {uint8_t bytes[NFLSTREET_COLOR_TABLE_BYTES];} nflstreet_color_table;
int nflstreet_color_table_build(const uint8_t *template_db,size_t,const nflstreet_gear_texture_catalog *,nflstreet_color_table *);
/* One 800D8680 operation (8 bytes in the original): kind 0 set color,
   1 set palette index, 2 blend range ends, 3 blend color->end, 4 blend
   start->color, 5 blend palette index->end, 6 blend start->palette index,
   7 multiply. color is a color-state id (0..23) or a palette index. */
typedef struct {uint32_t kind;uint8_t start,end,color,level;} nflstreet_dynclut_op;
/* 800D8918 over one pladyncl dynclut document (<op> elements of
   <operations>). Unknown op types reject. */
int nflstreet_dynclut_parse(const uint8_t *text,size_t,const nflstreet_gear_texture_catalog *,
 nflstreet_dynclut_op *,unsigned max,unsigned *count);
/* 800D9AF0: one op list per pladyncl entry (the r13-5B48 table). */
typedef struct {nflstreet_dynclut_op *ops;unsigned *first,*count;unsigned entries,total;} nflstreet_dynclut_table;
int nflstreet_dynclut_table_open(const nflstreet_archive *pladyncl,const nflstreet_gear_texture_catalog *,nflstreet_dynclut_table *);
void nflstreet_dynclut_table_close(nflstreet_dynclut_table *);
/* 800D9948 on one palette: format 1 = RGB565, 2 = RGB5A3 (other formats are
   left unchanged). colors[id] is the 16-byte color-table entry for state id. */
int nflstreet_dynclut_apply(uint16_t format,uint8_t *palette,size_t bytes,const nflstreet_dynclut_op *,unsigned count,
 const uint8_t *colors[NFLSTREET_COLOR_STATE_BYTES]);
/* 800D9C0C on a relocated MMAPv2 destination: palette record0 recolored with
   the op list of pladyncl entry, colors from the 24-byte color state, then one
   cache flush of the palette data (800D8A9C; its 8020380C call is not modeled). */
int nflstreet_palette_compose(uint8_t *mmap,size_t size,uint32_t token,const nflstreet_dynclut_table *,uint32_t entry,
 const nflstreet_color_table *,const uint8_t state[NFLSTREET_COLOR_STATE_BYTES],nflstreet_texture_flush,void *);
/* Slot-texture adapter: composes with one player's color state (record
   +4C..+63, copied to 802ABE6C by 800E3020). Flushes go to flush/opaque. */
typedef struct {const nflstreet_dynclut_table *table;const nflstreet_color_table *colors;const uint8_t *state;
 nflstreet_texture_flush flush;void *opaque;} nflstreet_palette_context;
int nflstreet_palette_compose_slot(void *context,unsigned slot,uint32_t palette,uint8_t *destination,size_t size,uint32_t token);
#endif
