#ifndef NFLSTREET_PLAYER_RECORD_H
#define NFLSTREET_PLAYER_RECORD_H
#include "gear_texture.h"
/* Original 800DE6C8 side byte (record +64): 1 when the r13-6F68 byte is set,
   else player > 6 when the 8014C268(2) word (8030B1FC) is nonzero, else
   player < 7. Input meanings are not claimed. */
unsigned nflstreet_player_side(unsigned player,uint8_t r13_6f68,uint32_t word_8030b1fc);
/* Original 800DE000 for one PLAY row of the match GAME database:
   - TEAM row whose DIGT equals the PLAY ITGT, searched in the STAT TEAM table
     (8015082C/801509C8): +5F/+60/+61 = 1CMT/2CMT/3CMT, +3E = LGLT, +3C = LDTC!=0
     when PYTT is 5, else 1.
   - The 8014F60C/getter fields of nflstreet_gear_player_record_from_play.
   - +64 = side, +5E = 1AHP+0x80 (8014F9D8).
   - No STAT LCGP row with this DIGP (8014FB4C), so 800DD64C fills +4C..+5D from
     the 802717BC selectors (+5F..+61 copies, F0 -> 75, F1 -> 7F), +62 = 75,
     +63 = +60, and for side 0 800DD6CC shifts +50/+51 to +51/+52, sets +50 = 75
     and +62 = +5F.
   Missing tags read zero. A missing TEAM row, or a matching LCGP row (its query
   goes through the PLAY cursor and is not modeled), rejects. Bytes +3D and
   +65..+67 are retained; output is written only on success. */
int nflstreet_player_record_build(const nflstreet_team_table *play,unsigned row,const nflstreet_gear_records *,
 const nflstreet_team_table *teams,const nflstreet_team_table *player_colors,const nflstreet_gear_texture_catalog *,
 unsigned side,uint8_t record[NFLSTREET_GEAR_PLAYER_RECORD_BYTES]);
#endif
