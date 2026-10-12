#include "scene.h"
#include "mesh_render.h"
#include "environment.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
static void p32(uint8_t *p,uint32_t v){for(unsigned i=0;i<4;++i)p[i]=(uint8_t)(v>>(24-8*i));}
static void pf(uint8_t *p,float f){uint32_t v;memcpy(&v,&f,4);p32(p,v);}
static void model(uint8_t *b,unsigned index){
    memset(b,0,512);memcpy(b,"TMdl",4);p32(b+4,512);b[9]=3;b[11]=16;
    memcpy(b+16,"Geom",4);p32(b+20,64);p32(b+24,224);
    memcpy(b+32,"Info",4);p32(b+36,320);p32(b+40,64);
    memcpy(b+48,"Matl",4);p32(b+52,384);p32(b+56,112);
    if(!index)snprintf((char *)b+320,64,"Test_rField.ea3");
    else snprintf((char *)b+320,64,"Test_FieldPass%u.ea3",index);
    p32(b+384,1);p32(b+388,392);p32(b+396,480);memcpy(b+480,"Flat",5);
    p32(b+64,256);p32(b+68,1);p32(b+72,168);p32(b+76,1);
    pf(b+128,(float)index*3);pf(b+140,(float)index*3+1);pf(b+152,(float)index*3);pf(b+156,1);
    p32(b+168,128);b[173]=3;b[174]=9;b[175]=12;b[176]=1;b[177]=4;b[179]=2;
    p32(b+256,192);b[261]=1;b[272]=1;
    b[192]=0x90;b[194]=3;b[196]=1;b[197]=2;
}
static void environment_checks(void){
    uint8_t b[216]={0};p32(b,105);p32(b+0x28,1);p32(b+0x58,1);
    memcpy(b+156,"Fixture",8);pf(b+180,12);pf(b+192,90);
    nflstreet_environment e;nflstreet_scene_instance v;float m[9];
    assert(nflstreet_environment_open(b,sizeof b,&e));
    assert(e.offset[3]==156&&e.offset[9]==180);
    assert(nflstreet_environment_instance(&e,0,0,&v)&&!strcmp(v.name,"Fixture")&&v.position[0]==12);
    assert(nflstreet_environment_rotation(&v,m));
    assert(fabsf(m[4])<0.00001f&&fabsf(m[5]+1)<0.00001f&&fabsf(m[7]-1)<0.00001f);
    memset(v.angles,0,sizeof v.angles);assert(nflstreet_environment_rotation(&v,m));
    for(unsigned i=0;i<9;++i)assert(fabsf(m[i]-(i%4==0?1:0))<0.000001f);
    assert(!nflstreet_environment_instance(&e,3,0,&v)&&!nflstreet_environment_instance(&e,0,1,&v));
    b[205]=1;assert(!nflstreet_environment_instance(&e,0,0,&v));b[205]=0;
    pf(b+180,NAN);assert(!nflstreet_environment_instance(&e,0,0,&v));pf(b+180,12);
    assert(!nflstreet_environment_open(b,sizeof b-1,&e));
    memset(b+156,'x',24);assert(!nflstreet_environment_instance(&e,0,0,&v));
}
static void hash_checks(void){
    uint8_t b[64]={0};memcpy(b,"TERF",4);p32(b+4,16);b[15]=1;
    memcpy(b+16,"HSH1",4);p32(b+20,28);p32(b+24,1);p32(b+28,4);p32(b+32,4);p32(b+36,10);
    p32(b+40,((unsigned)'A'*10+'B')*10+'C');
    memcpy(b+44,"DIR1",4);p32(b+48,16);p32(b+52,8);
    memcpy(b+60,"DATA",4); /* Use an empty DATA payload and eight-byte chunk. */
    uint8_t full[68];memcpy(full,b,64);p32(full+64,8);
    nflstreet_archive a;unsigned index=99;
    assert(nflstreet_archive_open(full,sizeof full,&a));
    assert(nflstreet_archive_find(&a,"aBc",&index)&&index==0);
    assert(!nflstreet_archive_find(&a,"ABD",&index));
    p32(full+24,2);assert(!nflstreet_archive_find(&a,"ABC",&index));p32(full+24,1);
    p32(full+20,24);assert(!nflstreet_archive_find(&a,"ABC",&index));
}
int main(void){
    environment_checks();hash_checks();
    uint8_t bytes[16+56+8+6*512]={0};const unsigned data=72;
    memcpy(bytes,"TERF",4);p32(bytes+4,16);bytes[15]=6;
    memcpy(bytes+16,"DIR1",4);p32(bytes+20,56);memcpy(bytes+data,"DATA",4);p32(bytes+data+4,8+6*512);
    /* Deliberately reverse archive order: selection must use original names. */
    for(unsigned i=0;i<6;++i){p32(bytes+24+i*8,8+i*512);p32(bytes+28+i*8,512);model(bytes+data+8+i*512,5-i);}
    nflstreet_archive archive;assert(nflstreet_archive_open(bytes,sizeof bytes,&archive));nflstreet_scene scene={0};
    assert(nflstreet_scene_open(&archive,"Test",&scene));
    assert(scene.layers==6&&scene.mesh.count==6&&scene.mesh.material_count==6&&scene.entries[0]==5);
    assert(scene.mesh.minimum[0]==0&&scene.mesh.maximum[0]==16);
    for(unsigned i=0;i<6;++i)assert(scene.mesh.triangles[i].material==i&&scene.mesh.triangles[i].vertex[0].position[0]==(float)i*3);
    nflstreet_texture image={0};assert(nflstreet_mesh_render(&scene.mesh,64,64,0.6f,&image));nflstreet_texture_close(&image);
    nflstreet_scene_close(&scene);assert(!scene.mesh.triangles&&!scene.owned[0]);
    assert(!nflstreet_scene_open(&archive,"Missing",&scene));
    assert(!nflstreet_scene_open(&archive,"../Test",&scene));
    /* Duplicate a required layer; do not publish an incomplete scene. */
    model(bytes+data+8,4);assert(!nflstreet_scene_open(&archive,"Test",&scene));model(bytes+data+8,5);
    bytes[data+8+197]=3;assert(!nflstreet_scene_open(&archive,"Test",&scene));
    printf("scene checks passed: named selection, environment bounds/name/pose validation, original hash lookup, archive reorder, shared coordinates, material remapping, rendering, owned lifetime, missing/duplicate layers, malformed geometry\n");return 0;
}
