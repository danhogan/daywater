#include "camera_projection.h"
#include "marker_ram.h"

static int screen_mapping(nflstreet_draw *d,uint32_t out,float left,float top,float width,float height,float depth){
 float k,zero,one;uint32_t a=0x80263cf8u;
 if(!mk_float(d,a,&k)||!mk_float(d,a+4,&zero)||!mk_float(d,a+8,&one))return 0;
 const float sx=width*k,sy=height*k,sz=depth*k;
 /* 801DF8C4 writes the depth pair at words 10 and 11: -scale, +scale. */
 float m[16]={sx,zero,zero,left+sx, zero,sy,zero,top+sy, zero,zero,-sz,sz, zero,zero,zero,one};
 for(unsigned i=0;i<16;++i)if(!mk_putf(d,out+4*i,m[i]))return 0;return 1;
}
static int perspective(nflstreet_draw *d,uint32_t out,uint32_t r13,float top,float bottom,float left,float right,float near_z,float far_z){
 float c10,c8,c0,c14; if(!mk_float(d,r13-0x4cf8u,&c10)||!mk_float(d,r13-0x4cf4u,&c8)||
    !mk_float(d,r13-0x4cf0u,&c0)||!mk_float(d,r13-0x4cecu,&c14))return 0;
 /* 801FFF64: each operation rounds to binary32, matching fsubs/fdivs/fmuls. */
 const float dx=right-left,dy=top-bottom,dz=far_z-near_z;
 const float x=c10/dx,y=c10/dy,z=c10/dz;
 const float near_term=c8*near_z;
 const float xx=near_term*x,xy=x*(right+left),yy=near_term*y,yx=y*(top+bottom);
 const float zz=(-near_z)*z,zw=z*(-(far_z*near_z));
 float m[16]={xx,c0,xy,c0, c0,yy,yx,c0, c0,c0,zz,zw, c0,c0,c14,c0};
 for(unsigned i=0;i<16;++i)if(!mk_putf(d,out+4*i,m[i]))return 0;return 1;
}
static int asymmetric(nflstreet_draw *d,uint32_t out,float left,float right,float bottom,float top,float near_z,float far_z){
 float c4,c8,c0,c14;uint32_t k=0x80263d04u;
 if(!mk_float(d,k,&c4)||!mk_float(d,k+4,&c8)||!mk_float(d,k+8,&c0)||!mk_float(d,k+12,&c14))return 0;
 const float rt=top+bottom,lr=right+left,nf=(-far_z)+(-near_z);
 const float dt=top-bottom,dl=right-left,dn=(-far_z)-(-near_z);
 const float a=c4/dl,b=c4/dt,c=c8/dn;
 float m[16]={a,c0,c0,-(lr/dl), c0,b,c0,-(rt/dt), c0,c0,c,-(nf/dn), c0,c0,c0,c14};
 /* The final depth translation is exactly -((far+near)/(far-near)); the
    explicit negated endpoints above match 801DF970's fneg inputs. */
 m[11]=-(nf/dn);
 for(unsigned i=0;i<16;++i)if(!mk_putf(d,out+4*i,m[i]))return 0;return 1;
}
int nflstreet_camera_projection(const nflstreet_view_context *c,uint32_t node,float viewport_scale){
 static nflstreet_render_state unused;
 if(!c||!c->x||!c->x->span||!node)return 0;
 nflstreet_draw dd={c->x,&unused},*d=&dd;
 float left,top,width,height,l,r,b,t,n,f;uint32_t flags;
 if(!mk_float(d,node+4,&left)||!mk_float(d,node+8,&top)||!mk_float(d,node+0x14,&width)||!mk_float(d,node+0x18,&height)||
    !mk_word(d,node+0x1c,&flags)||!mk_float(d,node+0xd8,&l)||!mk_float(d,node+0xdc,&r)||
    !mk_float(d,node+0xe4,&b)||!mk_float(d,node+0xe0,&t)||!mk_float(d,node+0xe8,&n)||!mk_float(d,node+0xec,&f))return 0;
 if(!screen_mapping(d,c->slots+0x40,left,top,width,height,viewport_scale))return 0;
 if(flags&4u)return asymmetric(d,c->slots+0x80,l,r,b,t,n,f);
 return perspective(d,c->slots+0x80,c->r13,t,b,l,r,n,f);
}
