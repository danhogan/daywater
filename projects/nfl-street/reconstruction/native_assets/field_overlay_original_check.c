/* Whole-RAM and ordered GX/pipe comparison for camera traversal entry 14.
   Case zero: saved RAM with its enable byte forced on and field stack selected.
   Edited cases supply controlled records and camera slots, retaining all game
   routines in the oracle. GXPeekZ is the only supplied hardware read. */
#include <windows.h>
#include <math.h>
#include "scenery_draw.h"
#include "check_fixture.h"
#include "field_overlay.h"
#include "field_overlay_oracle.h"
#define CASES 601u
#define RECORDS 0x80f00000u
#define DESCS 0x80f01000u
static void putf(uint8_t *ram,uint32_t a,float v){wr32(ram_at(ram,a,4),fbits(v));}
static int depth(void *p,uint32_t x,uint32_t y,uint32_t *v){uint32_t index=*(uint32_t *)p;
 *v=index%4==0?0:index%4==1?0xffffffu:index%4==2?0x7fffffu:(x*131u+y*197u+index*977u)&0xffffffu;return 1;}
static void identity(uint8_t *ram,uint32_t a){for(unsigned i=0;i<16;++i)putf(ram,a+4*i,i%5==0?1.0f:0.0f);}
static void prepare(uint8_t *ram,unsigned index){
 nflstreet_matrix_stacks k=venue_stacks(ram);wr32(ram_at(ram,R13-0x65e8u,4),k.table+12u*ram_at(ram,R13-0x4ee8u,1)[0]);ram_at(ram,R13-0x71c0u,1)[0]=1;
 if(!index)return;
 /* Controlled matrices make projected boundary conditions reproducible and
    avoid mode-1 reads of undefined output after 80159BB4 rejects a point. */
 identity(ram,0x80331fc0u);identity(ram,0x80331f80u);
 if(index%11==0)for(unsigned i=0;i<12;++i)putf(ram,0x80331f80u+4*i,rndf(-1.5f,1.5f));
 uint32_t n=1+rnd()%4;wr32(ram_at(ram,0x802aa440u,4),n);wr32(ram_at(ram,0x802aa448u,4),RECORDS);
 memset(ram_at(ram,RECORDS,0xc4*4),0,0xc4*4);memset(ram_at(ram,DESCS,24*20),0,24*20);
 for(unsigned i=0;i<n;++i){uint32_t rec=RECORDS+0xc4*i;uint8_t *r=ram_at(ram,rec,0xc4);unsigned layers=rnd()%5;
  wr32(r,DESCS+24*5*i);wr32(r+4,rnd()%2);wr32(r+8,rnd()%2);unsigned mode=rnd()%5;wr32(r+0xc,mode);
  r[0x10]=index%17==0?0:(uint8_t)(rnd()|1);r[0x11]=(uint8_t)(rnd()%2);
  putf(ram,rec+0x14,(float[]){0,1,rndf(0.0f,1.0f),-0.1f,1.1f}[rnd()%5]);
  float x=(float[]){-1,1,0,-0.5f,0.5f,-1.3f,1.3f,-2.5f,2.5f,rndf(-2.4f,2.4f)}[rnd()%10];
  float y=rndf(-2.4f,2.4f),z=rndf(-0.9f,1.0f);if(mode==1){x=rndf(-1.1f,1.1f);y=rndf(-1.1f,1.1f);}
  if(index%19==0&&mode!=1)z=-1.01f;
  putf(ram,rec+0x18,x);putf(ram,rec+0x1c,y);putf(ram,rec+0x20,z);
  for(unsigned j=0;j<3;++j)putf(ram,rec+0x24+4*j,rndf(-2,2));putf(ram,rec+0x30,rndf(0,0.9f));
  r[0x34]=(uint8_t)(index%3?0:1+rnd()%5);r[0x35]=(uint8_t)(rnd()%7);r[0x36]=(uint8_t)layers;
  putf(ram,rec+0x38,(float[]){0,0.1f,1.0f,rndf(0,0.3f)}[rnd()%4]);
  for(unsigned j=0;j<=layers;++j){uint32_t desc=DESCS+24*(5*i+j);if(j<layers){wr32(r+0x3c+8*j,desc);putf(ram,rec+0x40+8*j,rndf(-1,2));}
   else wr32(r,desc);
   wr32(ram_at(ram,desc,4),rnd()%4);for(unsigned b=1;b<6;++b)putf(ram,desc+4*b,b==1?rndf(1,80):rndf(-0.1f,1.2f));}
 }
 ram_at(ram,R13-0x6208u,1)[0]=(uint8_t)(rnd()%3);ram_at(ram,R13-0x592cu,1)[0]=(uint8_t)rnd();
 wr32(ram_at(ram,0x8030b31cu,4),index%2);
 if(index%23==0)ram_at(ram,R13-0x71c0u,1)[0]=0;
 uint32_t gx=ram32(ram,R13-0x4c90u);wr32(ram_at(ram,gx+0x420,4),rnd()%2);
 for(unsigned i=0;i<6;++i)putf(ram,gx+0x424+4*i,rndf(-2,2));
 for(unsigned i=0;i<4;++i)ram_at(ram,0x802aa450u+0x68*i+0x64,1)[0]=(uint8_t)(rnd()%2);
 uint32_t manager=ram32(ram,R13-0x61f8u),tab=ram32(ram,manager+0x14),rng=ram32(ram,tab+4);
 ram_at(ram,manager+1,1)[0]=(uint8_t[]){0,1,255}[(index/3)%3];wr32(ram_at(ram,rng+4,4),rnd());
 putf(ram,R13-0x68e8u,rndf(-1,1));putf(ram,R13-0x68e4u,rndf(-1,1));
 wr32(ram_at(ram,STATE+0x18,4),rnd());wr32(ram_at(ram,STATE+0x1c,4),rnd());
}
int main(int argc,char **argv){
 if(argc!=3)return 2;if(!gx_oracle_open(argv[2]))return 3;uint8_t *pristine=load_ram(argv[1]),*ram=malloc(RAM_BYTES),*before=malloc(RAM_BYTES);
 static nflstreet_gx_call calls[CALLS_MAX];if(!pristine||!ram||!before)return 3;
 unsigned total=0,writes=0,quads=0,peeks=0,saved=0,begin=0;uint32_t digest=2166136261u;
 for(uint32_t index=0;index<CASES;++index){memcpy(ram,pristine,RAM_BYTES);prepare(ram,index);memcpy(before,ram,RAM_BYTES);
  trace t={calls,0};nflstreet_player_draw_context x;nflstreet_scenery_context scene=scenery_context(ram,&x,&t);nflstreet_matrix_stacks stacks=venue_stacks(ram);
  nflstreet_field_overlay_context c={&scene,&stacks,R13,depth,&index};nflstreet_render_state s;saved_state(ram,&s);
  if(!nflstreet_field_overlay_draw(&c,&s)){fprintf(stderr,"native rejected case %u\n",index);return 5;}
  memcpy(oracle_ram,before,RAM_BYTES);unsigned n=0;field_oracle_service service={depth,&index,0,0};
  if(!field_oracle_run(&t,&service,&n)||n!=t.count){fprintf(stderr,"case %u: original %u native %u calls\n",index,n,t.count);return 6;}
  if(!gx_oracle_same_ram(ram,&s)){fprintf(stderr,"RAM case %u\n",index);return 7;}
  total+=n;peeks+=service.peeks;quads+=service.quads;
  for(unsigned i=0;i<n;++i){writes+=calls[i].op==NFLSTREET_GX_WRITE;begin+=calls[i].op==NFLSTREET_GX_BEGIN;}
  if(!index)saved=n;digest=fnv(digest,ram,RAM_BYTES);digest=fnv(digest,calls,n*sizeof *calls);
 }
 if(!peeks||!begin||!quads){fprintf(stderr,"missing branch coverage\n");return 8;}
 printf("Original field overlay: %u runs of 800E5CA0; saved forced-on %u calls; %u GX calls and pipe writes (%u writes, %u quads, %u depth reads, %u updates); all RAM matches; digest %08x.\n",CASES,saved,total,writes,begin,peeks,quads,digest);return 0;}
