#include "sky.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
/* Original signed 24-bit angle and paired-single sine polynomial.
   Each multiply/add rounds to float; fmaf preserves the paired fused steps. */
static float sine(int32_t angle,const float *c,float scale){
    uint32_t u=(uint32_t)angle&0xffffff;
    int32_t signed_angle=(int32_t)(u^0x800000)-0x800000;
    float x=(float)signed_angle*scale,x2=x*x,x3=x*x2,x4=x2*x2;
    float lo=c[0]*x,hi=c[1]*x3,p=x,q=x3;
    for(unsigned i=2;i<8;i+=2){p=p*x4;q=q*x4;lo=fmaf(c[i],p,lo);hi=fmaf(c[i+1],q,hi);}
    return lo+hi;
}
int nflstreet_sky_vertices(const float cfg[31],nflstreet_mesh_vertex vertices[41]){
    if(!cfg||!vertices)return 0;
    for(unsigned i=0;i<31;++i)if(!isfinite(cfg[i]))return 0;
    if(cfg[0]<=0||cfg[0]>10000||cfg[1]!=45||cfg[2]<=0||cfg[2]>100000||cfg[15]<=0||cfg[15]>1)return 0;
    for(unsigned i=16;i<31;++i)if(cfg[i]<0||cfg[i]>1)return 0;
    memset(vertices,0,41*sizeof *vertices);
    for(unsigned r=0;r<5;++r){
        double angle=r?(double)cfg[2]*cfg[2+r]:0;
        if(!isfinite(angle)||fabs(angle)>0x7fffff)return 0;
        int32_t elevation=r?(int32_t)angle:-0x18e38e;
        float radius=sine(elevation+0x400000,cfg+7,cfg[15])*cfg[0];
        for(unsigned s=0;s<8;++s){
            double azimuth=(double)(float)(cfg[1]*(float)s)*cfg[2];
            if(!isfinite(azimuth)||fabs(azimuth)>2147483000.0f)return 0;
            int32_t a=(int32_t)azimuth;nflstreet_mesh_vertex *v=vertices+r*8+s;
            v->position[0]=sine(a+0x400000,cfg+7,cfg[15])*radius;
            v->position[1]=sine(a,cfg+7,cfg[15])*radius;
            v->position[2]=sine(elevation,cfg+7,cfg[15])*cfg[0];
        }
    }
    vertices[40].position[2]=cfg[0];
    for(unsigned i=0;i<41;++i)for(unsigned j=0;j<3;++j)if(!isfinite(vertices[i].position[j]))return 0;
    return 1;
}
static nflstreet_mesh_vertex colored(nflstreet_mesh_vertex v,unsigned color,const float *cfg){
    for(unsigned i=0;i<3;++i)v.rgba[i]=(uint8_t)(cfg[16+color*3+i]*255.0f);
    v.rgba[3]=255;return v;
}
int nflstreet_sky_mesh(const float cfg[31],nflstreet_mesh *out){
    nflstreet_mesh_vertex vertices[41];if(!out||!nflstreet_sky_vertices(cfg,vertices))return 0;
    nflstreet_mesh m={0};m.triangles=calloc(72,sizeof *m.triangles);if(!m.triangles)return 0;
    m.count=72;m.draws=2;m.material_count=1;strcpy(m.name,"Original procedural sky");
    unsigned at=0;
    for(unsigned r=0;r<4;++r)for(unsigned s=0;s<8;++s){
        unsigned next=(s+1)%8,idx[4]={(r+1)*8+s,(r+1)*8+next,r*8+next,r*8+s};
        unsigned upper=r?r:0,lower=r?r-1:0,col[4]={upper,upper,lower,lower};
        const unsigned tri[2][3]={{0,1,2},{0,2,3}};
        for(unsigned t=0;t<2;++t){nflstreet_mesh_triangle *a=m.triangles+at++;a->source_offset=0;
            for(unsigned k=0;k<3;++k)a->vertex[k]=colored(vertices[idx[tri[t][k]]],col[tri[t][k]],cfg);
        }
    }
    for(unsigned s=0;s<8;++s){nflstreet_mesh_triangle *a=m.triangles+at++;a->source_offset=1;
        a->vertex[0]=colored(vertices[32+s],3,cfg);a->vertex[1]=colored(vertices[32+(s+1)%8],3,cfg);a->vertex[2]=colored(vertices[40],4,cfg);
    }
    *out=m;return 1;
}
