/* Absolute callees researched in GNNE69 revision 0. Keep this isolated. */
clear_score_unknown_40 = 0x801042C8;
score_table_get_for_side = 0x80053128;
set_score_for_side = 0x80104294;
score_table_set_for_side = 0x80053180;
score_event_update = 0x80109030;
score_event_signal = 0x8010A384;
ENTRY(add_score_for_side)
SECTIONS {
  . = 0x8002676C;
  .text : { *(.text) }
  /DISCARD/ : { *(.eh_frame) *(.note*) *(.comment) }
}
