#ifndef NFLSTREET_TEXTURE_APPLY_H
#define NFLSTREET_TEXTURE_APPLY_H
#include "face_texture.h"
/* Platform cache/upload notification, in original ordered address/size units.
   The host owns actual GPU/cache effects; rejection retains earlier writes. */
typedef int (*nflstreet_texture_flush)(void *,uint32_t token,uint32_t size);
/* Original80164CE4 for processed MMAPv2. Destination name0 determines image
   count and copy sizes; source name0 supplies image/palette selectors. */
int nflstreet_texture_apply(uint8_t *,size_t,uint32_t destination_token,
 const uint8_t *,size_t,uint32_t source_token,nflstreet_texture_flush,void *);
/* Own an independently loaded template destination and apply selected data. */
typedef struct {uint8_t *bytes;size_t size;uint32_t token;unsigned template_index;} nflstreet_applied_face_texture;
int nflstreet_face_texture_apply_template(const nflstreet_archive *,const nflstreet_face_texture_resource *,
 uint32_t,nflstreet_texture_flush,void *,nflstreet_applied_face_texture *);
void nflstreet_applied_face_texture_close(nflstreet_applied_face_texture *);
#endif
