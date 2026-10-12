#ifndef NFLSTREET_BALL_FLIGHT_H
#define NFLSTREET_BALL_FLIGHT_H
#include <stddef.h>
#include <stdint.h>
/* Bounded complete 800C0684: detached, positive +9C, wind zero, no spin.
   Force +7C and target plane are supplied inputs. No launch/ground policy.
   Unsupported/nonfinite inputs preserve the complete record. */
int nflstreet_ball_update_airborne(uint8_t *,size_t,double delta,double target_plane);
#endif
