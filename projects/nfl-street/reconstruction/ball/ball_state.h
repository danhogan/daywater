#ifndef NFLSTREET_BALL_STATE_H
#define NFLSTREET_BALL_STATE_H
#include <stddef.h>
#include <stdint.h>
typedef struct {uint32_t position_words[3],flags,opaque_state,associated_actor_handle;unsigned active_index;} nflstreet_ball_state;
/* Owned native view of the original bounded pool bytes, not RAM addresses.
   GNNE69 r0: pool count +0, active index +1, records +8 stride 0xf4. */
int nflstreet_ball_observe(const uint8_t *,size_t,nflstreet_ball_state *);
/* Original 800BE4F8 lookup for a supplied valid actor header. The first
   matching association is returned; top-byte bits are ignored by 80020944.
   This is association lookup, not native attach/release lifecycle. */
int nflstreet_ball_find_actor(const uint8_t *,size_t,uint32_t,unsigned *,unsigned *);
/* Original 0x800BE678 render-position choice. Coordinate interpretation and
   whether the two supplied points are model bounds remain unverified. */
int nflstreet_ball_anchor(const float current[3],const float first[3],
                         const float second[3],float out[3]);
typedef struct {float position[3],velocity[3],acceleration_xy[2];} nflstreet_ball_trajectory;
/* Read the exact component fields consumed by 800C11DC from one owned
   record, rejecting nonfinite values atomically. No launch-state inference. */
int nflstreet_ball_read_trajectory(const uint8_t *,size_t,nflstreet_ball_trajectory *);
/* Bounded original 0x800C11DC: sample XY at the time selected for a target
   third-coordinate plane. Units and launch/bounce scheduling are unresolved. */
int nflstreet_ball_sample_plane(const nflstreet_ball_trajectory *,double plane,
                               float *time,float xy[2]);
#endif
