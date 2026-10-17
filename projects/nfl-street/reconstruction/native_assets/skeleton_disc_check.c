/* Composition check: adapter catalog/resources versus independent NSC1/archive;
   no original module routines execute in this checker. */
#include "skeleton_disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define REQUIRE(x) do{if(!(x)){fprintf(stderr,"comparison failed at line %u\n",__LINE__);return 4;}}while(0)
static uint8_t *read_file(const char *path,size_t *size){
    FILE *f=fopen(path,"rb");if(!f)return NULL;
    if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}long n=ftell(f);
    if(n<=0||fseek(f,0,SEEK_SET)){fclose(f);return NULL;}
    uint8_t *b=malloc((size_t)n);if(!b||fread(b,1,(size_t)n,f)!=(size_t)n){free(b);fclose(f);return NULL;}
    fclose(f);*size=(size_t)n;return b;
}
static unsigned random_word(unsigned *seed){*seed=*seed*1664525u+1013904223u;return *seed;}
static unsigned hash(unsigned h,const uint8_t *b,size_t n){for(size_t i=0;i<n;++i)h=(h^b[i])*16777619u;return h;}
static int equal(const nflstreet_skeleton_startup *a,const nflstreet_skeleton_startup *b){
    if(memcmp(a->base_vectors,b->base_vectors,sizeof a->base_vectors))return 0;
    for(unsigned i=0;i<6;++i){const nflstreet_skeleton_resource *x=a->resources+i,*y=b->resources+i;
        if(x->size!=y->size||x->token!=y->token||x->archive_index!=y->archive_index||memcmp(x->bytes,y->bytes,x->size))return 0;}
    return 1;
}
int main(int argc,char **argv){
    if(argc!=7)return 2;
    size_t cn=0,an=0,rn=0;uint8_t *cb=read_file(argv[2],&cn),*ab=read_file(argv[3],&an),*ram=read_file(argv[4],&rn);
    if(!cb||!ab||!ram||rn!=24u*1024u*1024u)return 3;
    unsigned context=(unsigned)strtoul(argv[5],NULL,10),edited=(unsigned)strtoul(argv[6],NULL,10);
    nflstreet_skeleton_startup_catalog expected,actual;nflstreet_archive archive;
    REQUIRE(nflstreet_skeleton_startup_catalog_open(cb,cn,&expected));
    REQUIRE(nflstreet_archive_open(ab,an,&archive));
    nflstreet_skeleton_disc_source *source=nflstreet_skeleton_disc_open(argv[1]);REQUIRE(source);
    REQUIRE(nflstreet_skeleton_disc_catalog(source,&actual));
    REQUIRE(!memcmp(actual.base_positions,expected.base_positions,384));
    REQUIRE(!memcmp(actual.hand_positions,expected.hand_positions,12));
    REQUIRE(!memcmp(actual.hair_positions,expected.hair_positions,114));
    REQUIRE(actual.scale_word==expected.scale_word);
    unsigned seed=0x46e3de85u^context,digest=2166136261u;
    /* Snapshot bytes seed test inputs only; production adapter never receives RAM. */
    for(size_t i=0;i<rn;i+=4096)seed=(seed^ram[i])*16777619u;
    uint8_t seen[65536]={0};
    for(unsigned i=0;i<=edited;++i){
        unsigned slot=i?random_word(&seed)>>16:0;while(seen[slot])slot=(slot+1)&65535u;seen[slot]=1;
        uint32_t identity=0x81000000u+slot*0x1000u;
        nflstreet_skeleton_startup a={0},b={0};
        REQUIRE(nflstreet_skeleton_disc_startup(source,identity,&a));
        REQUIRE(nflstreet_skeleton_startup_open(&archive,&expected,identity,&b));
        REQUIRE(equal(&a,&b));
        for(unsigned j=0;j<6;++j)digest=hash(digest,a.resources[j].bytes,a.resources[j].size);
        digest=hash(digest,(const uint8_t *)a.base_vectors,sizeof a.base_vectors);
        nflstreet_skeleton_startup_close(&a);nflstreet_skeleton_startup_close(&b);
    }
    nflstreet_skeleton_startup marker={0},rejected={0};
    REQUIRE(!nflstreet_skeleton_disc_startup(source,0,&rejected));REQUIRE(!memcmp(&marker,&rejected,sizeof marker));
    REQUIRE(!nflstreet_skeleton_disc_startup(source,0xfffffff0u,&rejected));REQUIRE(!memcmp(&marker,&rejected,sizeof marker));
    REQUIRE(!nflstreet_skeleton_disc_startup(NULL,0x81000000u,&rejected));
    REQUIRE(!nflstreet_skeleton_disc_catalog(NULL,&actual));REQUIRE(!nflstreet_skeleton_disc_catalog(source,NULL));
    REQUIRE(!nflstreet_skeleton_disc_startup(source,0x81000000u,NULL));
    REQUIRE(!nflstreet_skeleton_disc_open(NULL));
    /* Outputs own resources independently from source, tested after all inputs close. */
    nflstreet_skeleton_startup a={0},b={0};
    REQUIRE(nflstreet_skeleton_disc_startup(source,0x81000000u,&a));
    REQUIRE(nflstreet_skeleton_startup_open(&archive,&expected,0x81000000u,&b));
    nflstreet_skeleton_startup retained=a;
    REQUIRE(!nflstreet_skeleton_disc_startup(source,0x81000000u,&a));
    REQUIRE(!memcmp(&retained,&a,sizeof a));
    nflstreet_skeleton_disc_close(source);free(cb);free(ab);free(ram);
    REQUIRE(equal(&a,&b));nflstreet_skeleton_startup_close(&a);nflstreet_skeleton_startup_close(&b);
    nflstreet_skeleton_disc_close(NULL);
    printf("Skeleton disc: %u cases, %u complete resources, 4 catalog spans, 8 guards, independent output ownership; digest %08x.\n",edited+1,(edited+1)*6,digest);
    return 0;
}
