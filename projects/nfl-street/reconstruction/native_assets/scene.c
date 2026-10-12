#include "scene.h"
#include "environment.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned u16(const uint8_t *p){return (unsigned)p[0]<<8|p[1];}
static uint32_t u32(const uint8_t *p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static int span(size_t size,size_t off,size_t n){return off<=size&&n<=size-off;}
static int name(const uint8_t *b,size_t n,char *out,size_t cap){
    if(n<16||memcmp(b,"TMdl",4)||!span(n,u16(b+10),(size_t)u16(b+8)*16))return 0;
    unsigned found=0;
    for(unsigned i=0;i<u16(b+8);++i){const uint8_t *s=b+u16(b+10)+i*16;
        if(memcmp(s,"Info",4))continue;
        size_t off=u32(s+4),len=u32(s+8);if(++found!=1||!span(n,off,len))return 0;
        const uint8_t *end=memchr(b+off,0,len);if(!end||(size_t)(end-b-off)>=cap)return 0;
        memcpy(out,b+off,(size_t)(end-b-off)+1);
    }
    return found==1;
}
int nflstreet_scene_open(const nflstreet_archive *a,const char *venue,nflstreet_scene *out){
    if(!a||!venue||!out||!*venue||strlen(venue)>16)return 0;
    for(const char *p=venue;*p;++p)if(!((*p>='A'&&*p<='Z')||(*p>='a'&&*p<='z')))return 0;
    nflstreet_scene scene={0};nflstreet_mesh meshes[NFLSTREET_SCENE_LAYERS]={0};
    char expected[NFLSTREET_SCENE_LAYERS][64];
    snprintf(expected[0],sizeof expected[0],"%s_rField.ea3",venue);
    for(unsigned i=1;i<NFLSTREET_SCENE_LAYERS;++i)snprintf(expected[i],sizeof expected[i],"%s_FieldPass%u.ea3",venue,i);
    size_t count=0;unsigned materials=0;
    for(unsigned i=0;i<a->count;++i){nflstreet_archive_entry entry;uint8_t *b=NULL;size_t n;char asset[128];
        if(!nflstreet_archive_get(a,i,&entry)||!nflstreet_archive_decode(&entry,&b,&n))goto fail;
        int selected=-1;
        if(name(b,n,asset,sizeof asset))for(unsigned j=0;j<NFLSTREET_SCENE_LAYERS;++j)if(!strcmp(asset,expected[j]))selected=(int)j;
        if(selected<0){free(b);continue;}
        if(scene.owned[selected]){free(b);goto fail;}
        scene.owned[selected]=b;scene.entries[selected]=i;
        if(!nflstreet_mesh_open_inspection(b,n,meshes+selected))goto fail;
        if(meshes[selected].count>2000000-count||meshes[selected].material_count>65535-materials)goto fail;
        scene.triangles[selected]=meshes[selected].count;count+=meshes[selected].count;materials+=meshes[selected].material_count;++scene.layers;
    }
    if(scene.layers!=NFLSTREET_SCENE_LAYERS||!materials)goto fail;
    scene.mesh.triangles=malloc(count*sizeof *scene.mesh.triangles);
    scene.mesh.materials=malloc(materials*sizeof *scene.mesh.materials);
    if(!scene.mesh.triangles||!scene.mesh.materials)goto fail;
    snprintf(scene.mesh.name,sizeof scene.mesh.name,"%s environment layers (inspection)",venue);
    unsigned offset=0;size_t at=0;
    for(unsigned i=0;i<NFLSTREET_SCENE_LAYERS;++i){nflstreet_mesh *m=meshes+i;
        memcpy(scene.mesh.materials+offset,m->materials,m->material_count*sizeof *m->materials);
        for(size_t j=0;j<m->count;++j){scene.mesh.triangles[at]=m->triangles[j];
            scene.mesh.triangles[at++].material=(uint16_t)(m->triangles[j].material+offset);
        }
        for(unsigned j=0;j<3;++j){if(!i||m->minimum[j]<scene.mesh.minimum[j])scene.mesh.minimum[j]=m->minimum[j];
            if(!i||m->maximum[j]>scene.mesh.maximum[j])scene.mesh.maximum[j]=m->maximum[j];}
        scene.mesh.draws+=m->draws;scene.mesh.ignored_color_indices+=m->ignored_color_indices;offset+=m->material_count;
    }
    scene.mesh.count=count;scene.mesh.material_count=materials;
    scene.mesh.inspection_opaque=1;
    for(unsigned i=0;i<NFLSTREET_SCENE_LAYERS;++i)nflstreet_mesh_close(meshes+i);
    *out=scene;return 1;
fail:for(unsigned i=0;i<NFLSTREET_SCENE_LAYERS;++i)nflstreet_mesh_close(meshes+i);
    nflstreet_scene_close(&scene);return 0;
}
void nflstreet_scene_close(nflstreet_scene *s){if(s){nflstreet_mesh_close(&s->mesh);
    for(unsigned i=0;i<NFLSTREET_SCENE_LAYERS;++i)free(s->owned[i]);
    for(unsigned i=0;i<s->prop_owned_count;++i)free(s->prop_owned[i]);
    memset(s,0,sizeof *s);
}}
