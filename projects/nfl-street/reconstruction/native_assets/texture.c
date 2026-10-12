#include "texture.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned u16(const uint8_t *p){return (unsigned)p[0]<<8|p[1];}
static uint32_t u32(const uint8_t *p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static int span(size_t n,size_t off,size_t len){return off<=n&&len<=n-off;}
static unsigned expand(unsigned v,unsigned bits){
    unsigned out=v<<(8-bits),filled=bits;
    while(filled<8){unsigned n=bits<8-filled?bits:8-filled;out|=(v>>(bits-n))<<(8-filled-n);filled+=n;}
    return out;
}
static uint32_t palette(unsigned v,unsigned fmt){
    unsigned r,g,b,a=255;
    if(fmt==0){r=g=b=v&255;a=v>>8;}
    else if(fmt==1){r=expand(v>>11,5);g=expand((v>>5)&63,6);b=expand(v&31,5);}
    else if(v&0x8000){r=expand((v>>10)&31,5);g=expand((v>>5)&31,5);b=expand(v&31,5);}
    else {a=expand(v>>12,3);r=expand((v>>8)&15,4);g=expand((v>>4)&15,4);b=expand(v&15,4);}
    return a<<24|r<<16|g<<8|b;
}
int nflstreet_texture_open_image(const uint8_t *b,size_t n,unsigned image,unsigned palette_index,nflstreet_texture *out){
    if(!b||!out||n<40||memcmp(b,"MMAP",4)||u16(b+4)!=2||image>=u16(b+14))return 0;
    size_t td=u32(b+24)+(size_t)image*16,pd=u32(b+28)+(size_t)palette_index*12;
    if(!span(n,td,16))return 0;
    unsigned w=u16(b+td),h=u16(b+td+2),fmt=u16(b+td+4),pf=0;
    if(!w||!h||w>4096||h>4096||fmt>14||fmt==7||(fmt>10&&fmt!=14))return 0;
    size_t size=u32(b+td+8),off=u32(b+td+12),ps=0,po=0;
    int paletted=fmt>=8&&fmt<=10;
    if(paletted){if(!u32(b+28)||palette_index>=u16(b+16)||!span(n,pd,12))return 0;
        pf=u16(b+pd+2);ps=u32(b+pd+4);po=u32(b+pd+8);
        if(pf>2||!span(n,po,ps)||ps%2)return 0;
    }
    unsigned tilew=(fmt==0||fmt==1||fmt==2||fmt==8||fmt==9||fmt==14)?8:4;
    unsigned tileh=(fmt==0||fmt==8||fmt==14)?8:4,block_size=fmt==6?64:32;
    size_t tilesx=(w+tilew-1)/tilew,tilesy=(h+tileh-1)/tileh;
    if(size<tilesx*tilesy*block_size||!span(n,off,size))return 0;
    nflstreet_texture t={w,h,NULL};t.bgra=malloc((size_t)w*h*4);if(!t.bgra)return 0;
    for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;++x){
        size_t block=((y/tileh)*tilesx+x/tilew)*block_size,within=(y%tileh)*tilew+x%tilew;
        const uint8_t *p=b+off+block;uint32_t pixel=0;unsigned idx=0,v=0,a=255;
        if(fmt==0||fmt==8){v=p[within/2];v=within%2?v&15:v>>4;idx=v;v=expand(v,4);}
        else if(fmt==1||fmt==2||fmt==9){v=p[within];idx=v;}
        else if(fmt==10)idx=u16(p+within*2)&0x3fff;
        if(paletted){if(idx>=ps/2){free(t.bgra);return 0;}pixel=palette(u16(b+po+idx*2),pf);}
        else if(fmt==0||fmt==1){pixel=v<<24|v<<16|v<<8|v;}
        else if(fmt==2){a=expand(v>>4,4);v=expand(v&15,4);pixel=a<<24|v<<16|v<<8|v;}
        else if(fmt==3)pixel=palette(u16(p+within*2),0);
        else if(fmt==4||fmt==5)pixel=palette(u16(p+within*2),fmt-3);
        else if(fmt==6){size_t j=within*2;pixel=(uint32_t)p[j]<<24|(uint32_t)p[j+1]<<16|(uint32_t)p[j+32]<<8|p[j+33];}
        else if(fmt==14){
            const uint8_t *q=p+((y%8)/4*2+(x%8)/4)*8;
            unsigned c0=u16(q),c1=u16(q+2),select=(q[4+y%4]>>(6-2*(x%4)))&3;
            uint32_t p0=palette(c0,1),p1=palette(c1,1);
            if(select<2)pixel=select?p1:p0;
            else {pixel=(select==3&&c0<=c1)?0:0xff000000;
                for(unsigned shift=0;shift<24;shift+=8){unsigned v0=(p0>>shift)&255,v1=(p1>>shift)&255,c;
                    if(c0<=c1)c=(v0+v1)/2;
                    else c=select==2?(v0*5+v1*3)/8:(v0*3+v1*5)/8;
                    pixel|=c<<shift;
                }
            }
        }
        t.bgra[(size_t)y*w+x]=pixel;
    }
    *out=t;return 1;
}
int nflstreet_texture_open(const uint8_t *b,size_t n,nflstreet_texture *out){return nflstreet_texture_open_image(b,n,0,0,out);}
int nflstreet_texture_find(const uint8_t *b,size_t n,const char *name,unsigned *image,unsigned *palette_index){
    if(!b||!name||!image||!palette_index||n<40||memcmp(b,"MMAP",4)||u16(b+4)!=2)return 0;
    unsigned count=u16(b+12),found=0;size_t names=u32(b+32),rows=u32(b+20);
    if(!names||!span(n,names,(size_t)count*16)||!span(n,rows,(size_t)count*12))return 0;
    unsigned selected=0,pal=0;
    for(unsigned i=0;i<count;++i)if(!strncmp(name,(const char *)b+names+i*16,16)){
        const uint8_t *r=b+rows+i*12;if(u16(r+2)!=1)return 0;
        selected=u16(r+4);pal=u16(r+8);++found;
    }
    if(found!=1||selected>=u16(b+14))return 0;
    *image=selected;*palette_index=pal;return 1;
}
void nflstreet_texture_close(nflstreet_texture *t){if(t){free(t->bgra);memset(t,0,sizeof *t);}}
static void le32(uint8_t *p,uint32_t v){for(unsigned i=0;i<4;++i)p[i]=(uint8_t)(v>>(i*8));}
int nflstreet_texture_bmp(const nflstreet_texture *t,const char *path){
    if(!t||!t->bgra||!path)return 0;
    uint8_t h[54]={0};h[0]='B';h[1]='M';le32(h+2,54+t->width*t->height*4);le32(h+10,54);le32(h+14,40);
    le32(h+18,t->width);le32(h+22,t->height);h[26]=1;h[28]=32;
    FILE *f=fopen(path,"wb");if(!f)return 0;int ok=fwrite(h,1,54,f)==54;
    for(unsigned y=t->height;y--&&ok;){
        /* Explicit little endian pixels for portable BMP output. */
        for(unsigned x=0;x<t->width;++x){uint8_t p[4];le32(p,t->bgra[(size_t)y*t->width+x]);if(fwrite(p,1,4,f)!=4){ok=0;break;}}
    }
    if(fclose(f))ok=0;
    return ok;
}
