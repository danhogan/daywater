#ifndef NFLSTREET_MARKER_RAM_H
#define NFLSTREET_MARKER_RAM_H
/* RAM access shared by the player-marker modules (select rings, name tag
   projection, camera slots): word, float and byte reads and writes through
   the draw span. Internal. */
#include "draw_engine.h"
static inline int mk_word(nflstreet_draw *d,uint32_t a,uint32_t *v){return nflstreet_draw_word(d,a,v);}
static inline int mk_byte(nflstreet_draw *d,uint32_t a,uint8_t *v){const uint8_t *p=nflstreet_draw_at(d,a,1);if(!p)return 0;*v=*p;return 1;}
static inline int mk_float(nflstreet_draw *d,uint32_t a,float *v){uint32_t w;if(!mk_word(d,a,&w))return 0;*v=nflstreet_as_float(w);return 1;}
static inline int mk_put(nflstreet_draw *d,uint32_t a,uint32_t v){uint8_t *p=nflstreet_draw_at(d,a,4);if(!p)return 0;nflstreet_wr32(p,v);return 1;}
static inline int mk_putf(nflstreet_draw *d,uint32_t a,float v){return mk_put(d,a,nflstreet_float_bits(v));}
static inline int mk_putb(nflstreet_draw *d,uint32_t a,uint8_t v){uint8_t *p=nflstreet_draw_at(d,a,1);if(!p)return 0;*p=v;return 1;}
/* Read-modify-write of the word at a: (word & ~clear) | set. */
static inline int mk_mask(nflstreet_draw *d,uint32_t a,uint32_t clear,uint32_t set){uint32_t w;return mk_word(d,a,&w)&&mk_put(d,a,(w&~clear)|set);}
/* Copy the word at from to to (the original's lfs/stfs pair on a float). */
static inline int mk_copy(nflstreet_draw *d,uint32_t to,uint32_t from){uint32_t w;return mk_word(d,from,&w)&&mk_put(d,to,w);}
#endif
