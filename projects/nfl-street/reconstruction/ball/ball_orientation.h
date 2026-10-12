#ifndef NFLSTREET_BALL_ORIENTATION_H
#define NFLSTREET_BALL_ORIENTATION_H
#include <stddef.h>
#include <stdint.h>
typedef struct {float position[3],quaternion[4];} nflstreet_ball_pose;
/* Original 80020C2C actor attachment, including model XYZ rotations and
   matrix-to-Euler-to-quaternion conversion. Owned affine 3x4 inputs. */
int nflstreet_ball_attachment_pose(const float world[12],const float (*)[12],
 size_t count,int selector,const float local[3],const int32_t rotation_xyz[3],
 nflstreet_ball_pose *);
/* Original 800C0ECC two quarter-turn products and two normalizations.
   Does not update ball model, association, velocity or lifecycle. */
int nflstreet_ball_attachment_rotation(const float actor_quaternion[4],float ball_quaternion[4]);
/* Original 800D4BF4: model root basis scaled before compact attachment.
   Position/quaternion are the model output before the ball quarter turns. */
int nflstreet_ball_model_attachment_pose(const float model_root[12],float scale,
 const float (*)[12],size_t count,int selector,const float local[3],
 const int32_t rotation_xyz[3],nflstreet_ball_pose *);
/* Bounded 800C0ECC after caller resolves association: update an owned 0xf4
   ball record and return the visual-model pose. No association/state changes.
   Previous position at +28 and caller delta are supplied original inputs. */
int nflstreet_ball_update_attached(uint8_t *record,size_t bytes,
 const float world[12],const float model_root[12],float scale,
 const float (*)[12],size_t count,int selector,const float local[3],
 const int32_t rotation_xyz[3],double delta,nflstreet_ball_pose *model_pose);
/* Original ordinary-mode 800C05E0 correction, then 801A3008 basis.
   Position is caller-supplied; alternate render mode remains unsupported. */
int nflstreet_ball_render_matrix(const nflstreet_ball_pose *,float matrix[12]);
/* Bounded airborne 800C0088 with zero angular inputs and identity delta. */
int nflstreet_ball_no_spin(float quaternion_xyzw[4],float delta_xyzw[4]);
#endif
