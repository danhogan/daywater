#include "sky.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
/* Explicit private dump lets the exporter compare actual Windows C output
   against reference arrays and topology, including every triangle color. */
int main(int argc,char **argv){
    if(argc!=3)return 2;
    FILE *f=fopen(argv[1],"rb");assert(f);unsigned char header[168];
    assert(fread(header,1,sizeof header,f)==sizeof header);fclose(f);
    assert(!memcmp(header,"NFR2",4)&&!memcmp(header+40,"NSK1",4));
    float cfg[31];memcpy(cfg,header+44,sizeof cfg);
    nflstreet_mesh_vertex vertices[41];nflstreet_mesh mesh={0};
    assert(nflstreet_sky_vertices(cfg,vertices)&&nflstreet_sky_mesh(cfg,&mesh));
    assert(mesh.count==72&&mesh.draws==2&&mesh.material_count==1);
    assert(vertices[40].position[0]==0&&vertices[40].position[1]==0&&vertices[40].position[2]==cfg[0]);
    for(unsigned i=0;i<41;++i)for(unsigned j=0;j<3;++j)assert(isfinite(vertices[i].position[j]));
    f=fopen(argv[2],"wb");assert(f);
    for(unsigned i=0;i<41;++i)assert(fwrite(vertices[i].position,4,3,f)==3);
    for(unsigned i=0;i<72;++i)for(unsigned j=0;j<3;++j){
        nflstreet_mesh_vertex *v=mesh.triangles[i].vertex+j;
        assert(mesh.triangles[i].source_offset==(i<64?0u:1u));
        assert(fwrite(v->position,4,3,f)==3&&fwrite(v->rgba,1,4,f)==4);
    }
    assert(!fclose(f));nflstreet_mesh_close(&mesh);
    cfg[0]=-1;assert(!nflstreet_sky_vertices(cfg,vertices));cfg[0]=300;
    cfg[16]=NAN;assert(!nflstreet_sky_vertices(cfg,vertices));
    puts("sky checks passed: native constructor, cap/topology counts, source selectors, finite vertices and malformed-config rejection");return 0;
}
