/* Native field-camera node entry 800E398C (projection changes, the sky
   801655D8 and the clouds 800D5B3C -> 8015F83C -> 8015F450) against the
   original on a saved context. Each case prepares a fresh copy of the saved
   RAM as the frame does before the venue draws (the viewport's stack
   selected), runs the entry natively, then runs 800E398C in the preserved
   original module from the same RAM. Every routine executes in the original
   except the GX SDK entry points (recorded and compared call by call,
   gx_oracle.h, with GXSetProjectionv 80207D5C recorded here as its seven
   argument words, and GXGetProjectionv 80207E1C executing: it only reads
   the GX shadow) and the OS cache routines (no RAM effect). Every immediate
   write to the GX pipe is compared. All RAM outside the oracle stack and the
   render-state globals must be equal. Case 0 is the saved frame; the others
   edit the enable bytes, the cloud texture word, the record count and list,
   every record field (layer counts up to 255, sizes, heights, fades,
   alphas, positions, with values that clamp, saturate, go negative or NaN,
   and boundaries hit exactly), the period index, blend and overlay amount
   (every branch of 8015F164 and their exact thresholds), the color table,
   the constants the routines read, the GX projection, the stack top, the
   sky's color table and the render state. */
#include <windows.h>
#include <math.h>
#include "moderngekko/module_abi.h"
#include "scenery_draw.h"
#include "check_fixture.h"
#include "cloud_draw.h"
#include "gx_oracle.h"
#define CASES 401u
#define NODE_OBJECT 0x80de8e1cu /* the field camera scene node's entry 1 object */
#define BIG_CALLS 65536u
#define RECORDS 0x80385c8cu
typedef struct {nflstreet_gx_call *calls;unsigned count;} big_trace;
static int record_big(void *t,const nflstreet_gx_call *c){big_trace *r=t;if(r->count>=BIG_CALLS)return 0;r->calls[r->count++]=*c;return 1;}
typedef struct {nflstreet_player_draw_context x;nflstreet_scenery_context scene;nflstreet_matrix_stacks stacks;nflstreet_cloud_context cloud;} fixture;
static void cloud_fixture(uint8_t *ram,fixture *f,big_trace *t){
 memset(f,0,sizeof *f);f->scene=scenery_context(ram,&f->x,NULL);f->x.emit=record_big;f->x.sink=t;f->stacks=venue_stacks(ram);
 nflstreet_cloud_context *c=&f->cloud;c->scene=&f->scene;c->stacks=&f->stacks;c->enable=R13-0x71e3u;c->sky_enable=R13-0x71e4u;
 c->gx_state=ram32(ram,R13-0x4c90u);c->texture=0x802aa7a4u;c->records=RECORDS;c->count=R13-0x7284u;c->texcoords=0x8028cc80u;
 c->period=0x802a3088u;c->overlay=0x802a3068u;c->palette=0x8028cbc0u;c->sky_record=0x8030feb4u;c->projection_value=ramf(ram,0x802536ccu);
 c->blend_cap=ramf(ram,0x802606d8u);c->blend_half=ramf(ram,0x802606dcu);c->blend_floor=ramf(ram,0x802606e0u);c->color_alpha=ramf(ram,0x802606e4u);
 c->one=ramf(ram,0x802606f8u);c->zero=ramf(ram,0x802606fcu);c->half=ramf(ram,0x80260700u);c->byte_scale=ramf(ram,0x80260704u);}
static void putf(uint8_t *ram,uint32_t a,float v){wr32(ram_at(ram,a,4),fbits(v));}
static unsigned count_of(uint8_t *ram){const uint8_t *p=ram_at(ram,R13-0x7284u,2);return (unsigned)(p[0]<<8|p[1]);}
/* A float for a color or scale field: ordinary, large, negative, zero or a quiet NaN. */
static float wild(float lo,float hi){
 switch(rnd()%12){case 0:return 0.0f;case 1:return -rndf(0.0f,4.0f);case 2:return rndf(2.0f,3e9f);case 3:return nanf("");case 4:return 1e10f;default:return rndf(lo,hi);}}
