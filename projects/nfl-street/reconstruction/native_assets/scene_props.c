#include "scene.h"
#include "environment.h"
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int space(uint8_t c){return c==' '||c=='\t'||c=='\r'||c=='\n';}
/* Narrow bounded XML attribute reader for the original object definitions.
   Ignores comments, accepts quoted attributes, rejects entities/duplicates.
   Returns 0 for no render tag, -1 for unsupported XML, 1 for a reference. */
static int render_reference(const uint8_t *b,size_t n,char out[128]){
    size_t at=0;int found=0;
    while(at<n){if(b[at++]!='<')continue;
        if(at+3<=n&&!memcmp(b+at,"!--",3)){
            at+=3;while(at+3<=n&&memcmp(b+at,"-->",3))++at;
            if(at+3>n)return -1;
            at+=3;continue;
        }
        if(at+6>n||memcmp(b+at,"render",6)||(at+6<n&&!space(b[at+6])&&b[at+6]!='>'))continue;
        at+=6;
        while(at<n&&b[at]!='>'){
            while(at<n&&space(b[at]))++at;
            if(at<n&&(b[at]=='>'||b[at]=='/')){if(b[at]=='/')++at;break;}
            size_t key=at;while(at<n&&(isalnum((unsigned char)b[at])||b[at]=='_'||b[at]==':'))++at;
            size_t length=at-key;if(!length)return -1;
            while(at<n&&space(b[at]))++at;
            if(at>=n||b[at++]!='=')return -1;
            while(at<n&&space(b[at]))++at;
            if(at>=n||(b[at]!='"'&&b[at]!='\''))return -1;
            unsigned quote=b[at++];size_t value=at;
            while(at<n&&b[at]!=quote){if(b[at]=='&'||b[at]==0)return -1;++at;}
            if(at>=n)return -1;
            if(length==7&&!memcmp(b+key,"tideRef",7)){
                if(found++||at-value>=128||at==value)return -1;
                memcpy(out,b+value,at-value);out[at-value]=0;
            }
            ++at;
        }
        if(at>=n||b[at]!='>')return -1;
        return found?1:-1;
    }
    return 0;
}
static int append(nflstreet_mesh *scene,const nflstreet_mesh *model,const nflstreet_scene_instance *instance){
    if(!model->material_count||model->count>2000000-scene->count||model->material_count>65535-scene->material_count)return 0;
    float rotation[9];if(!nflstreet_environment_rotation(instance,rotation))return 0;
    size_t count=scene->count+model->count;unsigned materials=scene->material_count+model->material_count;
    void *p=realloc(scene->triangles,count*sizeof *scene->triangles);if(!p)return 0;scene->triangles=p;
    p=realloc(scene->materials,materials*sizeof *scene->materials);if(!p)return 0;scene->materials=p;
    memcpy(scene->materials+scene->material_count,model->materials,model->material_count*sizeof *model->materials);
    for(size_t i=0;i<model->count;++i){nflstreet_mesh_triangle t=model->triangles[i];
        t.material=(uint16_t)(t.material+scene->material_count);
        for(unsigned v=0;v<3;++v){float position[3];
            for(unsigned j=0;j<3;++j){position[j]=instance->position[j];
                for(unsigned k=0;k<3;++k)position[j]+=rotation[j*3+k]*t.vertex[v].position[k];
                if(!isfinite(position[j]))return 0;
            }
            memcpy(t.vertex[v].position,position,sizeof position);
            for(unsigned j=0;j<3;++j){if(position[j]<scene->minimum[j])scene->minimum[j]=position[j];if(position[j]>scene->maximum[j])scene->maximum[j]=position[j];}
        }
        scene->triangles[scene->count+i]=t;
    }
    scene->count=count;scene->material_count=materials;scene->draws+=model->draws;
    scene->ignored_color_indices+=model->ignored_color_indices;return 1;
}
int nflstreet_scene_attach_props(nflstreet_scene *s,const nflstreet_archive *env,const nflstreet_archive *defs,const nflstreet_archive *models){
    if(!s||s->layers!=6||s->prop_owned_count||!env||!defs||!models||models->count>255)return 0;
    nflstreet_archive_entry entry;uint8_t *metadata=NULL;size_t size;nflstreet_environment view;
    if(!nflstreet_archive_get(env,s->entries[0]+1,&entry)||!nflstreet_archive_decode(&entry,&metadata,&size))return 0;
    if(!nflstreet_environment_open(metadata,size,&view)){free(metadata);return 0;}
    s->prop_owned[s->prop_owned_count++]=metadata;
    nflstreet_mesh cached[256]={0};uint8_t attempted[256]={0};int ok=0;
    const unsigned arrays[3]={9,11,12};unsigned total=0;
    for(unsigned g=0;g<3;++g){if(view.count[arrays[g]]>4096-total)goto done;total+=view.count[arrays[g]];}
    for(unsigned group=0;group<3;++group)for(unsigned i=0;i<view.count[arrays[group]];++i){
        nflstreet_scene_instance instance;unsigned definition,model_index;char reference[128];uint8_t *xml=NULL;size_t xml_size;
        if(!nflstreet_environment_instance(&view,group,i,&instance))goto done;
        ++s->instances;
        if(!nflstreet_archive_find(defs,instance.name,&definition)){
            printf("Unsupported scene instance: %s definition lookup\n",instance.name);++s->unsupported_instances;continue;
        }
        if(!nflstreet_archive_get(defs,definition,&entry)||!nflstreet_archive_decode(&entry,&xml,&xml_size))goto done;
        int result=render_reference(xml,xml_size,reference);free(xml);
        if(!result){++s->nonvisual_instances;continue;}
        if(result<0||!nflstreet_archive_find(models,reference,&model_index)){
            printf("Unsupported scene instance: %s render reference\n",instance.name);++s->unsupported_instances;continue;
        }
        if(!attempted[model_index]){
            uint8_t *bytes=NULL;size_t n;attempted[model_index]=1;
            if(!nflstreet_archive_get(models,model_index,&entry)||!nflstreet_archive_decode(&entry,&bytes,&n))goto done;
            if(nflstreet_mesh_open_bind_pose(bytes,n,cached+model_index))s->prop_owned[s->prop_owned_count++]=bytes;
            else free(bytes);
        }
        if(!cached[model_index].count){printf("Unsupported scene instance: %s matrix/skin/geometry\n",instance.name);++s->unsupported_instances;continue;}
        if(!append(&s->mesh,cached+model_index,&instance))goto done;
        ++s->rendered_instances;
        printf("Placed asset-pose instance: %s group %u record %u model entry %u\n",instance.name,group,i,model_index);
    }
    ok=1;
done:for(unsigned i=0;i<256;++i)nflstreet_mesh_close(cached+i);
    return ok;
}
