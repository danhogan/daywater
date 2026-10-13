#define main matrix_actor_fixture_main
#include "actor_model_init_original_check.c"
#undef main
#include "cluster_palette.h"
#include "model_relocate.h"
#include "../menus/team_player_body_binding.h"
#include <math.h>
#define MODEL_TOKEN 0x80400000u
#define DEST 0x81000000u
#define SCRATCH 0x81100000u
#define ANGLES 0x81200000u
#define PALETTE 0x81300000u
#define MATRIX_STACK 0x81400000u
static uint8_t *file_bytes(const char *path,size_t *size){FILE *f=fopen(path,"rb");if(!f)return NULL;
 if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}long n=ftell(f);if(n<=0||fseek(f,0,SEEK_SET)){fclose(f);return NULL;}
 uint8_t *p=malloc((size_t)n);if(!p||fread(p,1,(size_t)n,f)!=(size_t)n){free(p);fclose(f);return NULL;}fclose(f);*size=(size_t)n;return p;}
static float real(const uint8_t *p){uint32_t w=be(p);float f;memcpy(&f,&w,4);return f;}
static void put_float(uint8_t *p,float f){uint32_t w;memcpy(&w,&f,4);put(p,w);}
static int original_palette(const ModernGekkoModuleDesc *m,uint8_t *ram,uint32_t skel,uint32_t clst,int posed,unsigned mode){
 CPUState c={0};c.ram=ram;c.ram_size=RAM_SIZE;c.msr=0x2000;c.hid2=0xe0000000u;c.gqr[6]=0x0f070007;
 c.gpr[1]=0x81700000u;c.gpr[2]=0x803b9a40u;c.gpr[13]=0x803a9a40u;
 c.gpr[3]=PALETTE;c.gpr[4]=skel;c.gpr[5]=clst;c.gpr[6]=posed?ANGLES:0;c.gpr[7]=SCRATCH;
 c.pc=mode==2?0x801ca720u:0x801ca4e8u;
 for(unsigned step=0;c.pc&&step<2000000;++step){
  if(c.pc==0x801f0bd0u){c.pc=c.lr;continue;}
  if(c.pc==0x801f0dd4u){uint8_t *d=ptr(ram,c.gpr[3],c.gpr[5]),*s=ptr(ram,c.gpr[4],c.gpr[5]);if(!d||!s)return 0;memmove(d,s,c.gpr[5]);c.pc=c.lr;continue;}
  c.downcount=-1000000000;m->dispatch(&c,c.pc);if(c.exception)return 0;
 }return !c.pc;
}
int main(int argc,char **argv){
 if(argc!=4)return 2;HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;
 const ModernGekkoModuleDesc *m=get();uint8_t *baseline=load(argv[2]);size_t archive_size;uint8_t *archive=file_bytes(argv[3],&archive_size),*ram=malloc(RAM_SIZE);
 if(!m||!baseline||!archive||!ram)return 3;float polynomial[6];for(unsigned i=0;i<6;++i)polynomial[i]=real(ptr(baseline,0x8039e100u+i*4,4));
 unsigned cases=0,matrices=0,guards=0;
 for(unsigned type=0;type<4;++type)for(unsigned variant=0;variant<3;++variant){nflstreet_team_body_binding body={0};
  if(!nflstreet_team_body_binding_open(archive,archive_size,type==3?7u:type,variant,&body))return 4;
  size_t n;const nflstreet_model_chunk_registration *registry=nflstreet_model_default_registry(&n);
  if(!nflstreet_model_relocate(body.bytes,body.size,MODEL_TOKEN,registry,n))return 4;
  uint8_t view[52]={0};nflstreet_model_view_source source={body.bytes,body.size,MODEL_TOKEN};if(!nflstreet_model_view_attach(view,52,&source))return 4;
  uint32_t skel=be(view+0x10),clst=be(view+0x0c);size_t skel_at=skel-MODEL_TOKEN,clst_at=clst-MODEL_TOKEN;
  unsigned bones=be(body.bytes+skel_at),clusters=be(body.bytes+clst_at+4);size_t rotations=be(body.bytes+skel_at+12)-MODEL_TOKEN,rows=be(body.bytes+clst_at)-MODEL_TOKEN;
  uint8_t *original_rows=malloc(clusters*4u);if(!original_rows)return 4;memcpy(original_rows,body.bytes+rows,clusters*4u);
  for(unsigned test=0;test<20;++test){float root[12]={1,0,0,0,0,1,0,0,0,0,1,0};int16_t angles[768];
   if(test&1){root[3]=1.25f;root[7]=-2.5f;root[11]=0.375f;}
   if(test&2){root[0]=0;root[2]=1;root[8]=-1;root[10]=0;}
   if(test&4){root[0]*=1.25f;root[5]*=0.75f;root[10]*=1.5f;}
   if(test==19){float shear[12]={0.95f,0.125f,-0.3f,1.25f,-0.2f,0.8f,0.0625f,-2.5f,0.175f,-0.125f,1.125f,0.375f};memcpy(root,shear,sizeof root);}
   memcpy(body.bytes+rows,original_rows,clusters*4u);
   for(unsigned a=0;a<bones*3;++a){int base=(int16_t)be16(body.bytes+rotations+a*2u);
    angles[a]=(int16_t)(uint16_t)(base+((test&8)?((int)(a%7)-3)*437:0));if(test==17)angles[a]=a%2?INT16_MIN:INT16_MAX;
   }
   if(test>=17){const unsigned weights[]={0,1,2048,4096,65535};for(unsigned c=1;c<clusters;++c){unsigned w=test==18?0:weights[c%5];body.bytes[rows+c*4+2]=(uint8_t)(w>>8);body.bytes[rows+c*4+3]=(uint8_t)w;}}
   float native[256][12];unsigned count;if(!nflstreet_cluster_palette(&source,view,test==16?NULL:angles,bones*3,root,polynomial,native,&count)||count!=clusters)return 5;
   for(unsigned mode=0;mode<3;++mode){memcpy(ram,baseline,RAM_SIZE);memcpy(ptr(ram,MODEL_TOKEN,body.size),body.bytes,body.size);
    put(ptr(ram,PALETTE,4),DEST);put(ptr(ram,PALETTE+4,4),mode==1?SCRATCH:0);
    put(ptr(ram,0x803a3458u,4),MATRIX_STACK);put(ptr(ram,MATRIX_STACK,4),1);put(ptr(ram,MATRIX_STACK+4,4),0);put(ptr(ram,MATRIX_STACK+8,4),MATRIX_STACK+0x100);
    for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c)put_float(ptr(ram,MATRIX_STACK+0x100+(r*4+c)*4,4),r<3?root[r*4+c]:(c==3?1.0f:0.0f));
    for(unsigned a=0;a<bones*3;++a){uint8_t *p=ptr(ram,ANGLES+a*2,2);p[0]=(uint8_t)((uint16_t)angles[a]>>8);p[1]=(uint8_t)angles[a];}
    if(!original_palette(m,ram,skel,clst,test!=16,mode))return 6;uint32_t out_token=mode==2?SCRATCH:DEST;
    for(unsigned c=0;c<clusters;++c)for(unsigned w=0;w<12;++w){uint32_t bits;memcpy(&bits,native[c]+w,4);uint32_t actual=be(ptr(ram,out_token+c*48+w*4,4));
     if(bits!=actual){fprintf(stderr,"Matrix mismatch type%u variant%u test%u mode%u cluster%u word%u native=%08x original=%08x\n",type,variant,test,mode,c,w,bits,actual);return 7;}}
    ++cases;matrices+=clusters;
   }
  }
  memcpy(body.bytes+rows,original_rows,clusters*4u);
  if(!type&&!variant){float out[256][12],saved[256][12],root[12]={1,0,0,0,0,1,0,0,0,0,1,0};memset(out,0xa5,sizeof out);memcpy(saved,out,sizeof saved);unsigned count=99;int16_t angles[768]={0};
   uint8_t old=body.bytes[rows+5];body.bytes[rows+5]=1;
   if(nflstreet_cluster_palette(&source,view,angles,bones*3,root,polynomial,out,&count)||count!=99||memcmp(out,saved,sizeof out))return 8;++guards;body.bytes[rows+5]=old;
   if(nflstreet_cluster_palette(&source,view,angles,1,root,polynomial,out,&count)||count!=99||memcmp(out,saved,sizeof out))return 8;++guards;
   root[0]=NAN;if(nflstreet_cluster_palette(&source,view,NULL,0,root,polynomial,out,&count)||count!=99||memcmp(out,saved,sizeof out))return 8;++guards;
  }
  free(original_rows);nflstreet_team_body_binding_close(&body);
 }
 free(ram);free(baseline);free(archive);FreeLibrary(h);
 printf("Cluster palettes: %u original helper calls, %u complete matrices bit-exact; 12 body type/variant models x20 root/pose cases x3 output paths; %u atomic guards; polynomial and root supplied; no actor caller.\n",cases,matrices,guards);return 0;
}
