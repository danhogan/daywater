#include "player.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc,char **argv){
    if(argc!=4)return 2;
    FILE *f=fopen(argv[2],"rb");assert(f);
    assert(!fseek(f,0,SEEK_END));long n=ftell(f);assert(n>16&&n<16*1024*1024);rewind(f);
    uint8_t *bytes=malloc((size_t)n);assert(bytes);assert(fread(bytes,1,(size_t)n,f)==(size_t)n);fclose(f);
    nflstreet_player p={0};assert(nflstreet_player_open(argv[1],bytes,(size_t)n,&p));
    assert(p.count&&p.mesh.count&&p.texture_count&&nflstreet_mesh_write(&p.mesh,argv[3]));
    nflstreet_texture portrait={0};assert(nflstreet_player_portrait(&p,&portrait));nflstreet_texture_close(&portrait);
    printf("Native player checks: %u draws, %zu triangles, %u decoded texture planes\n",p.count,p.mesh.count,p.texture_count);
    nflstreet_player_close(&p);assert(!nflstreet_player_open(argv[1],bytes,(size_t)n-1,&p));
    bytes[0]='X';assert(!nflstreet_player_open(argv[1],bytes,(size_t)n,&p));
    free(bytes);puts("Player geometry/texture/portrait checks and invalid-profile rejection passed");return 0;
}
