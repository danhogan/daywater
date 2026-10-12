#ifndef NFLSTREET_MENU_TEXT_LINES_H
#define NFLSTREET_MENU_TEXT_LINES_H
#include "font.h"
/* Original801D8E54 bounded word/character wrapping (flags100+1000/2000).
   Owned ranges replace the original allocated line table. line_base is a
   caller-supplied logical address, never an inferred original heap pointer.
   A one-line object retains the original inline range at offsets36/38.
   Existing allocated objects and clipping-only modes reject atomically. */
typedef struct {uint16_t begin,end;} nflstreet_ui_text_line;
typedef struct {unsigned count;nflstreet_ui_text_line lines[255];} nflstreet_ui_text_lines;
int nflstreet_ui_text_prepare_lines(uint8_t object[40],const nflstreet_font*,
 const char*,uint32_t line_base,nflstreet_ui_text_lines*);
#endif
