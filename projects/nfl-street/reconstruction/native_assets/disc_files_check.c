/* Windows integration check: independent extracted-byte reference, not a
   recompiled game-function oracle. Context RAM is read-only input, not booted. */
#include "disc_files.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct {char *reference,*game;} row;
static uint32_t hash(uint32_t h,const uint8_t *p,size_t n){for(size_t i=0;i<n;++i)h=(h^p[i])*16777619u;return h;}
static int compare(nflstreet_disc_files *d,const row *r,unsigned variant,size_t *size){
 char path[2048];int w=snprintf(path,sizeof path,"%s%s",variant==1?"/":"",r->game);
 if(w<0||(size_t)w>=sizeof path)return 0;
 if(variant==2)for(char *p=path;*p;++p)if(*p=='/')*p='\\';
 uint8_t *actual=NULL;size_t n=0;if(!nflstreet_disc_read(d,path,&actual,&n)){
  fprintf(stderr,"disc read failed: %s\n",path);return 0;}
 FILE *f=fopen(r->reference,"rb");if(!f){free(actual);return 0;}
 uint8_t block[65536];size_t at=0,got;int ok=1;
 while((got=fread(block,1,sizeof block,f))!=0){if(at>n||got>n-at||memcmp(block,actual+at,got)){ok=0;break;}at+=got;}
 if(ferror(f)||at!=n)ok=0;fclose(f);free(actual);*size=n;
 if(!ok)fprintf(stderr,"byte/length mismatch: %s -> %s\n",r->reference,path);return ok;
}
int main(int argc,char **argv){
 if(argc!=5){fprintf(stderr,"usage: disc-files-check ISO manifest RAM context-number\n");return 2;}
 FILE *ram=fopen(argv[3],"rb");if(!ram)return 2;uint8_t block[65536];size_t total=0,got;uint32_t ram_hash=2166136261u;
 while((got=fread(block,1,sizeof block,ram))!=0){ram_hash=hash(ram_hash,block,got);total+=got;}
 int bad=ferror(ram);fclose(ram);if(bad||total!=24u*1024u*1024u)return 2;
 nflstreet_disc_files *d=nflstreet_disc_open(argv[1]);if(!d){fprintf(stderr,"nod disc open failed\n");return 3;}
 FILE *manifest=fopen(argv[2],"rb");if(!manifest)return 2;
 row *rows=calloc(20000,sizeof *rows);if(!rows)return 2;size_t count=0;char line[4096];
 while(fgets(line,sizeof line,manifest)){size_t n=strlen(line);if(!n||line[n-1]!='\n'||count==20000)return 2;
  line[--n]=0;if(n&&line[n-1]=='\r')line[--n]=0;char *tab=strchr(line,'\t');if(!tab)return 2;*tab++=0;
  rows[count].reference=malloc(strlen(line)+1);rows[count].game=malloc(strlen(tab)+1);
  if(!rows[count].reference||!rows[count].game)return 2;
  strcpy(rows[count].reference,line);strcpy(rows[count].game,tab);++count;}
 bad=ferror(manifest);fclose(manifest);if(bad||!count)return 2;
 unsigned long long bytes=0;size_t anim_rows[200];unsigned anim_count=0;
 for(size_t i=0;i<count;++i){size_t n;if(!compare(d,rows+i,0,&n))return 4;bytes+=n;
  if(!strncmp(rows[i].game,"animdata/",9)){if(anim_count>=200)return 4;anim_rows[anim_count++]=i;}}
 unsigned archive_count=0;if(!anim_count||!nflstreet_disc_entry_count(d,"animdata.dat",&archive_count)||archive_count!=anim_count)return 4;
 uint32_t seed=ram_hash^(uint32_t)strtoul(argv[4],NULL,10);unsigned variants[3]={0};
 for(unsigned i=0;i<401;++i){seed=seed*1664525u+1013904223u;size_t n;unsigned v=i%3;++variants[v];
  if(!compare(d,rows+anim_rows[seed%anim_count],v,&n))return 4;}
 static const char *invalid[]={"", "../animdata.dat", "animdata/../animdata.dat", "W:/animdata.dat", "//animdata.dat",
  "animdata//00111.bin", "animdata/00111.bin/", "animdata/99999.bin", "animdata/abcde.bin", "missing-file.dat"};
 for(size_t i=0;i<sizeof invalid/sizeof *invalid;++i){uint8_t sentinel=42,*p=&sentinel;size_t n=123;
  if(nflstreet_disc_read(d,invalid[i],&p,&n)||p!=&sentinel||n!=123||sentinel!=42){fprintf(stderr,"invalid path accepted: %s\n",invalid[i]);return 4;}}
 uint8_t sentinel=42,*p=&sentinel;size_t n=123;
 if(nflstreet_disc_read(d,NULL,&p,&n)||nflstreet_disc_read(NULL,"animdata.dat",&p,&n)||
    nflstreet_disc_read(d,"animdata.dat",NULL,&n)||nflstreet_disc_read(d,"animdata.dat",&p,NULL)||p!=&sentinel||n!=123)return 4;
 for(size_t i=0;i<count;++i){free(rows[i].reference);free(rows[i].game);}free(rows);nflstreet_disc_close(d);
 printf("{\"files\":%zu,\"bytes\":%llu,\"animation_entries\":%u,\"path_variants\":[%u,%u,%u],\"invalid_paths\":10,\"ram_fnv1a\":\"%08x\"}\n",
  count,bytes,anim_count,variants[0],variants[1],variants[2],ram_hash);return 0;
}
