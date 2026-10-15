#include "disc_files.h"
#include "archive.h"
#include <nod.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
struct nflstreet_disc_files {
 struct NodHandle *disc,*partition;
 uint8_t *cached;
 char cached_path[1024];
 nflstreet_archive archive;
};
static int normalize(const char *path,char out[1024]){
 if(!path)return 0;if(*path=='/'||*path=='\\')++path;
 size_t n=strlen(path);if(!n||n>=1024)return 0;
 for(size_t i=0;i<=n;++i){unsigned char c=(unsigned char)path[i];
  if(c==':'||(c<32&&c))return 0;out[i]=c=='\\'?'/':(char)c;}
 for(char *p=out;*p;){char *end=strchr(p,'/');size_t k=end?(size_t)(end-p):strlen(p);
  if(!k||(k==1&&p[0]=='.')||(k==2&&p[0]=='.'&&p[1]=='.'))return 0;
  if(!end)break;p=end+1;if(!*p)return 0;}
 return 1;
}
void nflstreet_disc_close(nflstreet_disc_files *d){
 if(!d)return;free(d->cached);nod_free(d->partition);nod_free(d->disc);free(d);
}
nflstreet_disc_files *nflstreet_disc_open(const char *iso){
 if(!iso||!*iso)return NULL;nflstreet_disc_files *d=calloc(1,sizeof *d);if(!d)return NULL;
 if(nod_disc_open(iso,NULL,&d->disc)!=NOD_RESULT_OK||
    nod_disc_open_partition_kind(d->disc,NOD_PARTITION_KIND_DATA,NULL,&d->partition)!=NOD_RESULT_OK){
  nflstreet_disc_close(d);return NULL;}return d;
}
static int fst_read(nflstreet_disc_files *d,const char *path,uint8_t **bytes,size_t *size){
 enum NodNodeKind kind=NOD_NODE_KIND_DIRECTORY;uint32_t length=0;
 uint32_t index=nod_partition_find_file(d->partition,path,&kind,&length);
 if(index==NOD_FST_STOP||kind!=NOD_NODE_KIND_FILE)return 0;
 struct NodHandle *f=NULL;if(nod_partition_open_file(d->partition,index,&f)!=NOD_RESULT_OK)return 0;
 uint8_t *b=malloc(length?length:1);if(!b){nod_free(f);return 0;}
 size_t at=0;while(at<length){int64_t got=nod_read(f,b+at,length-at);
  if(got<=0||(uint64_t)got>length-at){free(b);nod_free(f);return 0;}at+=(size_t)got;}
 nod_free(f);*bytes=b;*size=length;return 1;
}
static int cache_archive(nflstreet_disc_files *d,const char *archive_path){
 if(strcmp(archive_path,d->cached_path)){
  uint8_t *b=NULL;size_t n=0;nflstreet_archive a;
  if(!fst_read(d,archive_path,&b,&n))return 0;
  if(!nflstreet_archive_open(b,n,&a)){free(b);return 0;}
  free(d->cached);d->cached=b;d->archive=a;strcpy(d->cached_path,archive_path);
 }
 return 1;
}
int nflstreet_disc_entry_count(nflstreet_disc_files *d,const char *path,unsigned *count){
 char normalized[1024];if(!d||!count||!normalize(path,normalized)||!cache_archive(d,normalized))return 0;
 *count=d->archive.count;return 1;
}
int nflstreet_disc_read(nflstreet_disc_files *d,const char *path,uint8_t **bytes,size_t *size){
 char normalized[1024];if(!d||!bytes||!size||!normalize(path,normalized))return 0;
 if(fst_read(d,normalized,bytes,size))return 1;
 char *name=strrchr(normalized,'/');if(!name||strlen(name+1)!=9||strcmp(name+6,".bin"))return 0;
 unsigned index=0;for(unsigned i=1;i<=5;++i){if(name[i]<'0'||name[i]>'9')return 0;index=index*10+(unsigned)(name[i]-'0');}
 *name=0;char archive_path[1024];int written=snprintf(archive_path,sizeof archive_path,"%s.dat",normalized);
 if(written<0||(size_t)written>=sizeof archive_path||!cache_archive(d,archive_path))return 0;
 nflstreet_archive_entry e;if(!nflstreet_archive_get(&d->archive,index,&e))return 0;
 return nflstreet_archive_decode(&e,bytes,size);
}
