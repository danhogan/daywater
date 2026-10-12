/* Owned runtime guards and stop/restart lifecycle, without original module. */
#include "live_movement.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc,char **argv){if(argc!=2)return 2;FILE *f=fopen(argv[1],"rb");if(!f)return 2;fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);if(n<152)return 2;unsigned char *b=malloc((size_t)n);if(!b||fread(b,1,(size_t)n,f)!=(size_t)n)return 2;fclose(f);
 nflstreet_live_movement m,before;unsigned updates;if(!nflstreet_live_movement_check(b,(size_t)n,&updates)||!nflstreet_live_movement_open(b,(size_t)n,&m))return 3;
 unsigned stop=0;for(unsigned i=0;i<48;++i){nflstreet_live_motion motion;int result=nflstreet_live_movement_step(&m,0,0,&motion);if(result<0)return 4;if(!stop&&result==0)stop=i+1;}
 if(!stop)return 5;float x,y;nflstreet_live_motion motion;nflstreet_live_movement_observe(&m,&motion);x=motion.x;y=motion.y;
 for(unsigned i=0;i<12;++i)if(nflstreet_live_movement_step(&m,0,0,&motion)!=0||motion.x!=x||motion.y!=y)return 6;
 before=m;if(nflstreet_live_movement_step(&m,2,1,&motion)!=-1||memcmp(&m,&before,sizeof m))return 7;
 m.runtime.state[2]=0x3f800000;before=m;if(nflstreet_live_movement_step(&m,0,0,&motion)!=-1||memcmp(&m,&before,sizeof m))return 8;m.runtime.state[2]=0;
 if(nflstreet_live_movement_step(&m,1,0,&motion)!=1||motion.speed_per_update<=0)return 9;
 for(unsigned i=0;i<6;++i)if(nflstreet_live_movement_step(&m,1,0,&motion)!=1)return 10;
 for(int dy=-1;dy<=1;++dy)for(int dx=-1;dx<=1;++dx){if(!dx&&!dy)continue;
  nflstreet_live_movement trial=m;for(unsigned i=0;i<64;++i)if(nflstreet_live_movement_step(&trial,0,0,&motion)<0)return 13;
  if(trial.runtime.state[7])return 14;
  for(unsigned i=0;i<8;++i)if(nflstreet_live_movement_step(&trial,dx,dy,&motion)!=1){printf("Restart failed direction %d %d at %u\n",dx,dy,i);return 15;}
  for(unsigned i=0;i<64;++i)if(nflstreet_live_movement_step(&trial,0,0,&motion)<0)return 16;
  if(trial.runtime.state[7]||trial.runtime.state[10]||trial.runtime.state[11])return 17;
 }
 puts("Eight directions: starts, eight updates, releases and held stops pass");
 nflstreet_live_movement_reset(&m);if(memcmp(&m.seed,&m.runtime,sizeof m.seed))return 11;
 before=m;if(nflstreet_live_movement_open(b,(size_t)n-1,&m)||memcmp(&m,&before,sizeof m))return 12;
 printf("Native lifecycle passed: original recorded checkpoints=%u; stop at %u, held position, same-direction restart, unsupported/airborne rollback, reset, truncated fixture rejection\n",updates,stop);
 free(b);return 0;}
