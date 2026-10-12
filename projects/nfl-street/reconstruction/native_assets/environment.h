#ifndef NFLSTREET_ENVIRONMENT_H
#define NFLSTREET_ENVIRONMENT_H
#include <stddef.h>
#include <stdint.h>
typedef struct {const uint8_t *bytes;size_t size,offset[17];unsigned count[17];} nflstreet_environment;
typedef struct {char name[25];float position[3],angles[3];unsigned group,index,name_index;} nflstreet_scene_instance;
/* Probable original version-105 packed arrays, reconstructed from 0x80108320.
   Owns no bytes. BSP tail, event flags, physics and animation are not decoded. */
int nflstreet_environment_open(const uint8_t *,size_t,nflstreet_environment *);
/* Groups correspond to original header counts +0x58/+0x68/+0x70, not enums. */
int nflstreet_environment_instance(const nflstreet_environment *,unsigned,unsigned,nflstreet_scene_instance *);
int nflstreet_environment_rotation(const nflstreet_scene_instance *,float [9]);
#endif
