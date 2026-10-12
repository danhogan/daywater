#include "archive.h"
#include "field_render.h"
#include "sky.h"
#ifdef NFLSTREET_PLAYER_PREVIEW
#include "player.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint32_t le32(const uint8_t *p){return p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static uint8_t *read_file(const char *path,size_t *size){
    FILE *f=fopen(path,"rb");if(!f)return NULL;
    if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}long n=ftell(f);
    if(n<16||n>128*1024*1024||fseek(f,0,SEEK_SET)){fclose(f);return NULL;}
    uint8_t *b=malloc((size_t)n);if(!b){fclose(f);return NULL;}
    int ok=fread(b,1,(size_t)n,f)==(size_t)n;fclose(f);if(!ok){free(b);return NULL;}
    *size=(size_t)n;return b;
}
int main(int argc,char **argv){
    int player_mode=0;
#ifdef NFLSTREET_PLAYER_PREVIEW
    player_mode=(argc==7||argc==8)&&!strcmp(argv[4],"--player");
#endif
    if(argc!=4&&!player_mode){fprintf(stderr,"Usage: field-startup envrnmt.dat private-profile.bin output.bmp [--player unpacked-assets player-profile.bin]\n");return 2;}
    size_t archive_size=0,profile_size=0;uint8_t *archive=read_file(argv[1],&archive_size),*profile=read_file(argv[2],&profile_size),*owned[6]={0};
    nflstreet_mesh meshes[6]={0},sky={0};nflstreet_texture image={0};nflstreet_field_draw *draws=NULL;int status=1;
#ifdef NFLSTREET_PLAYER_PREVIEW
    uint8_t *player_profile=NULL;nflstreet_player player={0};
#endif
    if(!archive||!profile||profile_size<40||(memcmp(profile,"NFR1",4)&&memcmp(profile,"NFR2",4)))goto done;
    unsigned count=le32(profile+4),width=le32(profile+8),height=le32(profile+12);
    unsigned header=40;int has_sky=!memcmp(profile,"NFR2",4);
    if(has_sky){float cfg[31];header+=128;
        if(profile_size<header||memcmp(profile+40,"NSK1",4))goto done;
        for(unsigned i=0;i<31;++i){uint32_t bits=le32(profile+44+i*4);memcpy(cfg+i,&bits,4);}
        if(!nflstreet_sky_mesh(cfg,&sky))goto done;
        printf("Original procedural sky: 41 vertices, 32 quads, 8 cap triangles\n");
    }
    if(!count||count>20000||profile_size!=header+(size_t)count*208)goto done;
    nflstreet_archive a;if(!nflstreet_archive_open(archive,archive_size,&a))goto done;
    for(unsigned i=0;i<6;++i){nflstreet_archive_entry entry;size_t n;
        if(!nflstreet_archive_get(&a,le32(profile+16+i*4),&entry)||!nflstreet_archive_decode(&entry,owned+i,&n)||
           !nflstreet_mesh_open_render(owned[i],n,meshes+i))goto done;
        printf("Original field layer %u: %s, %zu native triangles\n",i,meshes[i].name,meshes[i].count);
    }
    unsigned total=count;
#ifdef NFLSTREET_PLAYER_PREVIEW
    if(player_mode){size_t n=0;player_profile=read_file(argv[6],&n);
        if(!player_profile||!nflstreet_player_open(argv[5],player_profile,n,&player))goto done;
        total+=player.count;
        printf("Independent player: %u checked draw commands, %zu posed triangles, %u selected native texture planes\n",player.count,player.mesh.count,player.texture_count);
    }
#endif
    draws=calloc(total,sizeof *draws);if(!draws)goto done;
    for(unsigned i=0;i<count;++i){const uint8_t *r=profile+header+(size_t)i*208;nflstreet_field_draw *d=draws+i;
        for(unsigned j=0;j<16;++j)d->state[j]=le32(r+j*4);
        float *arrays[]={d->camera,d->projection,d->viewport,d->texmatrix};unsigned sizes[]={12,6,6,12},at=16;
        for(unsigned j=0;j<4;++j)for(unsigned k=0;k<sizes[j];++k){uint32_t bits=le32(r+at++*4);memcpy(arrays[j]+k,&bits,4);}
    }
    nflstreet_field_stats stats;
#ifdef NFLSTREET_PLAYER_PREVIEW
    if(player_mode){memcpy(draws+count,player.draws,player.count*sizeof *draws);
        if(!nflstreet_field_render_player(meshes,has_sky?&sky:NULL,&player.mesh,draws,total,width,height,&image,&stats))goto done;
    }else
#endif
    if(!nflstreet_field_render_sky(meshes,has_sky?&sky:NULL,draws,count,width,height,&image,&stats))goto done;
    if(!nflstreet_texture_bmp(&image,argv[3]))goto done;
    printf("Independent field render: %u commands, %u selected triangles, %u changed clip polygons, %zu shaded samples\n",stats.commands,stats.triangles,stats.clipped,stats.shaded);
    printf("Captured camera/state; native asset/color/texture loading. No emulator, ISO or saved RAM. Base-mip filtering; complete original pixels and camera updates remain open.\n");status=0;
#ifdef NFLSTREET_PLAYER_PREVIEW
    if(player_mode&&argc==8){nflstreet_texture portrait={0};
        int ok=nflstreet_player_portrait(&player,&portrait)&&nflstreet_texture_bmp(&portrait,argv[7]);
        nflstreet_texture_close(&portrait);if(!ok)status=1;
    }
#endif
done:if(status)fprintf(stderr,"Invalid/unsupported native field assets, profile or rendering state\n");
    for(unsigned i=0;i<6;++i){nflstreet_mesh_close(meshes+i);free(owned[i]);}
#ifdef NFLSTREET_PLAYER_PREVIEW
    nflstreet_player_close(&player);free(player_profile);
#endif
    nflstreet_mesh_close(&sky);nflstreet_texture_close(&image);free(draws);free(profile);free(archive);return status;
}
