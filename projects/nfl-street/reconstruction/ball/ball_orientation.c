#include "ball_orientation.h"
#include "ball_attachment.h"
#include <math.h>
#include <string.h>
static float word(uint32_t w){float f;memcpy(&f,&w,4);return f;}
static int32_t wrap24(uint32_t a){a&=0xffffff;return a&0x800000?(int32_t)a-0x1000000:(int32_t)a;}
static float sine24(int32_t angle){
 float x=wrap24((uint32_t)angle)*(1.0f/8388608.0f),x2=x*x,x3=x*x2,x4=x2*x2;
 float x5=x*x4,x7=x3*x4,x9=x5*x4,x11=x7*x4,x13=x9*x4,x15=x11*x4;
 float a=word(0x40490fdb)*x,b=word(0xc0a55de8)*x3;
 a=fmaf(word(0x402335e5),x5,a);b=fmaf(word(0xbf196969),x7,b);
 a=fmaf(word(0x3da83c1d),x9,a);b=fmaf(word(0xbbf183ad),x11,b);
 a=fmaf(word(0x39f47a22),x13,a);b=fmaf(word(0xb7b7d6e3),x15,b);return a+b;
}
static float cosine24(int32_t angle){return sine24(wrap24((uint32_t)angle+0x400000));}
static int angle(double y,double x,int32_t*out){
 double ay=fabs(y),ax=fabs(x);float sum=(float)(ay+ax);if(!isfinite(sum)||sum==0)return 0;
 float ratio=(float)(ay-ax)/sum;int32_t base=0;
 if(y<0){base=0x800000;if(x>=0){base=-0x400000;ratio=-ratio;}}
 else if(x<0){base=0x400000;ratio=-ratio;}
 float square=ratio*ratio,power=ratio*square;
 float v=power*word(0xbeaaa61c);v=fmaf(ratio,word(0x3f7ffff5),v);
 const uint32_t coeff[6]={0x3e4c40a6,0xbe0e6c63,0x3dc577df,0xbd6501c4,0x3cb31652,0xbb84d7e7};
 for(unsigned i=0;i<6;++i){power*=square;v=fmaf(power,word(coeff[i]),v);}
 v=(v+word(0x3f490fdb))*word(0x4a22f983);
 if(!isfinite(v)||v>=2147483648.0f||v< -2147483648.0f)return 0;
 *out=wrap24((uint32_t)(int32_t)v+(uint32_t)base);return 1;
}
static void quaternion(const int32_t e[3],float q[4]){
 float sx=sine24(e[0]>>1),sy=sine24(e[1]>>1),sz=sine24(e[2]>>1);
 float cx=cosine24(e[0]>>1),cy=cosine24(e[1]>>1),cz=cosine24(e[2]>>1);
 float a=cz*cy,b=sz*cy,c=sz*sy,d=cz*sy;
 q[3]=fmaf(cx,a,sx*c);q[2]=fmaf(cx,b,-(sx*d));q[0]=fmaf(sx,a,-(cx*c));q[1]=fmaf(cx,d,sx*b);
}
static int matrix_quaternion(const float m[12],float q[4]){
 float y=-m[8],square=y*y;if(square>1)square=1;
 float cy=sqrtf(1.0f-square);float xx,xy,zx,zy;
 if(cy<=word(0x33d6bf95)){xx=-m[6];xy=m[5];zx=0;zy=1;}
 else {float inverse=1.0f/cy;xx=m[9]*inverse;xy=m[10]*inverse;zx=m[4]*inverse;zy=m[0]*inverse;}
 int32_t e[3];if(!angle(xx,xy,e)||!angle(y,cy,e+1)||!angle(zx,zy,e+2))return 0;
 quaternion(e,q);for(unsigned i=0;i<4;++i)if(!isfinite(q[i]))return 0;return 1;
}
static void rotate(float m[12],int32_t angle_value,unsigned axis){
 float s=sine24(angle_value),c=cosine24(angle_value);
 for(unsigned r=0;r<3;++r){float a,b;
 if(axis==2){a=m[r*4];b=m[r*4+1];m[r*4]=fmaf(a,c,b*s);m[r*4+1]=fmaf(a,-s,b*c);}
 else if(axis==1){a=m[r*4];b=m[r*4+2];m[r*4]=fmaf(a,c,b*(-s));m[r*4+2]=fmaf(a,s,b*c);}
 else {a=m[r*4+1];b=m[r*4+2];m[r*4+1]=fmaf(a,c,b*s);m[r*4+2]=fmaf(a,-s,b*c);}}
}
int nflstreet_ball_attachment_pose(const float world[12],const float (*bones)[12],size_t count,int selector,const float local[3],const int32_t rotation[3],nflstreet_ball_pose*out){
 if(!out)return 0;nflstreet_ball_pose pose;
 if(!nflstreet_ball_attachment_position(world,bones,count,selector,local,pose.position))return 0;
 if(selector== -1){pose.quaternion[0]=pose.quaternion[1]=pose.quaternion[2]=0;pose.quaternion[3]=1;*out=pose;return 1;}
 if(!rotation)return 0;float m[12];const float*b=bones[selector];
 for(unsigned r=0;r<3;++r)for(unsigned c=0;c<4;++c){float v=world[r*4]*b[c];v=fmaf(world[r*4+1],b[4+c],v);v=fmaf(world[r*4+2],b[8+c],v);if(c>=2)v=fmaf(c==3?1.0f:0.0f,world[r*4+3],v);m[r*4+c]=v;}
 rotate(m,rotation[2],2);rotate(m,rotation[1],1);rotate(m,rotation[0],0);
 if(!matrix_quaternion(m,pose.quaternion))return 0;*out=pose;return 1;
}
static void multiply(const float a[4],const float b[4],float q[4]){
 q[0]=fmaf(a[1],b[2],fmaf(a[3],b[0],a[0]*b[3]))-a[2]*b[1];
 q[1]=fmaf(a[2],b[0],fmaf(a[3],b[1],a[1]*b[3]))-a[0]*b[2];
 q[2]=fmaf(a[0],b[1],fmaf(a[3],b[2],a[2]*b[3]))-a[1]*b[0];
 q[3]=(fmaf(a[3],b[3],-(a[0]*b[0]))-a[1]*b[1])-a[2]*b[2];
}
static int normalize(float q[4]){
 float n=q[1]*q[1];n=fmaf(q[0],q[0],n);n=fmaf(q[2],q[2],n);n=fmaf(q[3],q[3],n);
 if(!isfinite(n)||n<0)return 0;float length=sqrtf(n);float inverse=length>word(0x33d6bf95)?1.0f/length:0;
 for(unsigned i=0;i<4;++i){q[i]*=inverse;if(!isfinite(q[i]))return 0;}return 1;
}
int nflstreet_ball_no_spin(float q[4],float delta[4]){
 if(!q||!delta)return 0;for(unsigned i=0;i<4;++i)if(!isfinite(q[i]))return 0;
 float identity[4]={0,0,0,1},axis[4]={0,0,0,cosine24(0)},d[4],result[4];
 if(!normalize(axis))return 0;
 multiply(axis,identity,d);multiply(d,q,result);if(!normalize(result))return 0;
 memcpy(q,result,sizeof result);memcpy(delta,d,sizeof d);return 1;
}
int nflstreet_ball_attachment_rotation(const float actor[4],float out[4]){
 if(!actor||!out)return 0;for(unsigned i=0;i<4;++i)if(!isfinite(actor[i]))return 0;
 int32_t first_angles[3]={-0x400000,0,0},second_angles[3]={0,0,-0x400000};float a[4],b[4],q[4],result[4];
 quaternion(first_angles,a);quaternion(second_angles,b);multiply(actor,a,q);if(!normalize(q))return 0;
 multiply(q,b,result);if(!normalize(result))return 0;memcpy(out,result,sizeof result);return 1;
}

