#include "mesh_render.h"
#include <math.h>
#include <stdlib.h>
typedef struct {float x,y,z,u,v;} projected;
static float edge(projected a,projected b,float x,float y){return (x-a.x)*(b.y-a.y)-(y-a.y)*(b.x-a.x);}
static int render_view(const nflstreet_mesh *mesh,unsigned width,unsigned height,float yaw,float pitch,float zoom,int y_up,nflstreet_texture *out){
    if(!mesh||!mesh->count||!out||!width||!height||width>4096||height>4096||!isfinite(yaw)||!isfinite(pitch)||!isfinite(zoom)||zoom<0.1f||zoom>10)return 0;
    nflstreet_texture image={width,height,NULL};size_t count=(size_t)width*height;
    nflstreet_texture *textures=calloc(mesh->material_count?mesh->material_count:1,sizeof *textures);
    image.bgra=malloc(count*4);float *depth=malloc(count*sizeof *depth);
    if(!image.bgra||!depth||!textures){nflstreet_texture_close(&image);free(depth);free(textures);return 0;}
    /* Static OnePass texture resolution. Original TEV shading remains
       separate from this inspection renderer. */
    size_t texture_budget=256*1024*1024;
    for(unsigned i=0;i<mesh->material_count;++i){const nflstreet_mesh_material *m=mesh->materials+i;
        if(m->texture_index<0||!m->texture_bytes)continue;
        if(!nflstreet_texture_open_image(m->texture_bytes,m->texture_size,(unsigned)m->texture_index,(unsigned)m->palette_index,textures+i))goto fail;
        size_t bytes=(size_t)textures[i].width*textures[i].height*4;if(bytes>texture_budget)goto fail;texture_budget-=bytes;
    }
    for(size_t i=0;i<count;++i){image.bgra[i]=0xff20252d;depth[i]=-INFINITY;}
    float center[3],radius=0;
    for(unsigned i=0;i<3;++i){center[i]=(mesh->minimum[i]+mesh->maximum[i])*0.5f;float e=(mesh->maximum[i]-mesh->minimum[i])*0.5f;radius+=e*e;}
    radius=sqrtf(radius);if(!isfinite(radius)||radius<0.000001f)goto fail;
    float scale=(float)(width<height?width:height)*0.42f*zoom/radius;
    float cy=cosf(yaw),sy=sinf(yaw),cp=cosf(pitch),sp=sinf(pitch);
    for(size_t ti=0;ti<mesh->count;++ti){const nflstreet_mesh_triangle *t=mesh->triangles+ti;projected p[3];
        for(unsigned j=0;j<3;++j){const nflstreet_mesh_vertex *v=t->vertex+j;
            /* Explicit host inspection basis; neither is an original camera. */
            float x=v->position[0]-center[0];
            float y=v->position[y_up?1:2]-center[y_up?1:2];
            float z=y_up?v->position[2]-center[2]:-(v->position[1]-center[1]);
            float rx=cy*x+sy*z,rz=-sy*x+cy*z;
            p[j]=(projected){width*0.5f+rx*scale,height*0.5f-(cp*y-sp*rz)*scale,sp*y+cp*rz,v->uv[0],v->uv[1]};
        }
        float area=edge(p[0],p[1],p[2].x,p[2].y);if(fabsf(area)<0.0001f)continue;
        int left=(int)floorf(fminf(p[0].x,fminf(p[1].x,p[2].x))),right=(int)ceilf(fmaxf(p[0].x,fmaxf(p[1].x,p[2].x)));
        int top=(int)floorf(fminf(p[0].y,fminf(p[1].y,p[2].y))),bottom=(int)ceilf(fmaxf(p[0].y,fmaxf(p[1].y,p[2].y)));
        if(left<0)left=0;
        if(top<0)top=0;
        if(right>(int)width)right=(int)width;
        if(bottom>(int)height)bottom=(int)height;
        float ax=p[1].x-p[0].x,ay=p[1].y-p[0].y,az=(p[1].z-p[0].z)*scale;
        float bx=p[2].x-p[0].x,by=p[2].y-p[0].y,bz=(p[2].z-p[0].z)*scale;
        float nx=ay*bz-az*by,ny=az*bx-ax*bz,nz=ax*by-ay*bx;
        float norm=sqrtf(nx*nx+ny*ny+nz*nz),light=0.45f;
        if(norm>0)light+=0.55f*fabsf((nx*0.3f-ny*0.5f+nz*0.8f)/norm)/sqrtf(0.98f);
        for(int y=top;y<bottom;++y)for(int x=left;x<right;++x){
            float a=edge(p[1],p[2],x+0.5f,y+0.5f)/area,b=edge(p[2],p[0],x+0.5f,y+0.5f)/area,c=1-a-b;
            if(a<0||b<0||c<0)continue;
            float z=a*p[0].z+b*p[1].z+c*p[2].z;size_t at=(size_t)y*width+(unsigned)x;
            if(z<=depth[at])continue;
            uint32_t color=0xffc2b087;
            const nflstreet_texture *tex=t->material<mesh->material_count?textures+t->material:NULL;
            if(tex&&tex->bgra){float u=a*p[0].u+b*p[1].u+c*p[2].u,v=a*p[0].v+b*p[1].v+c*p[2].v;
                u-=floorf(u);v-=floorf(v);unsigned tx=(unsigned)(u*tex->width),ty=(unsigned)(v*tex->height);
                if(tx>=tex->width)tx=tex->width-1;
                if(ty>=tex->height)ty=tex->height-1;
                color=tex->bgra[(size_t)ty*tex->width+tx];
            }
            unsigned alpha=color>>24;if(!mesh->inspection_opaque&&alpha<128)continue;
            uint32_t lit=0xff000000;
            for(unsigned shift=0;shift<24;shift+=8){unsigned value=(unsigned)(((color>>shift)&255)*light);if(value>255)value=255;lit|=value<<shift;}
            image.bgra[at]=lit;depth[at]=z;
        }
    }
    for(unsigned i=0;i<mesh->material_count;++i)nflstreet_texture_close(textures+i);
    free(textures);free(depth);*out=image;return 1;
fail:for(unsigned i=0;i<mesh->material_count;++i)nflstreet_texture_close(textures+i);
    free(textures);nflstreet_texture_close(&image);free(depth);return 0;
}
int nflstreet_mesh_render_view(const nflstreet_mesh *mesh,unsigned width,unsigned height,float yaw,float pitch,float zoom,nflstreet_texture *out){
    return render_view(mesh,width,height,yaw,pitch,zoom,0,out);
}
int nflstreet_mesh_render_view_y_up(const nflstreet_mesh *mesh,unsigned width,unsigned height,float yaw,float pitch,float zoom,nflstreet_texture *out){
    return render_view(mesh,width,height,yaw,pitch,zoom,1,out);
}
int nflstreet_mesh_render(const nflstreet_mesh *mesh,unsigned width,unsigned height,float yaw,nflstreet_texture *out){
    return nflstreet_mesh_render_view(mesh,width,height,yaw,mesh&&mesh->inspection_opaque?0.75f:0.35f,1,out);
}