static void edit_record(uint8_t *ram,uint32_t rec){
 uint8_t *r=ram_at(ram,rec,0x24);if(!r)return;
 if(rnd()%3==0)r[0]=(uint8_t)(rnd()%40==0?255:rnd()%8);
 if(rnd()%4==0)putf(ram,rec+8,rnd()%8==0?-rndf(10.0f,500.0f):rndf(50.0f,900.0f));
 if(rnd()%4==0)putf(ram,rec+0xc,rnd()%6==0?nanf(""):rndf(-40.0f,60.0f));
 if(rnd()%3==0)putf(ram,rec+0x10,wild(0.0f,1.5f));
 if(rnd()%3==0)putf(ram,rec+0x14,rnd()%6==0?rndf(300.0f,2000.0f):wild(0.0f,1.0f));
 if(rnd()%4==0){putf(ram,rec+0x18,rndf(-6000.0f,6000.0f));putf(ram,rec+0x1c,rndf(-6000.0f,6000.0f));}
 if(rnd()%4==0)putf(ram,rec+0x20,rndf(-100.0f,900.0f));
 /* alpha exactly 255 after the layer share (share = 1/layers, or 1) */
 if(rnd()%10==0){unsigned n=r[0];putf(ram,rec+0x14,n>1?255.0f*(float)n:255.0f);}
}
static void prepare(uint8_t *ram,unsigned index){
 nflstreet_matrix_stacks k=venue_stacks(ram);
 wr32(ram_at(ram,R13-0x65e8u,4),k.table+12u*ram_at(ram,R13-0x4ee8u,1)[0]);
 if(!index)return;
 if(rnd()%20==0)ram_at(ram,R13-0x71e3u,1)[0]=0;if(rnd()%20==0)ram_at(ram,R13-0x71e4u,1)[0]=0;
 if(rnd()%30==0)ram_at(ram,R13-0x71e3u,1)[0]=(uint8_t)(rnd()|1);
 if(rnd()%20==0)wr32(ram_at(ram,0x802aa7a4u,4),0);
 if(rnd()%6==0){unsigned n=rnd()%5==0?rnd()%4:rnd()%85;uint8_t *p=ram_at(ram,R13-0x7284u,2);p[0]=(uint8_t)(n>>8);p[1]=(uint8_t)n;}
 if(rnd()%8==0)for(unsigned i=0;i<4;++i){unsigned a=rnd()%80,b=rnd()%80;wr32(ram_at(ram,RECORDS+4*a,4),ram32(ram,RECORDS+4*b));}
 unsigned n=count_of(ram),edits=rnd()%4==0?n:rnd()%6;
 for(unsigned i=0;i<edits&&n;++i)edit_record(ram,ram32(ram,RECORDS+4*(rnd()%n)));
 /* the period index, its blend and the overlay, with exact thresholds */
 if(rnd()%4)wr32(ram_at(ram,0x802a3088u,4),(uint32_t[]){0,1,2,3,0,2,5,0xffffffffu}[rnd()%8]);
 if(rnd()%3)putf(ram,0x802a308cu,(float[]){rndf(0.0f,1.0f),0.5f,0.25f,0.75f,1.0f,rndf(1.0f,2.0f),-0.25f,0.0f,nanf("")}[rnd()%9]);
 if(rnd()%2)putf(ram,0x802a3068u,(float[]){0.0f,rndf(0.0f,1.0f),0.5f,1.0f,0.25f,-0.5f,nanf(""),rndf(0.5f,1.0f)}[rnd()%8]);
 if(rnd()%3==0)for(unsigned i=0;i<48;++i)if(rnd()%3==0)putf(ram,0x8028cbc0u+4*i,wild(0.0f,1.0f));
 if(rnd()%12==0)putf(ram,0x802606d8u,rndf(0.5f,1.5f));if(rnd()%12==0)putf(ram,0x802606dcu,rndf(0.25f,0.75f));
 if(rnd()%12==0)putf(ram,0x802606e0u,rndf(-0.2f,0.2f));if(rnd()%12==0)putf(ram,0x802606e4u,rndf(0.0f,1.0f));
 if(rnd()%12==0)putf(ram,0x802606f8u,rndf(0.5f,2.0f));if(rnd()%12==0)putf(ram,0x802606fcu,rndf(-0.3f,0.3f));
 if(rnd()%12==0)putf(ram,0x80260700u,rndf(0.2f,1.0f));if(rnd()%12==0)putf(ram,0x80260704u,rndf(100.0f,400.0f));
 if(rnd()%10==0)putf(ram,0x802536ccu,rndf(-1.0f,1.0f));
 /* the GX projection (type and parameters) */
 if(rnd()%4==0){uint32_t gx=ram32(ram,R13-0x4c90u);wr32(ram_at(ram,gx+0x420,4),(uint32_t[]){0,1,7,0x80000001u}[rnd()%4]);
  for(unsigned i=0;i<6;++i)if(rnd()%2)putf(ram,gx+0x424+4*i,rndf(-3.0f,3.0f));}
 if(rnd()%3==0){uint32_t s=ram32(ram,R13-0x65e8u),top=ram32(ram,s+8)+0x40u*ram32(ram,s+4);
  for(unsigned i=0;i<12;++i)if(rnd()%3==0)putf(ram,top+4*i,ramf(ram,top+4*i)+rndf(-0.5f,0.5f));}
 if(rnd()%5==0)for(unsigned i=0;i<16;++i)if(rnd()%3==0)putf(ram,0x8038668cu+16*(rnd()%16)+4*(rnd()%3),rndf(0.0f,1.2f));
 if(rnd()%5==0)wr32(ram_at(ram,STATE+0x1c,4),rnd()%7);if(rnd()%5==0)wr32(ram_at(ram,STATE+0x18,4),0);
 /* a fade reaching exactly zero at the second layer (period 1, no overlay, fade word 1) */
 if(n&&rnd()%8==0){uint32_t rec=ram32(ram,RECORDS+4*(rnd()%n));uint8_t *r=ram_at(ram,rec,1);if(r){r[0]=4;putf(ram,rec+0x10,4.0f);}
  wr32(ram_at(ram,0x802a3088u,4),1);putf(ram,0x802a3068u,0.0f);putf(ram,0x8028cbf8u,1.0f);putf(ram,0x802606f8u,1.0f);}}
