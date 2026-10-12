/*
 * Inspection preview for a selected PLAY head resource. The selected archive
 * entry must come from the separately verified original 801A8088 lookup.
 * This uses the TMdl stored cluster pose and a flat inspection renderer; it
 * does not perform the game's runtime pose, skin, material, or actor setup.
 */
#include "team_player_selected_head.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int read_file(const char *path,uint8_t **out,size_t *size){
 FILE *f=fopen(path,"rb");if(!f)return 0;
 if(fseek(f,0,SEEK_END)){fclose(f);return 0;}
 long n=ftell(f);if(n<16||n>128*1024*1024||fseek(f,0,SEEK_SET)){fclose(f);return 0;}
 uint8_t *b=(uint8_t*)malloc((size_t)n);
 if(!b||fread(b,1,(size_t)n,f)!=(size_t)n){free(b);fclose(f);return 0;}
 fclose(f);*out=b;*size=(size_t)n;return 1;
}

int main(int argc,char **argv){
 if(argc<4||argc>5){fprintf(stderr,"usage: selected-head-preview <plaface.dat> <verified-entry-index> <private-output.bmp> [private-mesh.bin]\n");return 2;}
 errno=0;char *end=NULL;unsigned long index=strtoul(argv[2],&end,10);
 if(errno||!argv[2][0]||!end||*end||index>65535)return 2;
 uint8_t *archive_bytes=NULL;size_t archive_size=0;
 nflstreet_archive archive;nflstreet_team_player_head head={0};nflstreet_texture image={0};
 int status=1;
 if(!read_file(argv[1],&archive_bytes,&archive_size)||
    !nflstreet_archive_open(archive_bytes,archive_size,&archive)||
    !nflstreet_team_player_head_open(&archive,(unsigned)index,&head))goto done;
 if(argc==5&&!nflstreet_mesh_write(&head.mesh,argv[4]))goto done;
 if(!nflstreet_team_player_head_render(&head,768,640,0.6f,&image)||
    !nflstreet_texture_bmp(&image,argv[3]))goto done;
 printf("Selected head resource inspection: archive entry %lu, Info=%s, draws=%u, triangles=%zu, materials=%u; stored cluster pose and flat inspection renderer.\n",
        index,head.info_name,head.mesh.draws,head.mesh.count,head.mesh.material_count);
 status=0;
done:
 if(status)fprintf(stderr,"Selected-head inspection unsupported for this TMdl/render branch.\n");
 nflstreet_texture_close(&image);nflstreet_team_player_head_close(&head);free(archive_bytes);
 return status;
}
