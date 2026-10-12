#include "field_render.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
/* Semantics checked against public GX definitions and the local reference's
   VertexShaderManager, LightingShaderGen and PixelShaderGen. Native C only. */
typedef struct {float clip[4],uv[3],rgba[4];} vertex;
typedef struct {float x,y,z,invw,uv[3],rgba[4];} screen;
static int compare(float a,float b,unsigned op){
    switch(op){case 0:return 0;case 1:return a<b;case 2:return a==b;
    case 3:return a<=b;case 4:return a>b;case 5:return a!=b;case 6:return a>=b;default:return 1;}
}
static int alpha_test(unsigned alpha,uint32_t state){
    int a=compare((float)alpha,(float)(state&255),(state>>16)&7);
    int b=compare((float)alpha,(float)((state>>8)&255),(state>>19)&7);
    switch((state>>22)&3){case 0:return a&&b;case 1:return a||b;case 2:return a!=b;default:return a==b;}
}
static unsigned tev(unsigned tex,unsigned raster,int textured,unsigned scale){
    unsigned value=textured?(tex*(raster+(raster>>7))*scale+128)>>8:raster*scale;
    return value>255?255:value;
}
static float plane(const vertex *v,unsigned p){
    switch(p){case 0:return v->clip[3]+v->clip[0];case 1:return v->clip[3]-v->clip[0];
    case 2:return v->clip[3]+v->clip[1];case 3:return v->clip[3]-v->clip[1];
    case 4:return v->clip[3]+v->clip[2];default:return -v->clip[2];}
}
static vertex mix(vertex a,vertex b,float t){
    vertex out;
    for(unsigned i=0;i<4;++i){out.clip[i]=a.clip[i]+(b.clip[i]-a.clip[i])*t;out.rgba[i]=a.rgba[i]+(b.rgba[i]-a.rgba[i])*t;}
    for(unsigned i=0;i<3;++i)out.uv[i]=a.uv[i]+(b.uv[i]-a.uv[i])*t;
    return out;
}
static unsigned clip(vertex *v){
    unsigned n=3;vertex tmp[16];
    for(unsigned p=0;p<6&&n;++p){unsigned out=0;
        for(unsigned i=0;i<n;++i){vertex a=v[i],b=v[(i+1)%n];float da=plane(&a,p),db=plane(&b,p);
            if(da>=0){if(out>=16)return 0;tmp[out++]=a;}
            if((da>=0)!=(db>=0)){if(out>=16)return 0;tmp[out++]=mix(a,b,da/(da-db));}
        }
        memcpy(v,tmp,out*sizeof *v);n=out;
    }
    return n;
}
static int wrap(int x,unsigned size,unsigned mode){
    if(mode==0||mode==3){if(x<0)return 0;if(x>=(int)size)return (int)size-1;return x;}
    int period=mode==2?(int)size*2:(int)size;x%=period;if(x<0)x+=period;
    if(mode==2&&x>=(int)size)x=period-1-x;
    return x;
}
static void sample(const nflstreet_texture *t,float u,float v,uint32_t mode,unsigned rgba[4]){
    float coords[2]={u,v};
    for(unsigned i=0;i<2;++i){unsigned wrapmode=(mode>>(i*2))&3;
        if(wrapmode==1)coords[i]-=floorf(coords[i]);
        else if(wrapmode==2){coords[i]=fmodf(coords[i],2);if(coords[i]<0)coords[i]+=2;if(coords[i]>1)coords[i]=2-coords[i];}
        else coords[i]=fminf(1,fmaxf(0,coords[i]));
    }
    u=coords[0];v=coords[1];
    float x=u*t->width-0.5f,y=v*t->height-0.5f;
    int ix=(int)floorf(x),iy=(int)floorf(y);float fx=x-ix,fy=y-iy;
    /* Base-mip bilinear filtering. Derivative-based mip selection remains open. */
    for(unsigned c=0;c<4;++c){float value=0;unsigned shift=c==0?16:c==1?8:c==2?0:24;
        for(unsigned j=0;j<2;++j)for(unsigned i=0;i<2;++i){
            uint32_t pixel=t->bgra[(size_t)wrap(iy+(int)j,t->height,(mode>>2)&3)*t->width+(unsigned)wrap(ix+(int)i,t->width,mode&3)];
            value+=((pixel>>shift)&255)*(i?fx:1-fx)*(j?fy:1-fy);
        }
        rgba[c]=(unsigned)floorf(value+0.5f);
    }
}
static float edge(screen a,screen b,float x,float y){return (x-a.x)*(b.y-a.y)-(y-a.y)*(b.x-a.x);}
static void raster(vertex *v,const nflstreet_field_draw *draw,const nflstreet_texture *tex,
                   nflstreet_texture *image,float *depth,nflstreet_field_stats *stats){
    screen s[3];const uint32_t *state=draw->state;
    for(unsigned i=0;i<3;++i){float inv=1/v[i].clip[3];
        s[i].x=v[i].clip[0]*inv*draw->viewport[0]+draw->viewport[3]-342;
        s[i].y=v[i].clip[1]*inv*draw->viewport[1]+draw->viewport[4]-342;
        s[i].z=v[i].clip[2]*inv*draw->viewport[2]+draw->viewport[5];s[i].invw=inv;
        for(unsigned j=0;j<3;++j)s[i].uv[j]=v[i].uv[j]*inv;
        for(unsigned j=0;j<4;++j)s[i].rgba[j]=v[i].rgba[j]*inv;
    }
    float area=edge(s[0],s[1],s[2].x,s[2].y);if(!isfinite(area)||fabsf(area)<0.000001f)return;
    int left=(int)floorf(fminf(s[0].x,fminf(s[1].x,s[2].x))),right=(int)ceilf(fmaxf(s[0].x,fmaxf(s[1].x,s[2].x)));
    int top=(int)floorf(fminf(s[0].y,fminf(s[1].y,s[2].y))),bottom=(int)ceilf(fmaxf(s[0].y,fmaxf(s[1].y,s[2].y)));
    if(left<0)left=0;
    if(top<0)top=0;
    if(right>(int)image->width)right=(int)image->width;
    if(bottom>(int)image->height)bottom=(int)image->height;
    for(int y=top;y<bottom;++y)for(int x=left;x<right;++x){
        float a=edge(s[1],s[2],x+0.5f,y+0.5f)/area,b=edge(s[2],s[0],x+0.5f,y+0.5f)/area,c=1-a-b;
        if(a<0||b<0||c<0)continue;
        size_t at=(size_t)y*image->width+(unsigned)x;float z=floorf(a*s[0].z+b*s[1].z+c*s[2].z);
        if((state[2]&1)&&!compare(z,depth[at],(state[2]>>1)&7))continue;
        int early=(state[13]&64)!=0;
        if(early&&(state[2]&16))depth[at]=z;
        float invw=a*s[0].invw+b*s[1].invw+c*s[2].invw;
        float uv[3];for(unsigned i=0;i<3;++i)uv[i]=(a*s[0].uv[i]+b*s[1].uv[i]+c*s[2].uv[i])/invw;
        if(fabsf(uv[2])<0.000001f)continue;
        unsigned texture_rgba[4]={255,255,255,255},out[4];
        if(tex)sample(tex,uv[0]/uv[2],uv[1]/uv[2],state[7],texture_rgba);
        if(draw->actor_overlay){unsigned over[4];sample(draw->actor_overlay,uv[0]/uv[2],uv[1]/uv[2],draw->overlay_mode,over);
            /* Measured skin stages: base texture, alpha-interpolated overlay,
               then raster lighting at scale two. Alpha follows base alpha. */
            unsigned a=over[3]+(over[3]>>7);
            for(unsigned i=0;i<3;++i)texture_rgba[i]=(texture_rgba[i]*(256-a)+over[i]*a+128)>>8;
        }
        for(unsigned i=0;i<4;++i){float value=(a*s[0].rgba[i]+b*s[1].rgba[i]+c*s[2].rgba[i])/invw;
            unsigned ras=(unsigned)fminf(255,fmaxf(0,floorf(value+0.5f)));
            out[i]=tev(texture_rgba[i],ras,tex!=NULL,state[0]==6?1:2);
        }
        if(!alpha_test(out[3],state[6]))continue;
        if(!early&&(state[2]&16))depth[at]=z;
        if(state[3]&8){uint32_t dst=image->bgra[at],result=dst&0xff000000;
            for(unsigned i=0;i<3;++i){unsigned shift=i==0?16:i==1?8:0,value=out[i];
                if(state[3]&1)value=(out[i]*out[3]+((dst>>shift)&255)*(255-out[3])+127)/255;
                result|=value<<shift;
            }
            image->bgra[at]=result;
        }
        if(state[3]&16)image->bgra[at]=(image->bgra[at]&0x00ffffff)|(out[3]<<24);
        ++stats->shaded;
    }
}
static int valid(const nflstreet_field_draw *d){
    const uint32_t *s=d->state;
    int sky=s[0]==6,actor=s[0]==7;
    if(sky){
        if(s[2]!=6||s[3]!=0x54b8||s[4]!=0x8fffa||s[5]!=0x8ffd0||s[10]!=0x705||s[11]!=0x404||s[12]!=0x282||s[15]!=UINT32_MAX||
           d->camera[3]!=0||d->camera[7]!=0||d->camera[11]!=0||d->projection[4]!=0)return 0;
    }else if(actor){
        if(!d->actor_texture||s[2]!=23||(s[3]!=0x54b8&&s[3]!=0x54b9)||s[4]!=0x18f8af||s[5]!=0x18f2f0||s[10]!=0x51e||(s[11]!=0x51e&&s[11]!=0x51c)||s[12]!=0x280||s[15]!=UINT32_MAX)return 0;
    }else if(s[0]>=6||s[10]!=0x403||s[11]!=0x403||s[3]!=0x54b9||
       (s[4]!=0x18f8af&&s[4]!=0x18fffa)||(s[5]!=0x18f2f0&&s[5]!=0x18ffd0)||
       (s[12]!=0x280&&s[12]!=0x282))return 0;
    if(!sky&&!actor&&(s[15]==UINT32_MAX)!=(s[4]==0x18fffa))return 0;
    const float *arrays[]={d->camera,d->projection,d->viewport,d->texmatrix};unsigned sizes[]={12,6,6,12};
    for(unsigned i=0;i<4;++i)for(unsigned j=0;j<sizes[i];++j)if(!isfinite(arrays[i][j]))return 0;
    return d->projection[0]>0&&d->projection[2]>0&&d->projection[5]<0&&
           d->viewport[0]>0&&d->viewport[1]<0&&fabsf(d->viewport[0])<=4096&&fabsf(d->viewport[1])<=4096&&
           fabsf(d->viewport[3])<=8192&&fabsf(d->viewport[4])<=8192;
}
static int render(const nflstreet_mesh meshes[6],const nflstreet_mesh *sky,const nflstreet_mesh *player,const nflstreet_field_draw *draws,unsigned count,
                           unsigned width,unsigned height,nflstreet_texture *out,nflstreet_field_stats *result,
                           const nflstreet_field_surface *seed,float **retained_depth){
    if(!meshes||!draws||!count||count>20000||!width||!height||width>4096||height>4096||!out||!result)return 0;
    nflstreet_texture image={width,height,NULL},*textures[6]={0};unsigned texture_count[6]={0};
    float *depth=NULL;nflstreet_field_stats stats={0};
    image.bgra=malloc((size_t)width*height*4);depth=malloc((size_t)width*height*sizeof *depth);
    if(!image.bgra||!depth)goto fail;
    if(seed){if(!seed->depth||!seed->image.bgra||seed->image.width!=width||seed->image.height!=height)goto fail;
        memcpy(image.bgra,seed->image.bgra,(size_t)width*height*4);memcpy(depth,seed->depth,(size_t)width*height*sizeof *depth);
    }else for(size_t i=0;i<(size_t)width*height;++i){image.bgra[i]=0xff20252d;depth[i]=16777215;}
    size_t budget=256*1024*1024;
    for(unsigned di=0;di<count;++di){const nflstreet_field_draw *d=draws+di;if(!valid(d))goto fail;
        unsigned layer=d->state[0];const nflstreet_mesh *mesh=layer==7?player:layer==6?sky:meshes+layer;
        if(!mesh)goto fail;
        if(d->state[14]>=mesh->material_count)goto fail;
        const nflstreet_texture *tex=layer==7?d->actor_texture:NULL;
        if(d->state[15]!=UINT32_MAX){unsigned image_index=d->state[15];
            if(image_index>=4096||!mesh->texture_bytes)goto fail;
            if(texture_count[layer]<=image_index){unsigned n=image_index+1;nflstreet_texture *next=calloc(n,sizeof *next);
                if(!next)goto fail;
                if(textures[layer])memcpy(next,textures[layer],texture_count[layer]*sizeof *next);
                free(textures[layer]);textures[layer]=next;texture_count[layer]=n;
            }
            nflstreet_texture *cached=textures[layer]+image_index;
            if(!cached->bgra){if(!nflstreet_texture_open_image(mesh->texture_bytes,mesh->texture_size,image_index,0,cached))goto fail;
                size_t bytes=(size_t)cached->width*cached->height*4;if(bytes>budget)goto fail;budget-=bytes;
            }
            tex=cached;
        }
        unsigned selected=0;
        for(size_t ti=0;ti<mesh->count;++ti){const nflstreet_mesh_triangle *t=mesh->triangles+ti;
            if(t->source_offset!=d->state[1])continue;
            if(t->material!=d->state[14])goto fail;
            vertex v[16];++selected;++stats.triangles;
            for(unsigned i=0;i<3;++i){const nflstreet_mesh_vertex *source=t->vertex+i;float eye[3];
                for(unsigned j=0;j<3;++j){eye[j]=d->camera[j*4+3];for(unsigned k=0;k<3;++k)eye[j]+=d->camera[j*4+k]*source->position[k];}
                v[i].clip[0]=d->projection[0]*eye[0]+d->projection[1]*eye[2];
                v[i].clip[1]=d->projection[2]*eye[1]+d->projection[3]*eye[2];
                v[i].clip[2]=d->projection[4]*eye[2]+d->projection[5];v[i].clip[3]=-eye[2];
                for(unsigned j=0;j<3;++j)v[i].uv[j]=d->texmatrix[j*4]*source->uv[0]+d->texmatrix[j*4+1]*source->uv[1]+d->texmatrix[j*4+2]+d->texmatrix[j*4+3];
                if(d->state[12]&2)v[i].uv[2]=1;
                for(unsigned j=0;j<4;++j){
                    if(layer==6){v[i].rgba[j]=(float)(j==3?(d->state[8]&255):source->rgba[j]);continue;}
                    if(layer==7){v[i].rgba[j]=source->rgba[j];continue;}
                    unsigned ambient=(d->state[9]>>(24-j*8))&255;
                    v[i].rgba[j]=(float)((source->rgba[j]*(ambient+(ambient>>7)))>>8);
                }
                for(unsigned j=0;j<4;++j)if(!isfinite(v[i].clip[j]))goto fail;
                for(unsigned j=0;j<3;++j)if(!isfinite(v[i].uv[j]))goto fail;
            }
            unsigned n=clip(v);if(n!=3)++stats.clipped;
            for(unsigned i=2;i<n;++i){vertex tri[3]={v[0],v[i-1],v[i]};raster(tri,d,tex,&image,depth,&stats);}
        }
        if(!selected)goto fail;
        ++stats.commands;
    }
    for(unsigned i=0;i<6;++i){for(unsigned j=0;j<texture_count[i];++j)nflstreet_texture_close(textures[i]+j);free(textures[i]);}
    if(retained_depth)*retained_depth=depth;else free(depth);*out=image;*result=stats;return 1;
fail:for(unsigned i=0;i<6;++i){for(unsigned j=0;j<texture_count[i];++j)nflstreet_texture_close(textures[i]+j);free(textures[i]);}
    free(depth);nflstreet_texture_close(&image);return 0;
}
int nflstreet_field_render_player(const nflstreet_mesh meshes[6],const nflstreet_mesh *sky,const nflstreet_mesh *player,const nflstreet_field_draw *draws,unsigned count,
                           unsigned width,unsigned height,nflstreet_texture *out,nflstreet_field_stats *result){
    return render(meshes,sky,player,draws,count,width,height,out,result,NULL,NULL);
}
int nflstreet_field_prepare(const nflstreet_mesh meshes[6],const nflstreet_mesh *sky,const nflstreet_field_draw *draws,unsigned count,
                           unsigned width,unsigned height,nflstreet_field_surface *out,nflstreet_field_stats *stats){
    if(!out)return 0;
    nflstreet_field_surface surface={0};
    if(!render(meshes,sky,NULL,draws,count,width,height,&surface.image,stats,NULL,&surface.depth))return 0;
    *out=surface;return 1;
}
int nflstreet_field_overlay(const nflstreet_field_surface *surface,const nflstreet_mesh *player,const nflstreet_field_draw *draws,unsigned count,
                           nflstreet_texture *out,nflstreet_field_stats *stats){
    if(!surface||!player||!draws)return 0;
    for(unsigned i=0;i<count;++i)if(draws[i].state[0]!=7)return 0;
    nflstreet_mesh empty[6]={0};return render(empty,NULL,player,draws,count,surface->image.width,surface->image.height,out,stats,surface,NULL);
}
int nflstreet_field_layer(const nflstreet_field_surface *seed,const nflstreet_mesh *player,const nflstreet_field_draw *draws,unsigned count,
                         nflstreet_field_surface *out,nflstreet_field_stats *stats){
    if(!seed||!player||!draws||!out||seed==out)return 0;
    for(unsigned i=0;i<count;++i)if(draws[i].state[0]!=7)return 0;
    nflstreet_mesh empty[6]={0};nflstreet_field_surface next={0};
    if(!render(empty,NULL,player,draws,count,seed->image.width,seed->image.height,&next.image,stats,seed,&next.depth))return 0;
    *out=next;return 1;
}
void nflstreet_field_surface_close(nflstreet_field_surface *surface){if(surface){nflstreet_texture_close(&surface->image);free(surface->depth);memset(surface,0,sizeof *surface);}}

int nflstreet_field_render(const nflstreet_mesh meshes[6],const nflstreet_field_draw *draws,unsigned count,
                           unsigned width,unsigned height,nflstreet_texture *out,nflstreet_field_stats *result){
    return nflstreet_field_render_sky(meshes,NULL,draws,count,width,height,out,result);
}
int nflstreet_field_render_sky(const nflstreet_mesh meshes[6],const nflstreet_mesh *sky,const nflstreet_field_draw *draws,unsigned count,
                              unsigned width,unsigned height,nflstreet_texture *out,nflstreet_field_stats *result){
    return nflstreet_field_render_player(meshes,sky,NULL,draws,count,width,height,out,result);
}
