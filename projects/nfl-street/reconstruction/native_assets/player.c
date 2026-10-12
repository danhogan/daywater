#include "player.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define RECORD 3876u
static const char *categories[]={"plamodel","plaface","plahair","plahat","plaglass","plarosterhair","statmod"};
static unsigned be16(const uint8_t *p){return (unsigned)p[0]<<8|p[1];}
static uint32_t be32(const uint8_t *p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static uint32_t le32(const uint8_t *p){return p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static float real(const uint8_t *p){uint32_t u=le32(p);float f;memcpy(&f,&u,4);return f;}
static int span(size_t n,size_t o,size_t s){return o<=n&&s<=n-o;}
static uint8_t *asset(const char *root,const char *category,unsigned entry,size_t *size){
    char path[4096];if(entry>99999||snprintf(path,sizeof path,"%s/%s/%05u.bin",root,category,entry)>=(int)sizeof path)return NULL;
    FILE *f=fopen(path,"rb");if(!f)return NULL;
    if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}long n=ftell(f);
    if(n<16||n>16*1024*1024||fseek(f,0,SEEK_SET)){fclose(f);return NULL;}
    uint8_t *b=malloc((size_t)n);if(!b){fclose(f);return NULL;}
    int ok=fread(b,1,(size_t)n,f)==(size_t)n;fclose(f);if(!ok){free(b);return NULL;}*size=(size_t)n;return b;
}
static void normalize(float v[3]){float n=sqrtf(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);if(n>0)for(unsigned i=0;i<3;++i)v[i]/=n;}
static int vertex(const uint8_t *b,size_t n,const uint8_t *attrs,unsigned count,const uint8_t **at,const uint8_t *end,
                  const uint8_t *r,unsigned indexed_matrix,nflstreet_mesh_vertex *v){
    if(*at==end)return 0;
    unsigned slot=indexed_matrix?*(*at)++:0;
    if(slot>63)return 0;
    float pos[3]={0},normal[3]={0};int gotpos=0,gotnormal=0;memset(v,0,sizeof *v);
    for(unsigned i=0;i<count;++i){const uint8_t *a=attrs+i*12;unsigned mode=a[11]&15,index,type=a[9],frac=a[10],kind=a[6];
        if((mode!=2&&mode!=3)||(size_t)(end-*at)<mode-1)return 0;
        index=mode==2?*(*at)++:be16(*at);if(mode==3)*at+=2;
        size_t off=be32(a)+(size_t)index*a[7];if(index>=be16(a+4)||!span(n,off,a[7]))return 0;
        if(kind!=9&&kind!=10&&kind!=13)continue;
        unsigned components=kind==13?2:3;
        if(frac>31||(type!=1&&type!=3)||a[7]<components*(type==1?1u:2u))return 0;
        for(unsigned j=0;j<components;++j){int value=type==1?(int)(int8_t)b[off+j]:(int)(int16_t)be16(b+off+j*2);
            float x=ldexpf((float)value,-(int)frac);
            if(kind==9)pos[j]=x;else if(kind==10)normal[j]=x;else v->uv[j]=x;
        }
        gotpos|=kind==9;gotnormal|=kind==10;
    }
    if(!gotpos||!gotnormal)return 0;
    const uint8_t *p=r+256+slot*16,*nm=r+1312+(slot&31)*12;
    float transformed[3];
    for(unsigned j=0;j<3;++j){v->position[j]=real(p+j*16+12);transformed[j]=0;
        for(unsigned k=0;k<3;++k){v->position[j]+=real(p+j*16+k*4)*pos[k];transformed[j]+=real(nm+j*12+k*4)*normal[k];}
        if(!isfinite(v->position[j])||!isfinite(transformed[j]))return 0;
    }
    normalize(transformed);
    const uint8_t *lights=r+2336;const uint8_t *state=r+48;
    uint32_t ambient=le32(state+9*4),material=le32(state+8*4);
    for(unsigned c=0;c<4;++c){if(!(le32(state+(c==3?11:10)*4)&2)){v->rgba[c]=(uint8_t)(material>>(24-c*8));continue;}int light=(ambient>>(24-c*8))&255;
        for(unsigned l=0;l<3;++l){const uint8_t *source=lights+l*64;float direction[3];
            for(unsigned j=0;j<3;++j)direction[j]=real(source+40+j*4)-v->position[j];
            normalize(direction);
            float diffuse=0;for(unsigned j=0;j<3;++j)diffuse+=direction[j]*transformed[j];diffuse=fmaxf(0,diffuse);
            unsigned channel=(le32(source+12)>>(24-c*8))&255;light+=(int)nearbyintf(diffuse*channel);
        }
        if(light>255)light=255;
        if(light<0)light=0;
        v->rgba[c]=(uint8_t)((((material>>(24-c*8))&255)*(unsigned)(light+(light>>7)))>>8);
    }
    return 1;
}
static int append(nflstreet_mesh *m,const nflstreet_mesh_vertex *a,const nflstreet_mesh_vertex *b,const nflstreet_mesh_vertex *c,unsigned id){
    if(m->count>=30000)return 0;
    nflstreet_mesh_triangle *t=m->triangles+m->count++;
    t->vertex[0]=*a;t->vertex[1]=*b;t->vertex[2]=*c;t->material=(uint16_t)id;t->source_offset=id;return 1;
}
static int primitive(nflstreet_mesh *mesh,const uint8_t *b,size_t n,const uint8_t *r,unsigned id){
    if(n<16||memcmp(b,"TMdl",4)||be32(b+4)>n)return 0;
    size_t directory=be16(b+10),geom=0;unsigned sections=be16(b+8);if(!span(n,directory,sections*16))return 0;
    for(unsigned i=0;i<sections;++i){const uint8_t *s=b+directory+i*16;if(!memcmp(s,"Geom",4))geom=be32(s+4);}
    if(!geom||!span(n,geom,16))return 0;
    unsigned rowi=le32(r+8);if(rowi>=be32(b+geom+4))return 0;
    size_t row=be32(b+geom)+(size_t)rowi*24;if(!span(n,row,24))return 0;
    size_t off=le32(r+12),begin=be32(b+row),endoff=begin+(size_t)be16(b+row+4)*32;
    if(be16(b+row+6)!=le32(r+20)||off<begin||off>=endoff||!span(n,off,3)||!span(n,begin,endoff-begin))return 0;
    size_t attrs=be32(b+geom+8)+(size_t)be16(b+row+14)*12;unsigned count=b[row+16];if(!span(n,attrs,count*12))return 0;
    const uint8_t *at=b+off,*end=b+endoff;unsigned cmd=*at++,vertices=be16(at);at+=2;
    if(vertices!=le32(r+16)||vertices<3||(cmd!=0x80&&cmd!=0x90&&cmd!=0x98&&cmd!=0xa0))return 0;
    if((cmd==0x80&&vertices%4)||(cmd==0x90&&vertices%3))return 0;
    nflstreet_mesh_vertex first={0},prev={0},older={0},quad[4];
    for(unsigned i=0;i<vertices;++i){nflstreet_mesh_vertex v;if(!vertex(b,n,b+attrs,count,&at,end,r,b[row+17]&1,&v))return 0;
        if(!i)first=v;
        if(cmd==0x80){quad[i%4]=v;if(i%4==3&&(!append(mesh,quad,quad+1,quad+2,id)||!append(mesh,quad,quad+2,quad+3,id)))return 0;}
        else if(cmd==0x90){if(i%3==2&&!append(mesh,&older,&prev,&v,id))return 0;}
        else if(i>=2){if(cmd==0xa0){if(!append(mesh,&first,&prev,&v,id))return 0;}
            else if(i&1){if(!append(mesh,&prev,&older,&v,id))return 0;}
            else if(!append(mesh,&older,&prev,&v,id))return 0;
        }older=prev;prev=v;
    }
    return 1;
}
static int texture(const char *root,const uint8_t *r,unsigned unit,nflstreet_texture *out){
    unsigned entry=le32(r+24+unit*12),image=le32(r+28+unit*12),fmt=le32(r+32+unit*12);size_t size;
    uint8_t *b=asset(root,le32(r)==6?"statmod":"platex",entry,&size);if(!b)return 0;
    int ok=0;if(size<40)goto done;size_t record=be32(b+24)+(size_t)image*16,pd=be32(b+28);
    if(size>=40&&!memcmp(b,"MMAP",4)&&span(size,record,16)&&be16(b+record+4)==14){ok=nflstreet_texture_open_image(b,size,image,0,out);goto done;}
    if(size<40||memcmp(b,"MMAP",4)||image>=be16(b+14)||!span(size,record,16)||!span(size,pd,12)||!be16(b+16)||fmt>2)goto done;
    unsigned tf=be16(b+record+4),count=tf==8?32:tf==9?512:0;size_t pal=be32(b+pd+8);
    if(!count||be32(b+pd+4)<count||!span(size,pal,count))goto done;
    b[pd+2]=0;b[pd+3]=(uint8_t)fmt;memcpy(b+pal,r+2848+unit*512,count);
    ok=nflstreet_texture_open_image(b,size,image,0,out);
done:free(b);return ok;
}
int nflstreet_player_open(const char *root,const uint8_t *b,size_t n,nflstreet_player *out){
    if(!root||!b||!out||n<16||memcmp(b,"NAP1",4))return 0;
    unsigned count=le32(b+4);if(!count||count>2000||n!=16+(size_t)count*RECORD)return 0;
    nflstreet_player p={0};p.count=count;p.mesh.material_count=count;p.mesh.draws=count;
    strcpy(p.mesh.name,"Captured fixed-pose player");p.mesh.triangles=calloc(30000,sizeof *p.mesh.triangles);
    p.draws=calloc(count,sizeof *p.draws);p.textures=calloc(count*2,sizeof *p.textures);if(!p.mesh.triangles||!p.draws||!p.textures)goto fail;
    /* Deduplicate decoded planes by original selector + measured palette. */
    unsigned texture_keys[4000];
    for(unsigned i=0;i<count;++i){const uint8_t *r=b+16+(size_t)i*RECORD;unsigned cat=le32(r);size_t size;
        if(cat>=7)goto fail;
        uint8_t *model=asset(root,categories[cat],le32(r+4),&size);if(!model)goto fail;
        int ok=primitive(&p.mesh,model,size,r,i);free(model);if(!ok){fprintf(stderr,"Player primitive %u: category %u entry %u row %u offset %u unsupported\n",i,cat,le32(r+4),le32(r+8),le32(r+12));goto fail;}
        nflstreet_field_draw *d=p.draws+i;for(unsigned j=0;j<16;++j)d->state[j]=le32(r+48+j*4);
        float *arrays[]={d->camera,d->projection,d->viewport,d->texmatrix};unsigned lengths[]={12,6,6,12},at=16;
        for(unsigned j=0;j<4;++j)for(unsigned k=0;k<lengths[j];++k)arrays[j][k]=real(r+48+at++*4);
        d->state[0]=7;d->state[1]=d->state[14]=i;d->state[15]=UINT32_MAX;
        for(unsigned u=0;u<2;++u){if(le32(r+24+u*12)==UINT32_MAX)continue;
            unsigned key=i*2+u,index=p.texture_count;
            for(unsigned t=0;t<p.texture_count;++t){unsigned old=texture_keys[t];const uint8_t *rr=b+16+(size_t)(old/2)*RECORD;unsigned ou=old%2;
                if(!memcmp(r+24+u*12,rr+24+ou*12,12)&&!memcmp(r+2848+u*512,rr+2848+ou*512,512)){index=t;break;}
            }
            if(index==p.texture_count){if(!texture(root,r,u,p.textures+index)){fprintf(stderr,"Player texture %u unit %u: entry %u image %u format %u unsupported\n",i,u,le32(r+24+u*12),le32(r+28+u*12),le32(r+32+u*12));goto fail;}texture_keys[p.texture_count++]=key;}
            if(u==0)d->actor_texture=p.textures+index;else d->actor_overlay=p.textures+index;
        }
        d->overlay_mode=le32(r+3872);
    }
    for(size_t i=0;i<p.mesh.count;++i)for(unsigned v=0;v<3;++v)for(unsigned j=0;j<3;++j){float x=p.mesh.triangles[i].vertex[v].position[j];
        if(!i&&!v)p.mesh.minimum[j]=p.mesh.maximum[j]=x;
        if(x<p.mesh.minimum[j])p.mesh.minimum[j]=x;
        if(x>p.mesh.maximum[j])p.mesh.maximum[j]=x;
    }
    *out=p;return 1;
fail:nflstreet_player_close(&p);return 0;
}
void nflstreet_player_close(nflstreet_player *p){if(!p)return;for(unsigned i=0;i<p->texture_count;++i)nflstreet_texture_close(p->textures+i);
    free(p->textures);free(p->draws);nflstreet_mesh_close(&p->mesh);memset(p,0,sizeof *p);}
int nflstreet_player_repose(nflstreet_player *p,const uint8_t *b,size_t n,const nflstreet_player_source *sources){
    if(!p||!b||!sources||!p->mesh.triangles||n!=16+(size_t)p->count*RECORD||memcmp(b,"NAP1",4)||le32(b+4)!=p->count)return 0;
    p->mesh.count=0;
    for(unsigned i=0;i<p->count;++i)if(!primitive(&p->mesh,sources[i].bytes,sources[i].size,b+16+(size_t)i*RECORD,i))return 0;
    if(!p->mesh.count)return 0;
    for(size_t i=0;i<p->mesh.count;++i)for(unsigned v=0;v<3;++v)for(unsigned j=0;j<3;++j){float x=p->mesh.triangles[i].vertex[v].position[j];
        if(!i&&!v)p->mesh.minimum[j]=p->mesh.maximum[j]=x;
        if(x<p->mesh.minimum[j])p->mesh.minimum[j]=x;if(x>p->mesh.maximum[j])p->mesh.maximum[j]=x;
    }
    return 1;
}
int nflstreet_player_portrait(const nflstreet_player *p,nflstreet_texture *out){
    if(!p||!p->count||!out)return 0;
    nflstreet_field_draw *draws=malloc((size_t)p->count*sizeof *draws);if(!draws)return 0;
    memcpy(draws,p->draws,(size_t)p->count*sizeof *draws);float center[3],radius=0;
    for(unsigned j=0;j<3;++j){center[j]=(p->mesh.minimum[j]+p->mesh.maximum[j])*0.5f;float e=(p->mesh.maximum[j]-p->mesh.minimum[j])*0.5f;radius+=e*e;}
    radius=sqrtf(radius);if(!isfinite(radius)||radius<0.001f){free(draws);return 0;}
    for(unsigned i=0;i<p->count;++i){for(unsigned j=0;j<3;++j)draws[i].camera[j*4+3]=-center[j];draws[i].camera[11]-=radius*3;}
    nflstreet_mesh unused[6]={0};nflstreet_field_stats stats;
    int ok=nflstreet_field_render_player(unused,NULL,&p->mesh,draws,p->count,640,448,out,&stats);free(draws);return ok;
}