int nflstreet_ball_model_attachment_pose(const float root[12],float scale,const float (*bones)[12],size_t count,int selector,const float local[3],const int32_t rotation[3],nflstreet_ball_pose*out){
 if(selector== -1)return nflstreet_ball_attachment_pose(NULL,NULL,0,-1,NULL,NULL,out);
 if(!root||!isfinite(scale))return 0;float scaled[12];
 for(unsigned i=0;i<12;++i){if(!isfinite(root[i]))return 0;scaled[i]=i%4==3?root[i]:root[i]*scale;if(!isfinite(scaled[i]))return 0;}
 return nflstreet_ball_attachment_pose(scaled,bones,count,selector,local,rotation,out);
}
static uint32_t read_be(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void write_float(uint8_t*p,float f){uint32_t w;memcpy(&w,&f,4);p[0]=w>>24;p[1]=w>>16;p[2]=w>>8;p[3]=w;}
int nflstreet_ball_update_attached(uint8_t*r,size_t n,const float world[12],const float root[12],float scale,const float (*bones)[12],size_t count,int selector,const float local[3],const int32_t rotation[3],double delta,nflstreet_ball_pose*out){
 if(!r||n<0xf4||!out||!isfinite(delta))return 0;
 nflstreet_ball_pose actor,model;float q[4],velocity[3];
 if(!nflstreet_ball_attachment_pose(world,bones,count,selector,local,rotation,&actor)||
    !nflstreet_ball_attachment_rotation(actor.quaternion,q)||
    !nflstreet_ball_model_attachment_pose(root,scale,bones,count,selector,local,rotation,&model))return 0;
 float reciprocal=delta==0?1.0f:(float)(1.0/delta);if(!isfinite(reciprocal))return 0;
 for(unsigned i=0;i<3;++i){uint32_t w=read_be(r+0x28+i*4);float previous;memcpy(&previous,&w,4);if(!isfinite(previous))return 0;
  velocity[i]=actor.position[i]-previous;if(delta!=0)velocity[i]*=reciprocal;if(!isfinite(velocity[i]))return 0;}
 for(unsigned i=0;i<3;++i){write_float(r+12+i*4,actor.position[i]);write_float(r+0x54+i*4,velocity[i]);}
 for(unsigned i=0;i<4;++i)write_float(r+0x18+i*4,q[i]);*out=model;return 1;
}

int nflstreet_ball_render_matrix(const nflstreet_ball_pose*pose,float out[12]){
 if(!pose||!out)return 0;for(unsigned i=0;i<3;++i)if(!isfinite(pose->position[i]))return 0;
 for(unsigned i=0;i<4;++i)if(!isfinite(pose->quaternion[i]))return 0;
 int32_t angles[3]={0x400000,0,0x400000};float correction[4],q[4];quaternion(angles,correction);multiply(pose->quaternion,correction,q);
 float x=q[0],y=q[1],z=q[2],w=q[3],xx=(x+x)*x,yy=(y+y)*y,zz=(z+z)*z;
 float xy=(x+x)*y,xz=(x+x)*z,yz=(y+y)*z,xw=(x+x)*w,yw=(y+y)*w,zw=(z+z)*w;
 float m[12]={1-(yy+zz),xy-zw,xz+yw,pose->position[0],xy+zw,1-(xx+zz),yz-xw,pose->position[1],xz-yw,yz+xw,1-(xx+yy),pose->position[2]};
 for(unsigned i=0;i<12;++i)if(!isfinite(m[i]))return 0;memcpy(out,m,sizeof m);return 1;
}
