#include "pose.h"
int nflstreet_pose_mix(int16_t*out,const int16_t*first,const int16_t*second,size_t n,unsigned weight){
 if(!out||!first||!second||n>768||weight>4096)return 0;
 for(size_t i=0;i<n;++i){int delta=(int16_t)((uint16_t)second[i]-(uint16_t)first[i]);
  int product=delta*(int)weight;int shifted=product>=0?product/4096:-((-product+4095)/4096);
  out[i]=(int16_t)(first[i]+shifted);}return 1;
}
#include <math.h>
#include <string.h>
static unsigned u16(const uint8_t *p){return (unsigned)p[0]<<8|p[1];}
static uint32_t u32(const uint8_t *p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static int span(size_t n,size_t o,size_t s){return o<=n&&s<=n-o;}
static float real(const uint8_t *p){uint32_t w=u32(p);float f;memcpy(&f,&w,4);return f;}
/* Explicit arithmetic shift avoids implementation-defined negative shifts. */
static int shift(int v,unsigned s){return v>=0?v/(1<<s):-1-((-1-v)/(1<<s));}
int nflstreet_pose_frame(const uint8_t *b,size_t n,unsigned frame,int16_t *out,size_t capacity,unsigned *count){
    if(!b||!out||!count||n<32||(u16(b+6)&5)!=1||frame>=u16(b+4))return 0;
    size_t ft=u32(b+16),desc=u32(b+20);
    if(!span(n,ft,(size_t)u16(b+4)*4)||!span(n,desc,24))return 0;
    unsigned components=u32(b+desc);size_t base=u32(b+desc+4),scale=u32(b+desc+8),width=u32(b+desc+20);
    if(!components||components>384||capacity<components||!span(n,base,components*2)||!span(n,scale,components*2)||!span(n,width,components*2))return 0;
    size_t at=u32(b+ft+frame*4);if(!span(n,at,10))return 0;
    unsigned header=10+((b[at]&64)?6:0)+((b[at]&128)?6:0);
    if(!span(n,at,header))return 0;
    size_t bit=(at+header)*8;int16_t result[384];
    for(unsigned i=0;i<components;++i){unsigned bits=b[width+i*2+1];uint32_t v=0;
        if(bits>16||bit>n*8||bits>n*8-bit)return 0;
        for(unsigned j=0;j<bits;++j,++bit)v=v*2+((b[bit/8]>>(7-bit%8))&1);
        int q=(int)(int16_t)(uint16_t)(v<<(16-bits));
        int value=(int)(int16_t)u16(b+base+i*2)+shift(q*(int)(int16_t)u16(b+scale+i*2),15);
        result[i]=(int16_t)(uint16_t)value;
    }
    memcpy(out,result,components*2);*count=components;return 1;
}
int nflstreet_pose_clusters(const uint8_t *b,size_t n,const int16_t *pose,size_t pose_count,float out[256][12],unsigned *count){
    if(!b||!out||!count||n<16||memcmp(b,"TMdl",4))return 0;
    size_t directory=u16(b+10),skel=0,clst=0;unsigned sections=u16(b+8);
    if(!span(n,directory,sections*16))return 0;
    for(unsigned i=0;i<sections;++i){const uint8_t *s=b+directory+i*16;
        if(!memcmp(s,"Skel",4))skel=u32(s+4);
        if(!memcmp(s,"Clst",4))clst=u32(s+4);
    }
    if(!skel||!clst||!span(n,skel,24)||!span(n,clst,8))return 0;
    unsigned bones=u32(b+skel),clusters=u32(b+clst+4);size_t tr=u32(b+skel+8),rot=u32(b+skel+12),rows=u32(b+clst);
    if(!bones||bones>256||!clusters||clusters>256||!span(n,tr,bones*16)||!span(n,rot,bones*6)||!span(n,rows,clusters*4)||(pose&&pose_count<bones*3))return 0;
    float matrices[256][12]={0};matrices[0][0]=matrices[0][5]=matrices[0][10]=1;
    for(unsigned i=1;i<clusters;++i){unsigned bone=b[rows+i*4],parent=b[rows+i*4+1],weight=u16(b+rows+i*4+2);
        if(bone>=bones||parent>=i||weight>4096)return 0;
        double a[3];for(unsigned j=0;j<3;++j){int r=(int)(int16_t)u16(b+rot+bone*6+j*2);
            if(pose)r+=shift((pose[bone*3+j]-r)*(int)weight,12);
            a[j]=r*(6.2831853071795864769/65536.0);
        }
        double sx=sin(a[0]),cx=cos(a[0]),sy=sin(a[1]),cy=cos(a[1]),sz=sin(a[2]),cz=cos(a[2]);
        float local[12]={(float)(cz*cy),(float)(-sz*cx+cz*sx*sy),(float)(sz*sx+cz*cx*sy),0,
          (float)(sz*cy),(float)(cz*cx+sz*sx*sy),(float)(-cz*sx+sz*cx*sy),0,
          (float)-sy,(float)(cy*sx),(float)(cy*cx),0};
        for(unsigned j=0;j<3;++j){local[j*4+3]=real(b+tr+bone*16+j*4);if(!isfinite(local[j*4+3]))return 0;}
        for(unsigned j=0;j<3;++j)for(unsigned k=0;k<4;++k){float v=k==3?matrices[parent][j*4+3]:0;
            for(unsigned l=0;l<3;++l)v+=matrices[parent][j*4+l]*local[l*4+k];
            if(!isfinite(v))return 0;
            matrices[i][j*4+k]=v;
        }
    }
    memcpy(out,matrices,clusters*12*sizeof(float));*count=clusters;return 1;
}