static int service(CPUState *c,void *p){(void)p;
 if(c->pc==0x80207d5cu){const uint8_t *v=ram_at(oracle_ram,c->gpr[3],28);if(!v)return -1;
  nflstreet_gx_call g;memset(&g,0,sizeof g);g.op=NFLSTREET_GX_SET_PROJECTION_V;for(unsigned i=0;i<7;++i)g.data[i]=rd32(v+4*i);
  if(pipe.n>=pipe.t->count||memcmp(&g,pipe.t->calls+pipe.n,sizeof g)){fprintf(stderr,"GX call %u (GXSetProjectionv) differs\n",pipe.n);
   print_call("original",&g);if(pipe.n<pipe.t->count)print_call("native",pipe.t->calls+pipe.n);return -1;}
  ++pipe.n;c->pc=c->lr;return 1;}
 if(c->pc>=0x80207e1cu&&c->pc<0x80207e7cu){ /* GXGetProjectionv executes */
  while(c->pc>=0x80207e1cu&&c->pc<0x80207e7cu){c->downcount=-1000000000;gx_module->dispatch(c,c->pc);if(c->exception)return -1;}return 1;}
 return 0;}
int main(int argc,char **argv){
 if(argc!=3)return 2;if(!gx_oracle_open(argv[2]))return 3;
 uint8_t *pristine=load_ram(argv[1]),*ram=malloc(RAM_BYTES),*before=malloc(RAM_BYTES);static nflstreet_gx_call calls[BIG_CALLS];
 if(!pristine||!ram||!before)return 3;
 if(!venue_present(pristine)||count_of(pristine)!=80||!ram32(pristine,0x802aa7a4u)){fprintf(stderr,"the saved RAM lacks the venue clouds\n");return 4;}
 unsigned saved_calls=0,saved_writes=0,total=0,writes=0,quads=0,drawn=0,projections=0;uint32_t digest=2166136261u;
 for(unsigned index=0;index<CASES;++index){
  memcpy(ram,pristine,RAM_BYTES);prepare(ram,index);memcpy(before,ram,RAM_BYTES);
  big_trace t={calls,0};nflstreet_render_state s;static fixture f;cloud_fixture(ram,&f,&t);saved_state(ram,&s);
  if(!nflstreet_sky_node_draw(&f.cloud,&s)){fprintf(stderr,"native case %u rejected\n",index);return 5;}
  memcpy(oracle_ram,before,RAM_BYTES);const uint32_t args[6]={NODE_OBJECT,0,0,0,0,0};unsigned n=0;
  const trace rest={t.calls,t.count};
  if(!gx_oracle_run(0x800e398cu,args,&rest,service,NULL,&n)){fprintf(stderr,"case %u\n",index);return 6;}
  if(n!=t.count){fprintf(stderr,"case %u: original issued %u GX calls and writes, native %u\n",index,n,t.count);return 7;}
  if(!gx_oracle_same_ram(ram,&s)){fprintf(stderr,"original cloud mismatch case %u\n",index);return 7;}
  for(unsigned i=0;i<n;++i){if(calls[i].op==NFLSTREET_GX_WRITE)++writes;
   else if(calls[i].op==NFLSTREET_GX_BEGIN&&calls[i].arg[0]==0x80&&calls[i].arg[1]==0&&i+1<n&&calls[i+1].op==NFLSTREET_GX_WRITE&&calls[i+1].arg[0]==4)++quads;
   else if(calls[i].op==NFLSTREET_GX_SET_PROJECTION_V)++projections;
   if(calls[i].op==NFLSTREET_GX_SET_ALPHA_COMPARE)++drawn;}
  if(!index){saved_calls=n;for(unsigned i=0;i<n;++i)saved_writes+=calls[i].op==NFLSTREET_GX_WRITE;}
  total+=n;digest=fnv(digest,ram,RAM_BYTES);digest=fnv(digest,calls,sizeof *calls*n);}
 printf("Original sky node: saved frame %u GX calls and pipe writes (%u pipe writes); %u runs of 800E398C match %u GX calls and pipe writes (%u pipe writes, %u projection changes), all RAM; "
  "%u cloud passes drew %u cloud strips; digest %08x.\n",saved_calls,saved_writes,CASES,total,writes,projections,drawn,quads,digest);
 return 0;}
