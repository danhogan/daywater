#include "mesh_render.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
static void p32(uint8_t *p,uint32_t v){for(unsigned i=0;i<4;++i)p[i]=(uint8_t)(v>>(24-8*i));}
static void pf(uint8_t *p,float f){uint32_t v;memcpy(&v,&f,4);p32(p,v);}
static void matrix_checks(void){
    uint8_t b[512]={0};memcpy(b,"TMdl",4);p32(b+4,sizeof b);b[9]=3;b[11]=16;
    memcpy(b+16,"Geom",4);p32(b+20,64);p32(b+24,224);
    memcpy(b+32,"Skel",4);p32(b+36,320);p32(b+40,112);
    memcpy(b+48,"Clst",4);p32(b+52,448);p32(b+56,16);
    p32(b+64,256);p32(b+68,1);p32(b+72,168);p32(b+76,1);
    pf(b+140,1);pf(b+156,1);
    p32(b+168,128);b[173]=3;b[174]=9;b[175]=12;b[176]=1;b[177]=4;b[179]=2;
    p32(b+256,192);b[261]=1;b[272]=1;b[273]=1;
    b[192]=0x20;b[194]=1;b[195]=0xb0;
    b[197]=0x90;b[199]=3;b[203]=1;b[205]=2;
    p32(b+320,2);p32(b+328,344);p32(b+332,376);p32(b+336,400);
    pf(b+360,2);b[386]=0x40;
    for(unsigned i=0;i<6;++i)pf(b+400+i*4,1);
    p32(b+448,456);p32(b+452,2);b[457]=255;b[458]=0x10;b[460]=1;b[462]=0x10;
    nflstreet_mesh mesh={0};
    assert(!nflstreet_mesh_open(b,sizeof b,&mesh)&&!nflstreet_mesh_open_inspection(b,sizeof b,&mesh));
    assert(nflstreet_mesh_open_bind_pose(b,sizeof b,&mesh)&&mesh.count==1);
    assert(fabsf(mesh.triangles[0].vertex[1].position[0]-2)<0.000001f);
    assert(fabsf(mesh.triangles[0].vertex[1].position[1]-1)<0.000001f);
    assert(fabsf(mesh.triangles[0].vertex[2].position[0]-1)<0.000001f);
    nflstreet_mesh_close(&mesh);
    b[200]=3;assert(!nflstreet_mesh_open_bind_pose(b,sizeof b,&mesh));b[200]=0;
    b[194]=2;assert(!nflstreet_mesh_open_bind_pose(b,sizeof b,&mesh));b[194]=1;
    b[195]=0xa0;assert(!nflstreet_mesh_open_bind_pose(b,sizeof b,&mesh));b[195]=0xb0;
    b[461]=1;assert(!nflstreet_mesh_open_bind_pose(b,sizeof b,&mesh));b[461]=0;
    pf(b+400,2);assert(!nflstreet_mesh_open_bind_pose(b,sizeof b,&mesh));pf(b+400,1);
    pf(b+360,NAN);assert(!nflstreet_mesh_open_bind_pose(b,sizeof b,&mesh));pf(b+360,2);
    p32(b+328,500);assert(!nflstreet_mesh_open_bind_pose(b,sizeof b,&mesh));p32(b+328,344);
    b[192]=0x28;b[194]=3;b[195]=0x24;
    assert(!nflstreet_mesh_open_bind_pose(b,sizeof b,&mesh)); /* no position matrix loaded */
}
int main(void){
    matrix_checks();
    uint8_t b[288]={0};memcpy(b,"TMdl",4);p32(b+4,sizeof b);b[9]=1;b[11]=16;
    memcpy(b+16,"Geom",4);p32(b+20,64);p32(b+24,224);
    p32(b+64,256);p32(b+68,1);p32(b+72,168);p32(b+76,1);
    pf(b+128,0);pf(b+132,0);pf(b+136,0);
    pf(b+140,1);pf(b+144,0);pf(b+148,0);
    pf(b+152,0);pf(b+156,1);pf(b+160,0);
    p32(b+168,128);b[173]=3;b[174]=9;b[175]=12;b[176]=1;b[177]=4;b[179]=2;
    p32(b+256,192);b[261]=1;b[272]=1;
    b[192]=0x90;b[194]=3;b[195]=0;b[196]=1;b[197]=2;
    nflstreet_mesh mesh={0};assert(nflstreet_mesh_open(b,sizeof b,&mesh));
    assert(mesh.count==1&&mesh.triangles[0].vertex[1].position[0]==1&&mesh.triangles[0].vertex[2].position[1]==1);
    nflstreet_texture image={0};assert(nflstreet_mesh_render(&mesh,64,64,0.6f,&image));
    unsigned colored=0;for(unsigned i=0;i<4096;++i)if(image.bgra[i]!=0xff20252d)++colored;
    assert(colored>100);nflstreet_texture_close(&image);nflstreet_mesh_close(&mesh);
    b[197]=3;assert(!nflstreet_mesh_open(b,sizeof b,&mesh));b[197]=2;
    b[273]=1;assert(!nflstreet_mesh_open(b,sizeof b,&mesh));b[273]=0;
    b[192]=0x40;assert(!nflstreet_mesh_open(b,sizeof b,&mesh));b[192]=0x90;
    p32(b+168,0xfffffff0);assert(!nflstreet_mesh_open(b,sizeof b,&mesh));p32(b+168,128);
    pf(b+128,0.0f/0.0f);assert(!nflstreet_mesh_open(b,sizeof b,&mesh));pf(b+128,0);
    for(size_t n=0;n<sizeof b;++n)assert(!nflstreet_mesh_open(b,n,&mesh));
    /* Color relaxation is explicit, counted, and never permits file overflow. */
    p32(b+76,2);b[272]=2;p32(b+180,0);b[185]=1;b[186]=11;b[187]=4;b[188]=1;b[189]=5;b[191]=2;
    b[195]=0;b[196]=1;b[197]=1;b[198]=1;b[199]=2;b[200]=1;
    assert(!nflstreet_mesh_open(b,sizeof b,&mesh));
    assert(nflstreet_mesh_open_inspection(b,sizeof b,&mesh)&&mesh.ignored_color_indices==3);
    nflstreet_mesh_close(&mesh);
    assert(nflstreet_mesh_open_render(b,sizeof b,&mesh)&&mesh.ignored_color_indices==3);
    for(unsigned i=0;i<3;++i)assert(!memcmp(mesh.triangles[0].vertex[i].rgba,b+4,4));
    assert(mesh.triangles[0].source_offset==192);nflstreet_mesh_close(&mesh);
    b[189]=4;assert(!nflstreet_mesh_open_render(b,sizeof b,&mesh));
    assert(nflstreet_mesh_open_inspection(b,sizeof b,&mesh));nflstreet_mesh_close(&mesh);b[189]=5;
    b[196]=255;assert(!nflstreet_mesh_open_inspection(b,sizeof b,&mesh));b[196]=1;
    b[195]=3;assert(!nflstreet_mesh_open_inspection(b,sizeof b,&mesh));
    printf("model checks passed: indexed matrix loading, translated/rotated cluster pose, strict static rejection, unloaded matrix, palette bounds/cycles/scales, original coordinates, raster output, bad index, command rejection, bounds, nonfinite positions, truncation, explicit color-only exception\n");return 0;
}
