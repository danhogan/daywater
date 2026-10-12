#include "team_player_selected_head.h"
#include "../native_assets/mesh_render.h"
#include <stdlib.h>
#include <string.h>

int nflstreet_team_player_head_open(const nflstreet_archive *archive,
 unsigned index,nflstreet_team_player_head *out){
 if(!archive||!out)return 0;
 nflstreet_archive_entry entry;
 uint8_t *decoded=NULL;size_t decoded_size=0;
 if(!nflstreet_archive_get(archive,index,&entry)||
    !nflstreet_archive_decode(&entry,&decoded,&decoded_size))return 0;
 nflstreet_team_player_head next={0};next.archive_index=index;
 next.owned_model_bytes=decoded;next.model_size=decoded_size;
 if(!nflstreet_mesh_open_bind_pose(decoded,decoded_size,&next.mesh)||
    !next.mesh.name[0]){
  nflstreet_mesh_close(&next.mesh);free(decoded);return 0;
 }
 memcpy(next.info_name,next.mesh.name,sizeof next.info_name);
 *out=next;return 1;
}

void nflstreet_team_player_head_close(nflstreet_team_player_head *head){
 if(!head)return;
 nflstreet_mesh_close(&head->mesh);free(head->owned_model_bytes);
 memset(head,0,sizeof *head);
}

int nflstreet_team_player_head_render(const nflstreet_team_player_head *head,
 unsigned width,unsigned height,float yaw,nflstreet_texture *out){
 if(!head||!head->owned_model_bytes||!head->mesh.triangles||!out)return 0;
 return nflstreet_mesh_render(&head->mesh,width,height,yaw,out);
}
