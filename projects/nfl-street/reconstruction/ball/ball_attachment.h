#ifndef NFLSTREET_BALL_ATTACHMENT_H
#define NFLSTREET_BALL_ATTACHMENT_H
#include <stddef.h>
/* Position portion of original 80020C54/80020C2C. Matrices are owned native
   row-major affine 3x4 arrays. Selector -1 returns original zero position.
   This does not supply attachment orientation or perform ownership changes. */
int nflstreet_ball_attachment_position(const float world[12],
 const float (*clusters)[12],size_t count,int selector,
 const float local[3],float position[3]);
#endif
