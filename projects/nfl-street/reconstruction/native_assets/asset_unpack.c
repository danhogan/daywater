#include "archive.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* Output directory must already exist. All proprietary output belongs on W:. */
int main(int argc,char **argv){
    if(argc!=3){fprintf(stderr,"Usage: asset-unpack archive.dat existing-output-directory\n");return 2;}
    FILE *f=fopen(argv[1],"rb");if(!f)return 1;
    if(fseek(f,0,SEEK_END)){fclose(f);return 1;}long size=ftell(f);
    if(size<16||size>512*1024*1024||fseek(f,0,SEEK_SET)){fclose(f);return 1;}
    uint8_t *b=malloc((size_t)size);if(!b){fclose(f);return 1;}
    int ok=fread(b,1,(size_t)size,f)==(size_t)size;fclose(f);
    nflstreet_archive a;if(!ok||!nflstreet_archive_open(b,(size_t)size,&a)){free(b);return 1;}
    unsigned decoded=0,failed=0;
    for(unsigned i=0;i<a.count;++i){nflstreet_archive_entry e;uint8_t *bytes=NULL;size_t n;
        if(!nflstreet_archive_get(&a,i,&e)||!nflstreet_archive_decode(&e,&bytes,&n)){
            fprintf(stderr,"Unsupported or invalid entry %u\n",i);++failed;continue;
        }
        char name[4096];int written=snprintf(name,sizeof name,"%s/%05u.bin",argv[2],i);
        if(written<0||(size_t)written>=sizeof name){free(bytes);free(b);return 1;}
        FILE *out=fopen(name,"wb");if(!out){free(bytes);free(b);return 1;}
        ok=fwrite(bytes,1,n,out)==n;if(fclose(out))ok=0;
        if(!ok){free(bytes);free(b);return 1;}
        printf("%u %u %zu %zu\n",i,e.codec,e.size,n);free(bytes);++decoded;
    }
    printf("Decoded %u, failed %u\n",decoded,failed);free(b);return failed?1:0;
}
