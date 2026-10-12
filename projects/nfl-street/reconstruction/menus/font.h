#ifndef NFLSTREET_MENU_FONT_H
#define NFLSTREET_MENU_FONT_H
#include <stddef.h>
#include <stdint.h>
#include "../native_assets/texture.h"
typedef struct {const uint8_t*bytes;size_t size,glyphs,kerning,atlas;unsigned count,width,height,line_height;} nflstreet_font;
typedef struct {int32_t x,y;uint32_t width,height;uint16_t u,v;} nflstreet_glyph_box;
int nflstreet_font_open(const uint8_t*,size_t,nflstreet_font*);
/* Original 80185A98 / 80185B18. Missing glyph/pair returns zero. */
const uint8_t*nflstreet_font_glyph(const nflstreet_font*,uint32_t code);
int nflstreet_font_kerning(const nflstreet_font*,uint32_t,uint32_t);
/* Original 801865FC and bounded 801863A4, byte strings <=354 bytes.
   flags masks 1=center, 2=right, 4=disable kerning.
   Nonpositive limit disables width wrapping. No UI layout/scale policy. */
int nflstreet_font_measure(const nflstreet_font*,const char*,unsigned,int32_t*);
int nflstreet_font_layout(const nflstreet_font*,const char*,unsigned,double,
 nflstreet_glyph_box*,size_t,unsigned*);
/* Original 8018675C XY and UV words; four corners TL/TR/BR/BL. */
int nflstreet_font_quad(const nflstreet_font*,const nflstreet_glyph_box*,float xyuv[16]);
/* Complete original 80186E58 palette values (GX IA8), pass0 then pass1. */
void nflstreet_font_palettes(uint16_t palettes[2][16]);
/* Bounded source atlas decoding. Pass0/1 selects the original IA8 palette;
   caller frees with nflstreet_texture_close. Screen colors/filtering are separate. */
int nflstreet_font_atlas(const nflstreet_font*,unsigned pass,nflstreet_texture*);
#endif
