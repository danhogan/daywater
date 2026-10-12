#include "field_render.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
#include <string.h>
static nflstreet_field_draw draw(unsigned layer,float distance,unsigned depth,int early){
    nflstreet_field_draw d={0};d.state[0]=layer;d.state[1]=1;d.state[2]=depth;d.state[3]=0x54b9;
    d.state[4]=0x18fffa;d.state[5]=0x18ffd0;d.state[6]=0x240020;
    d.state[9]=0xffffffff;d.state[10]=d.state[11]=0x403;d.state[12]=0x282;
    d.state[13]=early?64:0;d.state[15]=UINT32_MAX;
    d.camera[0]=d.camera[5]=d.camera[10]=1;d.camera[11]=-distance;
    d.projection[0]=d.projection[2]=1;d.projection[5]=-1;
    d.viewport[0]=16;d.viewport[1]=-16;d.viewport[2]=100;d.viewport[3]=d.viewport[4]=358;d.viewport[5]=100;
    d.texmatrix[0]=d.texmatrix[5]=d.texmatrix[10]=1;return d;
}
static void triangle(nflstreet_mesh_triangle *t,unsigned r,unsigned g,unsigned b,unsigned a){
    memset(t,0,sizeof *t);t->source_offset=1;
    const float positions[3][3]={{-1,-1,0},{1,-1,0},{0,1,0}};
    for(unsigned i=0;i<3;++i){memcpy(t->vertex[i].position,positions[i],sizeof positions[i]);
        t->vertex[i].rgba[0]=(uint8_t)r;t->vertex[i].rgba[1]=(uint8_t)g;t->vertex[i].rgba[2]=(uint8_t)b;t->vertex[i].rgba[3]=(uint8_t)a;}
}
static uint32_t center(nflstreet_mesh *meshes,nflstreet_field_draw *draws,unsigned n){
    nflstreet_texture image={0};nflstreet_field_stats stats;
    assert(nflstreet_field_render(meshes,draws,n,32,32,&image,&stats)&&stats.commands==n);
    uint32_t pixel=image.bgra[16*32+16];nflstreet_texture_close(&image);return pixel;
}
int main(void){
    nflstreet_mesh meshes[6]={0};nflstreet_mesh_triangle triangles[3];
    for(unsigned i=0;i<3;++i){meshes[i].triangles=triangles+i;meshes[i].count=1;meshes[i].material_count=1;}
    triangle(triangles,100,0,0,255);triangle(triangles+1,0,100,0,255);triangle(triangles+2,0,0,100,255);
    nflstreet_field_draw draws[3]={draw(0,2,23,0),draw(1,1,3,0),draw(2,1.5f,23,0)};
    assert((center(meshes,draws,3)&0xffffff)==0x0000c8); /* overlay didn't write depth */
    draws[1].state[2]=23;assert((center(meshes,draws,3)&0xffffff)==0x00c800);
    triangles[1].vertex[0].rgba[3]=triangles[1].vertex[1].rgba[3]=triangles[1].vertex[2].rgba[3]=0;
    draws[1].state[13]=64;assert((center(meshes,draws,3)&0xffffff)==0xc80000); /* early depth survives alpha reject */
    draws[1].state[13]=0;assert((center(meshes,draws,3)&0xffffff)==0x0000c8);
    draws[0].state[9]=0x7f7f7f82;assert((center(meshes,draws,1)&0xffffff)==0x620000); /* floor(100*127/256)*2 */
    uint8_t texture[128]={0};memcpy(texture,"MMAP",4);texture[5]=2;texture[15]=1;texture[27]=40;
    texture[41]=texture[43]=4;texture[45]=6;texture[51]=64;texture[55]=64;
    for(unsigned i=0;i<16;++i){texture[64+i*2]=255;texture[65+i*2]=texture[96+i*2]=texture[97+i*2]=128;}
    meshes[0].texture_bytes=texture;meshes[0].texture_size=sizeof texture;
    draws[0].state[15]=0;draws[0].state[4]=0x18f8af;draws[0].state[5]=0x18f2f0;draws[0].state[7]=0x1d5;
    assert((center(meshes,draws,1)&0xffffff)==0x310000); /* sampled half-intensity texture modulates lit color */
    nflstreet_texture image={0};nflstreet_field_stats stats;draws[0].state[10]=0x407;
    assert(!nflstreet_field_render(meshes,draws,1,32,32,&image,&stats));
    draws[0]=draw(0,0.5f,23,0);assert((center(meshes,draws,1)&0xffffff)==0x20252d); /* in front of near plane */
    nflstreet_mesh sky=meshes[0];nflstreet_mesh_triangle sky_triangle;
    triangle(&sky_triangle,100,125,200,0);sky.triangles=&sky_triangle;
    for(unsigned i=0;i<3;++i)sky_triangle.vertex[i].position[2]=-2;
    nflstreet_field_draw sky_draw=draw(6,0,6,0);
    sky_draw.state[3]=0x54b8;sky_draw.state[4]=0x8fffa;sky_draw.state[5]=0x8ffd0;
    sky_draw.state[8]=0xffffffff;sky_draw.state[9]=0;sky_draw.state[10]=0x705;sky_draw.state[11]=0x404;
    sky_draw.state[6]=0x3f0000;
    assert(!nflstreet_field_render(meshes,&sky_draw,1,32,32,&image,&stats)); /* requires explicit sky mesh */
    assert(nflstreet_field_render_sky(meshes,&sky,&sky_draw,1,32,32,&image,&stats));
    assert(image.bgra[16*32+16]==0xff647dc8); /* unlit scale one, register alpha, no blend */
    nflstreet_texture_close(&image);sky_draw.camera[3]=1;
    assert(!nflstreet_field_render_sky(meshes,&sky,&sky_draw,1,32,32,&image,&stats)); /* rotation-only */
    sky_draw.camera[3]=0;sky_draw.state[2]=23;
    assert(!nflstreet_field_render_sky(meshes,&sky,&sky_draw,1,32,32,&image,&stats)); /* sky cannot write depth */
    /* A cached depth buffer must hide actors behind geometry, allow actors
       in front, and remain unchanged after each overlay. */
    meshes[0].texture_bytes=NULL;meshes[0].texture_size=0;
    triangle(triangles,100,0,0,255);nflstreet_field_draw background=draw(0,2,23,0);
    nflstreet_field_surface cached={0};assert(nflstreet_field_prepare(meshes,NULL,&background,1,32,32,&cached,&stats));
    uint32_t white=0xffffffff;nflstreet_texture tex={1,1,&white};
    nflstreet_mesh actor=meshes[1];triangle(triangles+1,0,100,0,255);
    nflstreet_field_draw actor_draw=draw(7,3,23,0);actor_draw.actor_texture=&tex;
    actor_draw.state[4]=0x18f8af;actor_draw.state[5]=0x18f2f0;actor_draw.state[10]=actor_draw.state[11]=0x51e;actor_draw.state[12]=0x280;
    assert(nflstreet_field_overlay(&cached,&actor,&actor_draw,1,&image,&stats));
    assert(image.bgra[16*32+16]==cached.image.bgra[16*32+16]);nflstreet_texture_close(&image);
    actor_draw.camera[11]=-1.5f;
    assert(nflstreet_field_overlay(&cached,&actor,&actor_draw,1,&image,&stats));
    assert(image.bgra[16*32+16]!=cached.image.bgra[16*32+16]);nflstreet_texture_close(&image);
    actor_draw.camera[11]=-3;
    assert(nflstreet_field_overlay(&cached,&actor,&actor_draw,1,&image,&stats));
    assert(image.bgra[16*32+16]==cached.image.bgra[16*32+16]);nflstreet_texture_close(&image);
    nflstreet_field_surface_close(&cached);
    puts("field checks passed: projection/clipping, draw order, depth-write off/on, early and late alpha rejection, ambient integer lighting, sampled texture modulation, unsupported state rejection");return 0;
}
