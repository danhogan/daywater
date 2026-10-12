#include "compact_pose.h"
#include <math.h>
#include <string.h>
static unsigned u16(const uint8_t*p){return (unsigned)p[0]<<8|p[1];}
static uint32_t u32(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static float word(uint32_t w){float f;memcpy(&f,&w,4);return f;}
/* Original initialized 8039E100 six-coefficient paired polynomial. */
static float sine(int16_t angle){
 float x=angle*(1.0f/32768.0f),x2=x*x,x3=x*x2,x4=x2*x2,x5=x*x4,x7=x3*x4,x9=x5*x4,x11=x7*x4;
 float a=word(0x40490fdb)*x,b=word(0xc0a55de8)*x3;
 a=fmaf(word(0x402335e5),x5,a);b=fmaf(word(0xbf196969),x7,b);
 a=fmaf(word(0x3da83c1d),x9,a);b=fmaf(word(0xbbf183ad),x11,b);return a+b;
}
static void concat(const float*a,const float*b,float*out){
 for(unsigned r=0;r<3;++r)for(unsigned c=0;c<4;++c){float v=a[r*4]*b[c];
 v=fmaf(a[r*4+1],b[4+c],v);v=fmaf(a[r*4+2],b[8+c],v);
 if(c>=2)v=fmaf(c==3?1.0f:0.0f,a[r*4+3],v);out[r*4+c]=v;}
}
int nflstreet_compact_pose(const uint8_t*b,size_t n,const int16_t*angles,size_t angle_count,const float root[12],float out[256][12],unsigned*count){
 if(!b||!angles||!root||!out||!count||n<16||memcmp(b,"SKL1",4))return 0;
 unsigned bones=u16(b+6);if(!bones||bones>256||n<16+(size_t)bones*16||angle_count<(size_t)bones*3)return 0;
 for(unsigned i=0;i<12;++i)if(!isfinite(root[i]))return 0;
 float result[256][12];
 for(unsigned i=0;i<bones;++i){const uint8_t*row=b+16+i*16;int parent=(int16_t)u16(row+12);
 if((!i&&parent!= -1)||(i&&(parent<0||(unsigned)parent>=i)))return 0;
 float s[3],c[3];for(unsigned axis=0;axis<3;++axis){int16_t a=angles[i*3+axis];s[axis]=sine(a);c[axis]=sine((int16_t)(uint16_t)((uint16_t)a+16384));}
 float sx=s[0],sy=s[1],sz=s[2],cx=c[0],cy=c[1],cz=c[2];
 float a=sx*sy,d=cx*sy;
 float local[12]={cz*cy,fmaf(-sz,cx,cz*a),fmaf(sz,sx,cz*d),0,
 sz*cy,fmaf(cz,cx,sz*a),fmaf(-cz,sx,sz*d),0,-sy,cy*sx,cy*cx,0};
 for(unsigned axis=0;axis<3;++axis){local[axis*4+3]=word(u32(row+axis*4));if(!isfinite(local[axis*4+3]))return 0;}
 concat(i?result[parent]:root,local,result[i]);
 for(unsigned j=0;j<12;++j)if(!isfinite(result[i][j]))return 0;
 }
 memcpy(out,result,(size_t)bones*12*sizeof(float));*count=bones;return 1;
}
