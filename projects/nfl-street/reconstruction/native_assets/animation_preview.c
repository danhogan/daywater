/* Independent inspection playback of a direct bank clip. Captured draw,
   light/texture selections and an inspection placement remain inputs.
   Accessories follow the neck rigidly; this is not full actor playback. */
#include "player.h"
#include "pose.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define RECORD 3876u
static unsigned u16(const uint8_t *p){return (unsigned)p[0]<<8|p[1];}
static uint32_t u32(const uint8_t *p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static uint32_t le32(const uint8_t *p){return p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static void put(uint8_t *p,float f){uint32_t w;memcpy(&w,&f,4);for(unsigned i=0;i<4;++i)p[i]=(uint8_t)(w>>(i*8));}
static int span(size_t n,size_t o,size_t s){return o<=n&&s<=n-o;}
static uint8_t *readfile(const char *path,size_t *size){FILE *f=fopen(path,"rb");if(!f)return NULL;
    if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}long n=ftell(f);if(n<16||n>16*1024*1024||fseek(f,0,SEEK_SET)){fclose(f);return NULL;}
    uint8_t *b=malloc((size_t)n);if(b&&fread(b,1,(size_t)n,f)!=(size_t)n){free(b);b=NULL;}fclose(f);*size=(size_t)n;return b;
}
static int sections(const uint8_t *b,size_t n,size_t *sk,size_t *cl,size_t *ge){if(n<16||memcmp(b,"TMdl",4))return 0;
    unsigned dir=u16(b+10),count=u16(b+8);if(!span(n,dir,count*16))return 0;*sk=*cl=*ge=0;
    for(unsigned i=0;i<count;++i){const uint8_t *s=b+dir+i*16;size_t o=u32(s+4);if(!span(n,o,u32(s+8)))return 0;
        if(!memcmp(s,"Skel",4))*sk=o;if(!memcmp(s,"Clst",4))*cl=o;if(!memcmp(s,"Geom",4))*ge=o;}
    return *sk&&*cl&&*ge;
}
/* Resolve indexed loads up to the selected primitive. Their source indices
   are original Clst selectors; raw PN selectors need not be divisible by 3. */
static int slots(const uint8_t *b,size_t n,size_t geom,const uint8_t *r,int out[64]){for(unsigned i=0;i<64;++i)out[i]=-1;
    unsigned row=le32(r+8);if(row>=u32(b+geom+4))return 0;size_t d=u32(b+geom)+(size_t)row*24;if(!span(n,d,24))return 0;
    size_t at=u32(b+d),target=le32(r+12),end=at+(size_t)u16(b+d+4)*32;
    if(target<at||target>=end||!span(n,at,end-at))return 0;unsigned sz=1;size_t attrs=u32(b+geom+8)+(size_t)u16(b+d+14)*12;
    if(!span(n,attrs,b[d+16]*12))return 0;for(unsigned i=0;i<b[d+16];++i){unsigned mode=b[attrs+i*12+11]&15;if(mode<2||mode>3)return 0;sz+=mode-1;}
    while(at<target){unsigned cmd=b[at++];if(!cmd)continue;
        if(cmd==0x20||cmd==0x28){if(end-at<4)return 0;unsigned source=u16(b+at),packed=u16(b+at+2),address=packed&4095;at+=4;
            if(cmd==0x20){if((packed>>12)!=11||address%4||address/4>61)return 0;unsigned slot=address/4;
                for(unsigned i=0;i<64;++i)if(i<=slot+2&&slot<=i+2)out[i]=-1;out[slot]=(int)source;}continue;}
        if((cmd!=0x80&&cmd!=0x90&&cmd!=0x98&&cmd!=0xa0)||end-at<2)return 0;unsigned vertices=u16(b+at);at+=2;
        if((size_t)vertices*sz>end-at)return 0;at+=(size_t)vertices*sz;
    }
    if(at!=target||!span(n,target,3))return 0;
    unsigned vertices=u16(b+target+1);
    if(!span(n,target+3,(size_t)vertices*sz))return 0;
    for(unsigned i=0;i<vertices;++i){unsigned selector=b[target+3+(size_t)i*sz];if(selector>63||out[selector]<0)return 0;}
    return 1;
}
static int rotations(const uint8_t *bank,size_t bn,unsigned frame,const uint8_t *skeleton,size_t sn,const uint8_t *body,size_t n,size_t sk,int16_t pose[768]){
    int16_t raw[384];unsigned count;if(!nflstreet_pose_frame(bank,bn,frame,raw,384,&count)||sn<16||memcmp(skeleton,"SKL1",4))return 0;
    unsigned bones=u32(body+sk),external=u16(skeleton+6);size_t names=u32(body+sk+4),stored=u32(body+sk+12),table=u32(skeleton+12);
    if(bones>256||!span(n,names,bones*16)||!span(n,stored,bones*6)||!span(sn,table,external*20))return 0;
    for(unsigned i=0;i<bones*3;++i)pose[i]=(int16_t)u16(body+stored+i*2);
    /* Original non-scattered decoder preserves compact skeleton order. The
       descriptor IDs are used only by the alternate scattered branch. */
    for(unsigned i=0;i<count;++i){unsigned id=i,bone=id/3;if(bone>=external)return 0;
        const uint8_t *name=skeleton+table+bone*20+4;
        if(!memchr(name,0,16))return 0;
        for(unsigned j=0;j<bones;++j)if(!memcmp(name,body+names+j*16,16)){pose[j*3+id%3]=raw[i];break;}
    }return 1;
}
int main(int argc,char **argv){
    if(argc!=8){fprintf(stderr,"Usage: animation-preview unpacked-root fixed-profile bank skeleton clip frame-directory frame-count\n");return 2;}
    size_t pn,bn,sn;uint8_t *profile=readfile(argv[2],&pn),*bank=readfile(argv[3],&bn),*skeleton=readfile(argv[4],&sn);int status=1;
    if(!profile||!bank||!skeleton||memcmp(profile,"NAP1",4)||pn!=16+(size_t)le32(profile+4)*RECORD||!le32(profile+4))goto done;
    unsigned count=le32(profile+4),entry=le32(profile+20),clip=(unsigned)strtoul(argv[5],NULL,10),limit=(unsigned)strtoul(argv[7],NULL,10);
    if(le32(profile+16)!=0||count>2000||!limit||limit>600)goto done;
    char path[4096];if(snprintf(path,sizeof path,"%s/plamodel/%05u.bin",argv[1],entry)>=(int)sizeof path)goto done;
    size_t mn;uint8_t *body=readfile(path,&mn);size_t sk,cl,ge;if(!body||!sections(body,mn,&sk,&cl,&ge)){free(body);goto done;}
    size_t programs=u32(bank+12);if(!span(bn,programs,4)||clip>=u32(bank+programs)||!span(bn,programs+4+clip*4,4)){free(body);goto done;}
    size_t program=u32(bank+programs+4+clip*4);if(!span(bn,program,6)){free(body);goto done;}
    unsigned frames[1024],total=0;for(size_t at=program+4;span(bn,at,2)&&total<1024;at+=2){unsigned w=u16(bank+at);
        if(w==0x8000||w==0x8001)break;if(!(w&0x8000)){if(w>=u16(bank+4)){free(body);goto done;}frames[total++]=w;}
        else if(!(w&0x4000)){fprintf(stderr,"Unsupported clip control %04x\n",w);free(body);goto done;}}
    if(total<2||total==1024){free(body);goto done;}
    float current[256][12];unsigned clusters;int16_t pose[768];
    size_t rows=u32(body+cl),names=u32(body+sk+4);unsigned neck=0;
    if(!rotations(bank,bn,frames[0],skeleton,sn,body,mn,sk,pose)||!nflstreet_pose_clusters(body,mn,pose,u32(body+sk)*3,current,&clusters)){free(body);goto done;}
    for(unsigned i=1;i<clusters;++i){const uint8_t *row=body+rows+i*4;const uint8_t *name=body+names+row[0]*16;
        if(!strcmp((const char *)name,"neckhi")&&u16(row+2)==4096)neck=i;}
    if(!neck){free(body);goto done;}
    const char *categories[]={"plamodel","plaface","plahair","plahat","plaglass","plarosterhair"};
    uint8_t *animated=malloc(pn);if(!animated){free(body);goto done;}
    for(unsigned f=0;f<limit;++f){unsigned index=f%total;
        if(!rotations(bank,bn,frames[index],skeleton,sn,body,mn,sk,pose)||!nflstreet_pose_clusters(body,mn,pose,u32(body+sk)*3,current,&clusters))goto frames_done;
        memcpy(animated,profile,pn);
        unsigned oldcat=UINT32_MAX,oldentry=UINT32_MAX;uint8_t *component=NULL;size_t component_size=0,component_sk=0,component_cl=0,component_ge=0;
        float component_pose[256][12];unsigned component_clusters=0;
        for(unsigned i=0;i<count;++i){uint8_t *r=animated+16+(size_t)i*RECORD;unsigned cat=le32(r),model_entry=le32(r+4);
            if(cat>=6){free(component);goto frames_done;}
            if(cat!=oldcat||model_entry!=oldentry){free(component);component=NULL;
                if(snprintf(path,sizeof path,"%s/%s/%05u.bin",argv[1],categories[cat],model_entry)>=(int)sizeof path)goto frames_done;
                component=readfile(path,&component_size);if(!component||!sections(component,component_size,&component_sk,&component_cl,&component_ge)){free(component);goto frames_done;}
                if(!rotations(bank,bn,frames[index],skeleton,sn,component,component_size,component_sk,pose)||!nflstreet_pose_clusters(component,component_size,pose,u32(component+component_sk)*3,component_pose,&component_clusters)){free(component);goto frames_done;}
                /* Observed hair assets have identity-only Clst and a neckhi
                   root. In this inspection path attach that identity to the
                   full-weight body neck; secondary hair dynamics are absent. */
                if(component_clusters==1){size_t name_table=u32(component+component_sk+4);
                    if(!span(component_size,name_table,16)||strcmp((const char *)(component+name_table),"neckhi")){free(component);goto frames_done;}
                    memcpy(component_pose[0],current[neck],sizeof component_pose[0]);}
                oldcat=cat;oldentry=model_entry;
            }
            int sl[64];if(!slots(component,component_size,component_ge,r,sl)){free(component);goto frames_done;}
            for(unsigned j=0;j<64;++j)if(sl[j]>=0){if((unsigned)sl[j]>=component_clusters){free(component);goto frames_done;}
                const float *m=component_pose[sl[j]];
                for(unsigned k=0;k<12;++k)put(r+256+j*16+k*4,m[k]);
                for(unsigned row=0;row<3;++row)for(unsigned col=0;col<3;++col)put(r+1312+(j&31)*12+row*12+col*4,m[row*4+col]);
            }
        }
        free(component);
        nflstreet_player player={0};nflstreet_texture image={0};if(!nflstreet_player_open(argv[1],animated,pn,&player)||!nflstreet_player_portrait(&player,&image)){nflstreet_player_close(&player);goto frames_done;}
        if(snprintf(path,sizeof path,"%s/frame-%03u.bmp",argv[6],f)>=(int)sizeof path||!nflstreet_texture_bmp(&image,path)){nflstreet_texture_close(&image);nflstreet_player_close(&player);goto frames_done;}
        nflstreet_texture_close(&image);nflstreet_player_close(&player);
    }
    printf("Native inspection animation: clip %u, %u source frames, %u rendered frames. Native template head; hair follows neck. Inspection camera; root travel/events are not applied.\n",clip,total,limit);status=0;
frames_done:free(animated);free(body);
done:free(profile);free(bank);free(skeleton);return status;
}
