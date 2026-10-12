#include "live_contact.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
int main(int argc,char**argv){if(argc!=5)return 2;FILE*f=fopen(argv[3],"rb");if(!f)return 2;fseek(f,0,SEEK_END);long size=ftell(f);rewind(f);if(size<=0)return 2;uint8_t*b=malloc((size_t)size);if(!b||fread(b,1,(size_t)size,f)!=(size_t)size)return 2;fclose(f);
 nflstreet_live_contact state;if(!nflstreet_live_contact_open(&state,argv[1],argv[2],b,(size_t)size))return 3;
 f=fopen(argv[4],"wb");if(!f)return 2;if(fwrite(state.actor,1,sizeof state.actor,f)!=sizeof state.actor||fwrite(state.manager,1,256,f)!=256||fwrite(state.globals,1,8,f)!=8)return 2;fclose(f);
 nflstreet_live_contact saved=state;int16_t frames[2][384];memset(frames,0x5a,sizeof frames);unsigned count=91;
 if(nflstreet_live_contact_step(&state,NAN,frames,&count)||memcmp(&state,&saved,sizeof state)||count!=91)return 4;
 for(size_t i=0;i<sizeof frames;++i)if(((uint8_t*)frames)[i]!=0x5a)return 4;
 unsigned updates=0;while(state.active&&updates<200){if(!nflstreet_live_contact_step(&state,1,frames,&count)||count!=3)return 5;++updates;}
 if(state.active||state.completions!=1||state.callbacks!=5||!state.done[0]||!state.done[1])return 6;
 printf("Native paired application exported; %u updates reach both finite ends, %u supported callbacks; invalid-step rollback exact\n",updates,state.callbacks);free(b);return 0;
}
