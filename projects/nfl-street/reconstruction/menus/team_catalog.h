#ifndef NFLSTREET_TEAM_CATALOG_H
#define NFLSTREET_TEAM_CATALOG_H
#include <stddef.h>
#include <stdint.h>

/* A bounded view of a version8 DB block's raw MAET table. The caller owns
   bytes. No archive I/O, profile filtering, or match assignment is implied. */
typedef struct {
 const uint8_t *bytes;
 size_t size, fields_at, records_at;
 uint32_t record_bytes, record_bits;
 uint16_t allocated, used;
 uint8_t field_count;
} nflstreet_team_table;
typedef struct { uint32_t type, bit_offset, tag, width; } nflstreet_team_field;
int nflstreet_team_table_open_tag(const uint8_t *,size_t,uint32_t,nflstreet_team_table *);
int nflstreet_team_table_open(const uint8_t *,size_t,nflstreet_team_table *);
int nflstreet_team_table_field(const nflstreet_team_table *,unsigned,nflstreet_team_field *);
int nflstreet_team_table_number(const nflstreet_team_table *,unsigned,unsigned,uint32_t *);
int nflstreet_team_table_text(const nflstreet_team_table *,unsigned,unsigned,char *,size_t);
#endif
