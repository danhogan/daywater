#include "model.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc,char **argv){
    if(argc!=2)return 2;
    FILE *list=fopen(argv[1],"r");if(!list)return 1;
    unsigned accepted=0,rejected=0,other=0;size_t triangles=0;char path[4096];
    while(fgets(path,sizeof path,list)){
        path[strcspn(path,"\r\n")]=0;if(!path[0])continue;
        FILE *f=fopen(path,"rb");if(!f){fclose(list);return 1;}char tag[4];
        if(fread(tag,1,4,f)!=4||memcmp(tag,"TMdl",4)){fclose(f);++other;continue;}
        if(fseek(f,0,SEEK_END)){fclose(f);fclose(list);return 1;}long size=ftell(f);
        if(size<16||size>128*1024*1024||fseek(f,0,SEEK_SET)){fclose(f);fclose(list);return 1;}
        uint8_t *b=malloc((size_t)size);if(!b){fclose(f);fclose(list);return 1;}
        int ok=fread(b,1,(size_t)size,f)==(size_t)size;fclose(f);if(!ok){free(b);fclose(list);return 1;}
        nflstreet_mesh mesh={0};if(nflstreet_mesh_open(b,(size_t)size,&mesh)){
            printf("ACCEPT %s %s %u %zu\n",path,mesh.name,mesh.draws,mesh.count);++accepted;triangles+=mesh.count;
        }else {printf("UNSUPPORTED %s\n",path);++rejected;}
        nflstreet_mesh_close(&mesh);free(b);
    }
    fclose(list);printf("Static models accepted %u, unsupported %u, nonmodels %u, triangles %zu\n",accepted,rejected,other,triangles);return 0;
}
