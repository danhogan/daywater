#include "model.h"
#include "texture.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
static unsigned u16(const uint8_t *p){return (unsigned)p[0]<<8|p[1];}
static uint32_t u32(const uint8_t *p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static int span(size_t size,size_t offset,size_t n){return offset<=size&&n<=size-offset;}
static int string(const uint8_t *b,size_t size,size_t offset,char *out,size_t capacity){
    if(!offset||offset>=size)return 0;
    for(size_t i=0;i<capacity;++i){if(i>=size-offset)return 0;out[i]=(char)b[offset+i];if(!out[i])return 1;}
    return 0;
}
static float number(const uint8_t *p,unsigned type,unsigned frac){
    if(type==4){uint32_t bits=u32(p);float f;memcpy(&f,&bits,4);return f;}
    int value=type==0?p[0]:type==1?(int)(int8_t)p[0]:type==2?(int)u16(p):(int)(int16_t)u16(p);
    return ldexpf((float)value,-(int)frac);
}
static int vertex(const uint8_t *b,size_t size,const uint8_t *attrs,unsigned n,
                  const uint8_t **stream,const uint8_t *end,nflstreet_mesh_vertex *out,int inspection,unsigned *ignored){
    memset(out,0,sizeof *out);memset(out->rgba,255,4);int position=0;unsigned last=0;
    for(unsigned i=0;i<n;++i){const uint8_t *a=attrs+i*12;
        unsigned id=a[6],stride=a[7],type=a[9],frac=a[10],mode=a[11]&15;
        if(id<9||id>20||(i&&id<=last)||(mode!=2&&mode!=3)||!stride)return 0;
        last=id;unsigned index;
        if((size_t)(end-*stream)<(mode==2?1u:2u))return 0;
        if(mode==2)index=*(*stream)++;else {index=u16(*stream);*stream+=2;}
        if(!span(size,u32(a)+(size_t)stride*index,stride))return 0;
        if(index>=u16(a+4)){
            if(!inspection||(id!=11&&id!=12))return 0;
            ++*ignored;
        }
        if(inspection==3&&id==11){
            if(type!=5||stride!=4||a[8]!=1)return 0;
            memcpy(out->rgba,b+u32(a)+(size_t)stride*index,4);continue;
        }
        if(id!=9&&id!=13)continue;
        if(type>4||frac>31||a[8]>1)return 0;
        unsigned component=type<2?1:type<4?2:4,count=id==9?(a[8]?3:2):(a[8]?2:1);
        if(count*component>stride)return 0;
        const uint8_t *p=b+u32(a)+(size_t)stride*index;
        for(unsigned j=0;j<count;++j){float v=number(p+j*component,type,frac);if(!isfinite(v))return 0;
            if(id==9)out->position[j]=v;else out->uv[j]=v;
        }
        if(id==9)position=1;
    }
    return position;
}
static int triangle(nflstreet_mesh *m,size_t capacity,const nflstreet_mesh_vertex *a,
                    const nflstreet_mesh_vertex *b,const nflstreet_mesh_vertex *c,unsigned material,unsigned source_offset){
    if(m->count>=capacity)return 0;
    nflstreet_mesh_triangle *t=m->triangles+m->count++;t->vertex[0]=*a;t->vertex[1]=*b;t->vertex[2]=*c;t->material=(uint16_t)material;t->source_offset=source_offset;
    for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j){float v=t->vertex[i].position[j];
        if(m->count==1&&i==0)m->minimum[j]=m->maximum[j]=v;
        if(v<m->minimum[j])m->minimum[j]=v;
        if(v>m->maximum[j])m->maximum[j]=v;
    }
    return 1;
}
static int palette(const uint8_t *b,size_t size,size_t skel,size_t clst,float matrices[256][12],unsigned *out){
    if(!skel||!clst||!span(size,skel,24)||!span(size,clst,8))return 0;
    unsigned bones=u32(b+skel),count=u32(b+clst+4);size_t translations=u32(b+skel+8),rotations=u32(b+skel+12),scales=u32(b+skel+16),rows=u32(b+clst);
    if(!bones||bones>256||!count||count>256||!span(size,translations,bones*16)||!span(size,rotations,bones*6)||!span(size,scales,bones*12)||!span(size,rows,count*4))return 0;
    /* Stored-pose inspection chooses identity, then each cluster concatenates
       its parent palette with a bone's local TRS matrix. `Skel` stores three
       scale floats per bone; apply them to the rotation columns (R*S). This
       does not reproduce live animation interpolation or the renderer's
       current actor/world matrix. */
    for(unsigned i=0;i<bones*3;++i)
        if(!isfinite(number(b+scales+i*4,4,0)))return 0;
    memset(matrices,0,256*12*sizeof(float));matrices[0][0]=matrices[0][5]=matrices[0][10]=1;
    if(b[rows]!=0||b[rows+1]!=255||u16(b+rows+2)!=4096)return 0;
    for(unsigned i=1;i<count;++i){const uint8_t *r=b+rows+i*4;unsigned bone=r[0],parent=r[1];
        if(bone>=bones||parent>=i||u16(r+2)>4096)return 0;
        double a[3];for(unsigned j=0;j<3;++j)a[j]=(double)(int16_t)u16(b+rotations+bone*6+j*2)*(6.2831853071795864769/65536.0);
        double sx=sin(a[0]),cx=cos(a[0]),sy=sin(a[1]),cy=cos(a[1]),sz=sin(a[2]),cz=cos(a[2]);
        float local[12]={(float)(cz*cy),(float)(-sz*cx+cz*sx*sy),(float)(sz*sx+cz*cx*sy),0,
                         (float)(sz*cy),(float)(cz*cx+sz*sx*sy),(float)(-cz*sx+sz*cx*sy),0,
                         (float)-sy,(float)(cy*sx),(float)(cy*cx),0};
        for(unsigned row=0;row<3;++row)for(unsigned column=0;column<3;++column){
            float scale=number(b+scales+bone*12+column*4,4,0);
            local[row*4+column]*=scale;
            if(!isfinite(local[row*4+column]))return 0;
        }
        for(unsigned j=0;j<3;++j){local[j*4+3]=number(b+translations+bone*16+j*4,4,0);if(!isfinite(local[j*4+3]))return 0;}
        for(unsigned j=0;j<3;++j)for(unsigned k=0;k<4;++k){float v=k==3?matrices[parent][j*4+3]:0;
            for(unsigned l=0;l<3;++l)v+=matrices[parent][j*4+l]*local[l*4+k];
            if(!isfinite(v))return 0;
            matrices[i][j*4+k]=v;
        }
    }
    *out=count;return 1;
}
static int open_mesh(const uint8_t *b,size_t size,nflstreet_mesh *out,int inspection){
    if(!b||!out||size<16||memcmp(b,"TMdl",4)||u32(b+4)>size||b[15]!=0)return 0;
    unsigned sections=u16(b+8);size_t directory=u16(b+10),geom=0,matl=0,skel=0,clst=0;unsigned embedded=0;
    if(!span(size,directory,(size_t)sections*16))return 0;
    nflstreet_mesh m={0};
    for(unsigned i=0;i<sections;++i){const uint8_t *s=b+directory+i*16;size_t offset=u32(s+4),n=u32(s+8);
        if(!span(size,offset,n))return 0;
        if(!memcmp(s,"Geom",4)){if(geom||n<24)return 0;geom=offset;}
        if(!memcmp(s,"Matl",4)){if(matl||n<8)return 0;matl=offset;}
        if(!memcmp(s,"Skel",4)){if(skel||n<24)return 0;skel=offset;}
        if(!memcmp(s,"Clst",4)){if(clst||n<8)return 0;clst=offset;}
        if(!memcmp(s,"Info",4)){
            size_t length=0;while(length<n&&length<sizeof m.name-1&&b[offset+length])++length;
            memcpy(m.name,b+offset,length);m.name[length]=0;
        }
        if(!memcmp(s,"Text",4)&&n>=4&&!memcmp(b+offset,"MMAP",4)){
            ++embedded;m.texture_bytes=b+offset;m.texture_size=n;
        }
    }
    if(!geom)return 0;
    const uint8_t *g=b+geom;size_t draws=u32(g),arrays=u32(g+8);
    unsigned count=u32(g+4),attr_count=u32(g+12);
    if(!count||count>4096||attr_count>32768||!span(size,draws,(size_t)count*24)||!span(size,arrays,(size_t)attr_count*12))return 0;
    size_t capacity=0;int matrix_draws=0;
    for(unsigned i=0;i<count;++i){const uint8_t *d=b+draws+i*24;
        size_t bytes=(size_t)u16(d+4)*32;
        /* Some TMdl Geom tables retain empty row descriptors. They have no
           draw bytes and are skipped by the display-list walker. */
        if(!bytes){if(u32(d)!=0)return 0;continue;}
        if((d[17]&1)&&inspection!=2)return 0;
        if(d[17]&1)matrix_draws=1;
        if(!span(size,u32(d),bytes)||u16(d+14)>attr_count||d[16]>attr_count-u16(d+14))return 0;
        if(bytes/2>2000000||capacity>2000000-bytes/2)return 0;
        capacity+=bytes/2;
    }
    float matrices[256][12];unsigned palette_count=0;int slots[64];for(unsigned i=0;i<64;++i)slots[i]=-1;
    if(matrix_draws&&!palette(b,size,skel,clst,matrices,&palette_count))return 0;
    if(!capacity)return 0;
    m.triangles=malloc(capacity*sizeof *m.triangles);if(!m.triangles)return 0;m.draws=count;
    if(matl){m.material_count=u32(b+matl);size_t records=u32(b+matl+4);
        if(!m.material_count||m.material_count>4096||!span(size,records,(size_t)m.material_count*48))goto fail;
        m.materials=calloc(m.material_count,sizeof *m.materials);if(!m.materials)goto fail;
        for(unsigned i=0;i<m.material_count;++i){const uint8_t *r=b+records+i*48;nflstreet_mesh_material *material=m.materials+i;
            material->texture_index=-1;material->palette_index=-1;
            if(embedded==1){material->texture_bytes=m.texture_bytes;material->texture_size=m.texture_size;}
            if(!string(b,size,u32(r+4),material->shader,sizeof material->shader)||u16(r+8)>8)goto fail;
            if(u16(r+8)&&!string(b,size,u32(r+12),material->texture_name,sizeof material->texture_name))goto fail;
            /* Original names map to MMAP image/palette selectors. Only static
               one-frame names and a single OnePass texture are supported. */
            if(embedded==1&&!strcmp(material->shader,"OnePass")&&u16(r+8)==1&&
               m.texture_size>=40){unsigned image,pal;
                if(nflstreet_texture_find(m.texture_bytes,m.texture_size,material->texture_name,&image,&pal)){
                    material->texture_index=(int)image;material->palette_index=(int)pal;
                }
            }
        }
    }
    for(unsigned i=0;i<count;++i){const uint8_t *d=b+draws+i*24;
        const uint8_t *p=b+u32(d),*end=p+(size_t)u16(d+4)*32,*attrs=b+arrays+(size_t)u16(d+14)*12;
        unsigned nattrs=d[16],material=u16(d+6);
        if(m.material_count&&material>=m.material_count)goto fail;
        while(p<end){unsigned source_offset=(unsigned)(p-b),command=*p++;if(!command)continue;
            if(inspection==2&&(d[17]&1)&&(command==0x20||command==0x28)){
                if(end-p<4)goto fail;
                unsigned source=u16(p),packed=u16(p+2),length=(packed>>12)+1,address=packed&4095;p+=4;
                if(command==0x20){
                    if(source>=palette_count||length!=12||address%4||address/4>61)goto fail;
                    /* A partial overwrite invalidates an earlier three-row
                       mapping; never reuse a stale matrix after such a load. */
                    unsigned slot=address/4;
                    for(unsigned k=0;k<64;++k)if(k<=slot+2&&slot<=k+2)slots[k]=-1;
                    slots[address/4]=(int)source;
                }else {
                    /* Indexed normal rows are parsed/bounded; inspection
                       lighting uses triangle normals, not original normals. */
                    if(source>=palette_count*3||length!=3||address<0x400||address+3>0x500)goto fail;
                }
                continue;
            }
            /* Only VAT zero, as configured by original 0x801CCCC0. */
            if(command!=0x80&&command!=0x90&&command!=0x98&&command!=0xa0)goto fail;
            if(end-p<2)goto fail;
            unsigned vertices=u16(p);p+=2;
            if(vertices<3||(command==0x80&&vertices%4)||(command==0x90&&vertices%3))goto fail;
            nflstreet_mesh_vertex first={0},prev={0},older={0},quad[4];
            for(unsigned j=0;j<vertices;++j){nflstreet_mesh_vertex v;
                int transform=-1;
                if(d[17]&1){if(p==end||*p>=64||slots[*p]<0)goto fail;transform=slots[*p++];}
                if(!vertex(b,size,attrs,nattrs,&p,end,&v,inspection,&m.ignored_color_indices))goto fail;
                if(transform>=0){float xyz[3];for(unsigned row=0;row<3;++row){xyz[row]=matrices[transform][row*4+3];
                    for(unsigned k=0;k<3;++k)xyz[row]+=matrices[transform][row*4+k]*v.position[k];
                    if(!isfinite(xyz[row]))goto fail;
                }memcpy(v.position,xyz,sizeof xyz);}
                if(!j)first=v;
                if(command==0x80){quad[j%4]=v;if(j%4==3){
                    if(!triangle(&m,capacity,quad,quad+1,quad+2,material,source_offset)||!triangle(&m,capacity,quad,quad+2,quad+3,material,source_offset))goto fail;}}
                else if(command==0x90){if(j%3==2&&!triangle(&m,capacity,&older,&prev,&v,material,source_offset))goto fail;}
                else if(j>=2){
                    if(command==0xa0){if(!triangle(&m,capacity,&first,&prev,&v,material,source_offset))goto fail;}
                    else if(j&1){if(!triangle(&m,capacity,&prev,&older,&v,material,source_offset))goto fail;}
                    else if(!triangle(&m,capacity,&older,&prev,&v,material,source_offset))goto fail;
                }
                older=prev;prev=v;
            }
        }
    }
    if(!m.count)goto fail;
    if(embedded!=1){m.texture_bytes=NULL;m.texture_size=0;}
    *out=m;return 1;
fail:nflstreet_mesh_close(&m);return 0;
}
int nflstreet_mesh_open(const uint8_t *b,size_t n,nflstreet_mesh *out){return open_mesh(b,n,out,0);}
int nflstreet_mesh_open_inspection(const uint8_t *b,size_t n,nflstreet_mesh *out){return open_mesh(b,n,out,1);}
int nflstreet_mesh_open_bind_pose(const uint8_t *b,size_t n,nflstreet_mesh *out){return open_mesh(b,n,out,2);}
int nflstreet_mesh_open_render(const uint8_t *b,size_t n,nflstreet_mesh *out){return open_mesh(b,n,out,3);}
void nflstreet_mesh_close(nflstreet_mesh *m){if(m){free(m->triangles);free(m->materials);memset(m,0,sizeof *m);}}
static int word(FILE *f,uint32_t v){uint8_t b[4];for(unsigned i=0;i<4;++i)b[i]=(uint8_t)(v>>(i*8));return fwrite(b,1,4,f)==4;}
int nflstreet_mesh_write(const nflstreet_mesh *m,const char *path){
    if(!m||!path||m->count>UINT32_MAX)return 0;
    FILE *f=fopen(path,"wb");if(!f)return 0;int ok=fwrite("NMSH",1,4,f)==4&&word(f,(uint32_t)m->count)&&word(f,m->draws);
    for(size_t i=0;i<m->count&&ok;++i){ok=word(f,m->triangles[i].material);
        for(unsigned j=0;j<3&&ok;++j)for(unsigned k=0;k<5&&ok;++k){uint32_t bits;
            float v=k<3?m->triangles[i].vertex[j].position[k]:m->triangles[i].vertex[j].uv[k-3];memcpy(&bits,&v,4);ok=word(f,bits);
        }
    }
    if(fclose(f))ok=0;
    return ok;
}
