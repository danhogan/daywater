#include "animation_callbacks.h"
#include "asset_loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int readn(FILE *f,void *p,size_t n){return fread(p,1,n,f)==n;}
static int word(FILE *f,uint32_t *v){uint8_t b[4];if(!readn(f,b,4))return 0;
    *v=(uint32_t)b[0]<<24|(uint32_t)b[1]<<16|(uint32_t)b[2]<<8|b[3];return 1;}
static int bytes(FILE *f,const uint8_t **data,size_t *size,size_t limit){uint32_t n;
    if(!word(f,&n)||n>limit)return 0;
    uint8_t *p=malloc(n?n:1);if(!p)return 0;
    if(!readn(f,p,n)){free(p);return 0;}*data=p;*size=n;return 1;}
static int blobs(FILE *f,const nflstreet_animation_blob **out,size_t *size,size_t limit,size_t max_size){uint32_t count;
    if(!word(f,&count)||count>limit)return 0;
    nflstreet_animation_blob *b=calloc(count?count:1,sizeof *b);if(!b)return 0;
    *out=b;*size=count;
    for(unsigned i=0;i<count;++i)if(!word(f,&b[i].identity)||!bytes(f,&b[i].data,&b[i].size,max_size))return 0;
    return 1;}
static int sources(FILE *f,nflstreet_animation_sources *s){uint32_t value;size_t n;
    memset(s,0,sizeof *s);
    if(!word(f,&value)||value>255)return 0;
    s->bank_key=(uint16_t)value;
    if(!bytes(f,&s->bank,&n,32)||n!=32||!bytes(f,&s->groups,&s->groups_size,8196)||
       !bytes(f,&s->program_table,&s->program_table_size,16388)||
       !word(f,&s->program_identity)||!bytes(f,&s->program,&s->program_size,4096)||
       !blobs(f,&s->metadata_assets,&s->metadata_count,64,2548)||
       !bytes(f,&s->profile,&n,16)||n!=16||!bytes(f,&s->speed_table,&s->speed_table_size,64)||
       !word(f,&value)||value>256)return 0;
    s->side_source=(int)value-1;
    if(!bytes(f,&s->transition_table,&s->transition_table_size,520)||
       !blobs(f,&s->transition_assets,&s->transition_count,64,4100)||
       !blobs(f,&s->transition_targets,&s->target_count,16384,16)||!word(f,&value)||value>4096)return 0;
    nflstreet_animation_registry_entry *r=calloc(value?value:1,sizeof *r);if(!r)return 0;
    s->registry=r;s->registry_count=value;
    for(unsigned i=0;i<value;++i){
        if(!bytes(f,&r[i].registry,&r[i].registry_size,28)||r[i].registry_size!=28||
           !bytes(f,&r[i].bank,&n,32)||(n!=0&&n!=32)||!bytes(f,&r[i].groups,&r[i].groups_size,8196))return 0;
    }
    return 1;
}
static void free_blobs(const nflstreet_animation_blob *b,size_t n){for(size_t i=0;i<n;++i)free((void *)b[i].data);free((void *)b);}
static void release(nflstreet_animation_sources *s){free((void *)s->bank);free((void *)s->groups);free((void *)s->program);free((void *)s->program_table);
    free((void *)s->profile);free((void *)s->speed_table);free((void *)s->transition_table);
    free_blobs(s->metadata_assets,s->metadata_count);free_blobs(s->transition_assets,s->transition_count);free_blobs(s->transition_targets,s->target_count);
    for(size_t i=0;i<s->registry_count;++i){free((void *)s->registry[i].registry);free((void *)s->registry[i].bank);free((void *)s->registry[i].groups);}free((void *)s->registry);}
