#include "texture.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* Private list of extracted MMAP files, one host path per line. */
int main(int argc,char **argv){
    if(argc!=2)return 2;
    FILE *list=fopen(argv[1],"r");if(!list)return 1;
    char path[4096];unsigned passed=0,failed=0,other=0;unsigned long long pixels=0;
    while(fgets(path,sizeof path,list)){
        path[strcspn(path,"\r\n")]=0;if(!path[0])continue;
        FILE *f=fopen(path,"rb");if(!f){++failed;continue;}
        char magic[4];
        if(fread(magic,1,4,f)!=4||memcmp(magic,"MMAP",4)){fclose(f);++other;continue;}
        if(fseek(f,0,SEEK_END)){fclose(f);++failed;continue;}long size=ftell(f);
        if(size<32||size>128*1024*1024||fseek(f,0,SEEK_SET)){fclose(f);++failed;continue;}
        uint8_t *b=malloc((size_t)size);if(!b){fclose(f);++failed;continue;}
        int ok=fread(b,1,(size_t)size,f)==(size_t)size;fclose(f);nflstreet_texture t={0};
        if(ok&&nflstreet_texture_open(b,(size_t)size,&t)){++passed;pixels+=(size_t)t.width*t.height;}
        else {fprintf(stderr,"Unsupported/invalid texture: %s\n",path);++failed;}
        nflstreet_texture_close(&t);free(b);
    }
    fclose(list);printf("First texture planes decoded %u, failed %u, other entries %u, pixels %llu\n",passed,failed,other,pixels);return failed?1:0;
}
