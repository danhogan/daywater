#ifndef NFLSTREET_GEAR_TEXTURE_H
#define NFLSTREET_GEAR_TEXTURE_H
#include "archive.h"
#include "../menus/team_catalog.h"
#define NFLSTREET_GEAR_RECORD_BYTES 112u
#define NFLSTREET_GEAR_RECORDS_MAX 512u
#define NFLSTREET_GEAR_PLAYER_RECORD_BYTES 104u
#define NFLSTREET_GEAR_TEXTURE_SLOTS 25u
#define NFLSTREET_GEAR_TEXTURE_COLORS_MAX 256u
/* Private original data exported by export_gear_texture_catalog.py: DOL window
   802714F0..80271D58 (defaults, selectors, suffixes, initial color list,
   channel lists, 80271BC8/80271C58 GRSI tables), 802859F8 categories,
   80285968 GEAR query tags, the 80252CF8 excluded palette name and the 25
   initial 802799EC slot request indices (slot templates). */
typedef struct {const uint8_t *window,*categories,*tags;const char *excluded_dynamic_name;const uint8_t *templates;} nflstreet_gear_texture_catalog;
int nflstreet_gear_texture_catalog_open(const uint8_t *,size_t,nflstreet_gear_texture_catalog *);
/* Initial request index (template entry) of one slot; UINT32_MAX if out of range. */
uint32_t nflstreet_gear_texture_template(const nflstreet_gear_texture_catalog *,unsigned slot);
/* Original8014B918 112-byte GEAR record from one table row: text fields copy
   31 bytes plus the 801BFFDC type-0 terminator, then truncate at a space.
   Bytes outside written fields are retained. */
int nflstreet_gear_record_from_table(const nflstreet_team_table *,unsigned row,
 const nflstreet_gear_texture_catalog *,uint8_t record[NFLSTREET_GEAR_RECORD_BYTES]);
typedef struct {uint8_t records[NFLSTREET_GEAR_RECORDS_MAX][NFLSTREET_GEAR_RECORD_BYTES];unsigned count;} nflstreet_gear_records;
int nflstreet_gear_records_build(const nflstreet_team_table *,const nflstreet_gear_texture_catalog *,nflstreet_gear_records *);
/* 8014B8A0 GRTI/GEID selection. Missing/duplicate rows reject; the original
   failed-search cursor state is not reconstructed. */
int nflstreet_gear_records_find(const nflstreet_gear_records *,uint32_t category,uint32_t id,unsigned *row);
/* Player-derived fields of the original 0x68-byte record (802AAA60+slot*68):
   8014F60C bytes0..A and +0C..+3B, then 800DE000 +40 IKSP+1, +42 PLHT,
   +44 PHAR (3F->C), +46 800DDEBC, +48 800DDF44, +49 PLEX, +4A PJEN,
   +4B TBYP. Missing PLAY tags read zero as in 801BFFDC/80149614. Team/context
   bytes +3C..+3F, +4C..+67 are caller-owned and retained. */
int nflstreet_gear_player_record_from_play(const nflstreet_team_table *play,unsigned row,
 const nflstreet_gear_records *,uint8_t record[NFLSTREET_GEAR_PLAYER_RECORD_BYTES]);
/* Original800DD720 for one logical gear index; arrays retain untouched slots. */
int nflstreet_gear_texture_select(const nflstreet_gear_texture_catalog *,const nflstreet_archive *textures,
 const nflstreet_archive *dynamic_palettes,const uint8_t gear[NFLSTREET_GEAR_RECORD_BYTES],
 const uint8_t player[NFLSTREET_GEAR_PLAYER_RECORD_BYTES],unsigned logical_index,
 int32_t indices[NFLSTREET_GEAR_TEXTURE_SLOTS],uint32_t palettes[NFLSTREET_GEAR_TEXTURE_SLOTS]);
/* Original800DDBE8 selector kind: returns0 for kinds without effect, else the
   index/palette written and the color byte requested from packed. a5 is the
   +3E halfword minus one with 32-bit wrap. */
int nflstreet_gear_texture_kind(uint32_t kind,uint32_t packed,uint32_t a4,uint32_t a5,uint32_t a6,uint32_t a7,
 int32_t *index,uint32_t *palette,uint32_t *color_value);
/* Original800E3218 slot0 fallback for the +44 halfword. */
void nflstreet_gear_texture_fallback(uint32_t code,int32_t *index,uint32_t *palette);
/* One 800E217C(target,channel,value) request; the target is the caller's. */
typedef struct {uint16_t channel,value;} nflstreet_gear_color_request;
typedef struct {
 int32_t indices[NFLSTREET_GEAR_TEXTURE_SLOTS];uint32_t palettes[NFLSTREET_GEAR_TEXTURE_SLOTS];
 uint8_t fallback;int32_t fallback_index;uint32_t fallback_palette; /* second slot0 800E2E48 */
 uint8_t record_65;unsigned gear_rows[12],color_count;
 nflstreet_gear_color_request colors[NFLSTREET_GEAR_TEXTURE_COLORS_MAX];
} nflstreet_gear_texture_plan;
/* Original800DE3B8 decisions for one player record: 25 slot index/palette
   pairs passed to 800E2E48 in slot order, the 800E3218 slot0 fallback, ordered
   800E217C color requests and the stored +65 byte. Texture/palette HSH1 lookup
   is native. Request-record, 800E3020 state and color effects stay with the
   caller; output is written only on success. */
int nflstreet_gear_texture_plan_build(const nflstreet_gear_texture_catalog *,const nflstreet_gear_records *,
 const nflstreet_archive *textures,const nflstreet_archive *dynamic_palettes,
 const uint8_t player[NFLSTREET_GEAR_PLAYER_RECORD_BYTES],nflstreet_gear_texture_plan *);
#endif