static void hex(const uint8_t *p,size_t n){for(size_t i=0;i<n;++i)printf("%02x",p[i]);putchar('\n');}
int main(int argc,char **argv){FILE *f;uint8_t actor[0x14d4],partner[0x14d4],globals[8];uint32_t desc,new_group,old_group;
    nflstreet_animation_sources s={0};int mode,ok=0;
    if(argc!=2||!(f=fopen(argv[1],"rb")))return 2;
    mode=fgetc(f);
    if(mode=='A'||mode=='F'){
        nflstreet_asset_store store={0};nflstreet_loaded_animation *loaded=calloc(1,sizeof *loaded);
        const uint8_t *profile=NULL,*speed=NULL;size_t ps=0,ss=0;uint32_t bank,clip,context,side;int action=fgetc(f);
        if(!loaded||!readn(f,actor,sizeof actor)||!readn(f,partner,sizeof partner)||!readn(f,globals,8)||
           !word(f,&desc)||desc>255||!word(f,&new_group)||new_group>65535||!word(f,&old_group)||old_group>65535||
           !word(f,&bank)||bank>255||!word(f,&clip)||clip>65535||!word(f,&context)||context>65535||
           !word(f,&store.base)||!bytes(f,&store.arena,&store.arena_size,0x1800000)||
           !bytes(f,&store.registry,&store.registry_size,4096*28)||!bytes(f,&profile,&ps,16)||ps!=16||
           !bytes(f,&speed,&ss,64)||!word(f,&side)||side>256){ok=0;}
        else if(mode=='F'){
            uint32_t count;ok=word(f,&count)&&count<=4096;
            for(unsigned i=0;ok&&i<count;++i){uint32_t id;const uint8_t *raw=NULL;size_t n=0;
                ok=word(f,&id)&&bytes(f,&raw,&n,0x1000000);
                const uint8_t *destination=ok?nflstreet_asset_read(&store,id,n):NULL;
                if(!destination||!nflstreet_asset_relocate((uint8_t *)raw,n,id))ok=0;
                if(ok)memcpy((uint8_t *)destination,raw,n);
                free((void *)raw);
            }
        }else ok=1;
        if(ok&&fgetc(f)==EOF&&nflstreet_asset_sources(&store,(uint16_t)bank,(uint16_t)clip,(uint16_t)context,
                                      (uint16_t)new_group,(uint16_t)old_group,loaded)){
            loaded->source.profile=profile;loaded->source.speed_table=speed;loaded->source.speed_table_size=ss;
            loaded->source.side_source=(int)side-1;
            if(action=='C')ok=nflstreet_animation_callback(actor,partner,(uint8_t)desc,&loaded->source,globals);
            else if(action=='T')ok=nflstreet_animation_transition(actor,(uint16_t)new_group,(uint16_t)old_group,&loaded->source);
            else if(action=='G')ok=nflstreet_animation_group_update(actor,partner,(uint8_t)desc,&loaded->source,globals);
            else ok=0;
        }else ok=0;
        fclose(f);free(loaded);free((void *)store.arena);free((void *)store.registry);free((void *)profile);free((void *)speed);
        if(!ok)return 3;
        hex(actor,sizeof actor);hex(globals,8);return 0;
    }
    if(mode=='D'||mode=='L'){
        const uint8_t *blob=NULL;size_t size=0;uint32_t base=0;
        if((mode=='L'&&!word(f,&base))||!bytes(f,&blob,&size,0x1000000)||fgetc(f)!=EOF){fclose(f);free((void *)blob);return 2;}
        fclose(f);if(mode=='D'){float duration;uint32_t bits;
            ok=nflstreet_animation_program_duration(blob,size,&duration);free((void *)blob);if(!ok)return 3;
            memcpy(&bits,&duration,4);printf("%08x\n",bits);return 0;}
        ok=nflstreet_asset_relocate((uint8_t *)blob,size,base);if(ok)hex(blob,size);free((void *)blob);return ok?0:3;
    }
    if(mode=='P'){
        uint8_t manager[256],requests[2][16],descriptors[2];nflstreet_animation_sources pair[2]={{0},{0}};
        if(!readn(f,actor,sizeof actor)||!readn(f,partner,sizeof partner)||!readn(f,manager,256)||
           !readn(f,globals,8)||!readn(f,requests,sizeof requests)||!readn(f,descriptors,2)||
           !sources(f,pair)||!sources(f,pair+1)||fgetc(f)!=EOF){fclose(f);release(pair);release(pair+1);return 2;}
        fclose(f);ok=nflstreet_contact_animation_complete(actor,partner,manager,requests,descriptors,pair,globals);
        release(pair);release(pair+1);if(!ok)return 3;
        hex(actor,sizeof actor);hex(partner,sizeof partner);hex(manager,256);hex(globals,8);return 0;
    }
    if(mode=='S'){
        uint32_t clip,tag,group,target_clip,bank,identity;const nflstreet_animation_blob *assets=NULL;size_t count=0;
        if(!word(f,&clip)||clip>65535||!word(f,&tag)||tag>65535||!word(f,&group)||group>65535||
           !word(f,&target_clip)||target_clip>65535||!word(f,&bank)||bank>65535||
           !blobs(f,&assets,&count,64,4100)||fgetc(f)!=EOF){fclose(f);free_blobs(assets,count);return 2;}
        fclose(f);ok=nflstreet_animation_transition_select(assets,count,(uint16_t)clip,(uint16_t)tag,
                                                         (uint16_t)group,(uint16_t)target_clip,(uint16_t)bank,&identity);
        free_blobs(assets,count);if(!ok)return 3;printf("%08x\n",identity);return 0;
    }
    if(!readn(f,actor,sizeof actor)||!readn(f,partner,sizeof partner)||!readn(f,globals,8)||
       !word(f,&desc)||desc>255||!word(f,&new_group)||new_group>65535||!word(f,&old_group)||old_group>65535||
       !sources(f,&s)||fgetc(f)!=EOF){fclose(f);release(&s);return 2;}
    fclose(f);
    if(mode=='C')ok=nflstreet_animation_callback(actor,partner,(uint8_t)desc,&s,globals);
    else if(mode=='T')ok=nflstreet_animation_transition(actor,(uint16_t)new_group,(uint16_t)old_group,&s);
    else if(mode=='G')ok=nflstreet_animation_group_update(actor,partner,(uint8_t)desc,&s,globals);
    release(&s);if(!ok)return 3;hex(actor,sizeof actor);hex(globals,8);return 0;
}
