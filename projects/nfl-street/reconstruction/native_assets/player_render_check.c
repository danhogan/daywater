/* Renders the captured players of a saved context: the native player draw
   (80161E10 submission) for each actor in actor-list order, executed by the
   software GX executor into a 640x448 target. Frame state the draw does not
   set comes from the saved GX shadow (projection, viewport, color mode, konst
   selections) and from the frame start 801C769C (depth mode, texgens). The saved RAM is fixture input only. Writes the
   image and a stable digest; it is an inspection render, not a pixel claim. */
#include "gx_render.h"
#include "texture.h"
#include "scenery_draw.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define RAM_BASE 0x80000000u
#define RAM_BYTES (24u*1024u*1024u)
#define R13 0x803a9a40u
#define PLAYERS 14u
#define CALLS_MAX 16384u
#define WIDTH 640u
#define HEIGHT 448u
static uint32_t rd32(const uint8_t *p){return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static uint8_t *ram_at(uint8_t *ram,uint32_t a,uint32_t n){return a>=RAM_BASE&&(uint64_t)a-RAM_BASE+n<=RAM_BYTES?ram+(a-RAM_BASE):NULL;}
static uint32_t ram32(uint8_t *ram,uint32_t a){const uint8_t *p=ram_at(ram,a,4);return p?rd32(p):0;}
static float ramf(uint8_t *ram,uint32_t a){uint32_t v=ram32(ram,a);float f;memcpy(&f,&v,4);return f;}
#include "venue_fixture.h"
#include "hud_fixture.h"
static uint32_t fnv(uint32_t h,const void *p,size_t n){const uint8_t *b=p;for(size_t i=0;i<n;++i)h=(h^b[i])*16777619u;return h;}
static uint8_t *span(void *m,uint32_t token,uint32_t size){return ram_at(m,token,size);}
typedef struct {nflstreet_gx_call *calls;unsigned count;} trace;
/* Register images the GX SDK writes for the state in effect at each display
   list, encoded as the SDK functions do (8020791C cmode0, 80206F1C/80206FA4
   and 80206F60/8020700C stage-0 color/alpha environments, 80207320 alpha
   compare, 80205400 channel control): depth mode, cmode0, color env 0, alpha
   env 0, alpha compare, material color, ambient color, color and alpha channel. */
#define STATE_WORDS 9u
#define STATES_MAX 64u
typedef struct {uint32_t w[STATE_WORDS];} draw_state;
typedef struct {draw_state seen[STATES_MAX];unsigned count;uint32_t cin[4],cop[5],ain[4],aop[5],blend,alpha,mat,amb,chan,stages;} encoder;
static void encode(encoder *e,const nflstreet_gx_call *c,uint32_t zmode,uint32_t cmode_base){
 const uint32_t *a=c->arg;
 switch(c->op){
 case NFLSTREET_GX_SET_BLEND_MODE:e->blend=(a[2]&7)<<5|(a[1]&7)<<8|(a[3]&15)<<12|(a[0]==2?2u:0u)|(a[0]&1)|(a[0]==3?0x800u:0u)|(cmode_base&0x1c);break;
 case NFLSTREET_GX_SET_TEV_COLOR_IN:if(!a[0])for(unsigned k=0;k<4;++k)e->cin[k]=a[1+k];break;
 case NFLSTREET_GX_SET_TEV_ALPHA_IN:if(!a[0])for(unsigned k=0;k<4;++k)e->ain[k]=a[1+k];break;
 case NFLSTREET_GX_SET_TEV_COLOR_OP:if(!a[0])for(unsigned k=0;k<5;++k)e->cop[k]=a[1+k];break;
 case NFLSTREET_GX_SET_TEV_ALPHA_OP:if(!a[0])for(unsigned k=0;k<5;++k)e->aop[k]=a[1+k];break;
 case NFLSTREET_GX_SET_ALPHA_COMPARE:e->alpha=(a[4]&0xff)<<8|(a[1]&0xff)|(a[0]&7)<<16|a[3]<<19|a[2]<<22;break;
 case NFLSTREET_GX_SET_CHAN_MAT_COLOR:e->mat=c->data[0];break;
 case NFLSTREET_GX_SET_CHAN_AMB_COLOR:e->amb=c->data[0];break;
 case NFLSTREET_GX_SET_CHAN_CTRL:{uint32_t attn=a[6],diffuse=attn?a[5]:0,spot=attn==1||attn==0;
  e->chan=a[3]|(a[1]&0xff)<<1|a[2]<<6|diffuse<<7|(a[4]&15)<<2|(a[4]&0xf0)<<7|spot<<9|(attn!=0)<<10;break;}
 case NFLSTREET_GX_SET_NUM_TEV_STAGES:e->stages=a[0];break;
 case NFLSTREET_GX_CALL_DISPLAY_LIST:{if(e->stages!=1)break;
  draw_state d={{zmode,e->blend,e->cin[3]|e->cin[2]<<4|e->cin[1]<<8|e->cin[0]<<12|e->cop[1]<<16|(e->cop[0]&1)<<18|e->cop[3]<<19|e->cop[2]<<20|e->cop[4]<<22,
   e->ain[3]<<4|e->ain[2]<<7|e->ain[1]<<10|e->ain[0]<<13|e->aop[1]<<16|(e->aop[0]&1)<<18|e->aop[3]<<19|e->aop[2]<<20|e->aop[4]<<22,
   e->alpha,e->mat,e->amb,e->chan,e->chan}};
  unsigned k=0;while(k<e->count&&memcmp(e->seen+k,&d,sizeof d))++k;if(k==e->count&&e->count<STATES_MAX)e->seen[e->count++]=d;break;}
 default:break;}}
/* NAP1 actor draws of the 2026-10-08 original frame capture: state words
   2..6 and 8..11 (depth, cmode0, color/alpha env 0, alpha compare, material,
   ambient, channels), one TEV stage each. */
static unsigned captured_states(const char *path,draw_state *out,unsigned max){
 FILE *f=fopen(path,"rb");if(!f)return 0;uint8_t head[16];unsigned n=0;
 if(fread(head,1,16,f)!=16||memcmp(head,"NAP1",4)){fclose(f);return 0;}
 unsigned records=(unsigned)head[4]|(unsigned)head[5]<<8|(unsigned)head[6]<<16|(unsigned)head[7]<<24;uint8_t *r=malloc(3876);
 for(unsigned i=0;r&&i<records;++i){if(fread(r,1,3876,f)!=3876){n=0;break;}uint32_t w[16];
  for(unsigned k=0;k<16;++k)w[k]=(uint32_t)r[48+4*k]|(uint32_t)r[49+4*k]<<8|(uint32_t)r[50+4*k]<<16|(uint32_t)r[51+4*k]<<24;
  draw_state d={{w[2],w[3],w[4],w[5],w[6],w[8],w[9],w[10],w[11]}};
  unsigned k=0;while(k<n&&memcmp(out+k,&d,sizeof d))++k;if(k==n&&n<max)out[n++]=d;}
 free(r);fclose(f);return n;}
static int record(void *t,const nflstreet_gx_call *c){trace *r=t;if(r->count>=CALLS_MAX)return 0;r->calls[r->count++]=*c;return 1;}
static int face(void *p,uint32_t actor,int *compiled){(void)p;(void)actor;*compiled=0;return 1;}
/* Every captured slot texture (binding table 80276C6C: 14 players x 25
   slots; entry +18 texture set, +1C MMAP destination; set +14 texture
   objects, +1C palette objects) decoded twice: by the GX decoder from the
   texture object and its TLUT object, and by texture.c from the destination
   MMAP (image 0, palette 0). Returns the number of matching textures. */
static int cross_decode(uint8_t *ram,unsigned *textures,size_t *pixels){
 for(unsigned e=0;e<PLAYERS*25u;++e){uint32_t entry=0x80276c6cu+0x20u*e,set=ram32(ram,entry+0x18),dest=ram32(ram,entry+0x1c);
  if(!set||!dest||!ram32(ram,set+0x10))continue;
  uint32_t tex=ram32(ram,set+0x14),pal=ram32(ram,set+0x1c),w2=ram32(ram,tex+8),format=w2>>20&15,w=(w2&0x3ff)+1,h=(w2>>10&0x3ff)+1;
  uint32_t image=0x80000000u|(ram32(ram,tex+12)&0x1fffffu)<<5,tlut=pal?pal+4:0,entries=0,tf=0;const uint8_t *table=NULL;
  if(tlut){tf=ram32(ram,tlut)>>10&3;entries=ram32(ram,tlut+8)>>16;table=ram_at(ram,0x80000000u|(ram32(ram,tlut+4)&0x1fffffu)<<5,2*entries);}
  uint8_t *rgba=malloc((size_t)w*h*4);nflstreet_texture ref;const uint8_t *src=ram_at(ram,image,1);
  int g=rgba&&src&&nflstreet_gx_decode(src,RAM_BYTES-(image-RAM_BASE),w,h,format,table,entries,tf,rgba);
  /* The destination is relocated (801C5610): header and descriptor pointers
     are absolute. texture.c reads file offsets, so a copy is unrelocated. */
  const uint8_t *mm=ram_at(ram,dest,48);uint32_t td=mm?rd32(mm+24):0,pd=mm?rd32(mm+28):0;int r=0;
  if(td>dest&&pd>dest&&ram_at(ram,td,16)&&ram_at(ram,pd,12)){
   uint32_t io=ram32(ram,td+12),is=ram32(ram,td+8),po=ram32(ram,pd+8),ps=ram32(ram,pd+4);
   uint32_t extent=io+is>po+ps?io+is:po+ps;extent=extent>td+16?extent:td+16;extent=extent>pd+12?extent:pd+12;
   uint8_t *copy=io>dest&&po>dest?malloc(extent-dest):NULL;
   if(copy&&ram_at(ram,dest,extent-dest)){memcpy(copy,ram_at(ram,dest,1),extent-dest);
    uint32_t fix[4][2]={{24,td},{28,pd},{td-dest+12,io},{pd-dest+8,po}};
    for(unsigned k=0;k<4;++k){uint32_t v=fix[k][1]-dest;uint8_t *q=copy+fix[k][0];q[0]=(uint8_t)(v>>24);q[1]=(uint8_t)(v>>16);q[2]=(uint8_t)(v>>8);q[3]=(uint8_t)v;}
    r=nflstreet_texture_open_image(copy,extent-dest,0,0,&ref);}
   free(copy);}
  if(!g||!r||ref.width!=w||ref.height!=h){fprintf(stderr,"slot entry %u: gx %d mmap %d size %ux%u vs %ux%u format %u tlut %08x\n",e,g,r,w,h,r?ref.width:0,r?ref.height:0,format,tlut);
   if(r)nflstreet_texture_close(&ref);free(rgba);return 0;}
  for(size_t i=0;i<(size_t)w*h;++i){uint32_t v=ref.bgra[i];
   if(rgba[4*i]!=(v>>16&255)||rgba[4*i+1]!=(v>>8&255)||rgba[4*i+2]!=(v&255)||rgba[4*i+3]!=v>>24){
    fprintf(stderr,"slot entry %u pixel %zu: gx %02x%02x%02x%02x mmap %08x format %u\n",e,i,rgba[4*i],rgba[4*i+1],rgba[4*i+2],rgba[4*i+3],v,format);
    nflstreet_texture_close(&ref);free(rgba);return 0;}}
  *pixels+=(size_t)w*h;++*textures;nflstreet_texture_close(&ref);free(rgba);}
 return 1;}
/* Executes one call list, reporting the first rejected call. */
static int execute(nflstreet_gx_machine *m,const trace *t,nflstreet_gx_stats *stats,const char *what,unsigned index){
 for(unsigned k=0;k<t->count;++k)if(!nflstreet_gx_run(m,t->calls+k,1,stats)){const nflstreet_gx_call *c=t->calls+k;
  fprintf(stderr,"executor rejected %s %u call %u op %d args %08x %08x %08x %08x %08x %08x %08x\n",what,index,k,(int)c->op,
   c->arg[0],c->arg[1],c->arg[2],c->arg[3],c->arg[4],c->arg[5],c->arg[6]);return 0;}
 return 1;}
static nflstreet_player_draw_context draw_context(uint8_t *ram,trace *t){
 nflstreet_player_draw_context x;memset(&x,0,sizeof x);
 x.span=span;x.memory=ram;x.emit=record;x.sink=t;x.face=face;x.shared_model=0x803102b4u;x.player_records=0x802aaa60u;
 x.light_sets=0x8034fe9cu;x.default_texture=ram32(ram,R13-0x640cu);x.null_shader=ram32(ram,R13-0x5684u);
 x.setups[0]=0x801c9d0cu;x.setups[1]=0x801c9dacu;x.setups[2]=0x801c9dfcu;x.setups[3]=0x801c9d5cu;x.dim[0]=ramf(ram,R13-0x6850u);x.dim[1]=ramf(ram,R13-0x684cu);
 return x;}
/* The 14 captured players in actor-list order. */
static int draw_players(uint8_t *ram,nflstreet_gx_machine *m,trace *t,nflstreet_render_state *s,nflstreet_gx_stats *stats,encoder *enc,
 const nflstreet_gx_frame *frame,unsigned *calls){
 nflstreet_player_draw_context x=draw_context(ram,t);
 for(unsigned i=0;i<PLAYERS;++i){uint32_t actor=ram32(ram,ram32(ram,R13-0x7bbcu)+4*i);t->count=0;
  if(!nflstreet_player_draw(&x,actor,s)){fprintf(stderr,"draw %u rejected\n",i);return 0;}
  if(!execute(m,t,stats,"actor",i))return 0;
  if(enc)for(unsigned k=0;k<t->count;++k)encode(enc,t->calls+k,frame->zmode,frame->cmode0);
  *calls+=t->count;}
 return 1;}
static void clear_target(uint32_t *bgra,uint32_t *depth){for(unsigned i=0;i<WIDTH*HEIGHT;++i){bgra[i]=0xff404040u;depth[i]=0xffffffu;}}
static void reset_state(uint8_t *ram,nflstreet_render_state *s){memset(s,0,sizeof *s);for(unsigned i=0;i<6;++i)s->viewport[i]=ramf(ram,0x8039e584u+4*i);}
int main(int argc,char **argv){
 if(argc!=4&&argc!=5)return 2;
 FILE *f=fopen(argv[1],"rb");if(!f)return 3;uint8_t *ram=malloc(RAM_BYTES),*pristine=malloc(RAM_BYTES);
 if(!ram||!pristine||fread(pristine,1,RAM_BYTES,f)!=RAM_BYTES){fclose(f);return 3;}fclose(f);memcpy(ram,pristine,RAM_BYTES);
 uint32_t gx=ram32(ram,R13-0x4c90u);nflstreet_gx_frame frame;memset(&frame,0,sizeof frame);
 frame.projection_type=ram32(ram,gx+0x420);for(unsigned i=0;i<6;++i){frame.projection[i]=ramf(ram,gx+0x424+4*i);frame.viewport[i]=ramf(ram,gx+0x43c+4*i);}
 /* The shadow holds whatever the scene set last (context 15 ends with depth
    off); depth and texgens come from the frame start 801C769C instead:
    GXSetZMode(1, LEQUAL, 1) and every coordinate MTX2x4 from TEX0, identity. */
 frame.zmode=0x17;frame.cmode0=ram32(ram,gx+0x1d0);for(unsigned i=0;i<8;++i)frame.ksel[i]=ram32(ram,gx+0x1b0+4*i);
 for(unsigned i=0;i<8;++i){frame.texgen_src[i]=4;frame.texgen_mtx[i]=60;}
 uint32_t *bgra=malloc(WIDTH*HEIGHT*4),*depth=malloc(WIDTH*HEIGHT*4);trace t={malloc(CALLS_MAX*sizeof(nflstreet_gx_call)),0};
 if(!bgra||!depth||!t.calls)return 3;
 clear_target(bgra,depth);nflstreet_gx_target target={WIDTH,HEIGHT,bgra,depth};
 nflstreet_gx_machine *m=nflstreet_gx_open(span,ram,&frame,&target);if(!m)return 4;
 nflstreet_render_state s;reset_state(ram,&s);
 nflstreet_gx_stats stats;memset(&stats,0,sizeof stats);unsigned calls=0;encoder enc;memset(&enc,0,sizeof enc);
 if(!draw_players(ram,m,&t,&s,&stats,&enc,&frame,&calls))return 5;
 nflstreet_gx_close(m);
 size_t covered=0;for(unsigned i=0;i<WIDTH*HEIGHT;++i)covered+=depth[i]!=0xffffffu;
 nflstreet_texture image={WIDTH,HEIGHT,bgra};if(!nflstreet_texture_bmp(&image,argv[2]))return 7;
 printf("Player render: %u actors, %u GX calls, %u display lists, %u primitives, %u triangles (%u culled, %u clipped), "
  "%u decoded textures, %zu shaded pixels, %zu covered; digest %08x.\n",PLAYERS,calls,stats.display_lists,stats.primitives,
  stats.triangles,stats.culled,stats.clipped,stats.textures,stats.pixels,covered,fnv(2166136261u,bgra,WIDTH*HEIGHT*4));
 /* Scene: the original frame order (field passes 8015B670, main field
    80165DC4, then the players, with the sky 801655D8 first and the actor
    shadows 801643F4, field lines 800DA074 and player-select rings 800E3F7C after
    the passes) from a fresh copy of the saved RAM into
    one target with shared depth. Marks, ball and crowd are not drawn. */
 memcpy(ram,pristine,RAM_BYTES);clear_target(bgra,depth);reset_state(ram,&s);
 if(!venue_present(ram))return 11;
 m=nflstreet_gx_open(span,ram,&frame,&target);if(!m)return 4;
 nflstreet_gx_stats scene;memset(&scene,0,sizeof scene);unsigned venue_calls=0,player_calls=0;
 nflstreet_player_draw_context x=draw_context(ram,&t);nflstreet_scenery_context v=venue_context(ram,&x);
 /* The saved matrix stack already holds the next frame's camera; the drawn
    frame's view is the stored main-field matrix without the venue
    translation (80165DC4: view * T(object +4)). */
 {const uint8_t *mv=ram_at(ram,VENUE_MAIN_FIELD+0x24+8,0x30),*pos=ram_at(ram,VENUE_MAIN_FIELD+4,12);uint32_t stack=v.matrix_stack;
  uint8_t *top=ram_at(ram,ram32(ram,stack+8)+0x40u*ram32(ram,stack+4),0x30);if(!mv||!pos||!top)return 13;
  float m[12],p3[3];for(unsigned i=0;i<12;++i){uint32_t w=rd32(mv+4*i);memcpy(m+i,&w,4);}for(unsigned i=0;i<3;++i){uint32_t w=rd32(pos+4*i);memcpy(p3+i,&w,4);}
  for(unsigned r=0;r<3;++r){m[4*r+3]-=m[4*r]*p3[0]+m[4*r+1]*p3[1]+m[4*r+2]*p3[2];}
  for(unsigned i=0;i<12;++i){uint32_t w;memcpy(&w,m+i,4);top[4*i]=(uint8_t)(w>>24);top[4*i+1]=(uint8_t)(w>>16);top[4*i+2]=(uint8_t)(w>>8);top[4*i+3]=(uint8_t)w;}}
 t.count=0;if(!nflstreet_sky_draw(&v,&s)||!execute(m,&t,&scene,"sky",0))return 12;venue_calls+=t.count;
 t.count=0;if(!nflstreet_field_passes_draw(&v,VENUE_FIELD_PASSES,&s)||!execute(m,&t,&scene,"field passes",0))return 12;venue_calls+=t.count;
 t.count=0;if(!nflstreet_shadows_draw(&v,&s)||!execute(m,&t,&scene,"shadows",0))return 12;venue_calls+=t.count;
 for(unsigned i=0;i<4;++i){t.count=0;if(!nflstreet_line_draw(&v,venue_lines[i],&s)||!execute(m,&t,&scene,"line",i))return 12;venue_calls+=t.count;}
 for(unsigned i=0;i<3;++i){t.count=0;if(!nflstreet_ring_draw(&v,venue_rings[i],&s)||!execute(m,&t,&scene,"ring",i))return 12;venue_calls+=t.count;}
 t.count=0;if(!nflstreet_field_draw(&v,VENUE_MAIN_FIELD,&s)||!execute(m,&t,&scene,"main field",0))return 12;venue_calls+=t.count;
 if(!draw_players(ram,m,&t,&s,&scene,NULL,&frame,&player_calls))return 5;
 /* HUD: the HUD camera's screen view (8017D16C; the saved stack holds the
    last camera's), then the name tags (800DADC0) over the scene. */
 if(!hud_present(ram))return 11;
 nflstreet_hud_context hud=hud_context(ram,&v);unsigned hud_calls=0;
 t.count=0;if(!nflstreet_camera_view(&hud,HUD_CAMERA,&s)||t.count)return 12;
 for(unsigned i=0;i<HUD_NAME_TAG_COUNT;++i){t.count=0;if(!nflstreet_name_tag_draw(&hud,hud_name_tag(i),&s)||!execute(m,&t,&scene,"name tag",i))return 12;hud_calls+=t.count;}
 nflstreet_gx_close(m);
 image.bgra=bgra;if(!nflstreet_texture_bmp(&image,argv[3]))return 7;
 printf("Scene render: sky, EARS field passes, shadows, lines, rings and main field (%u GX calls) then 14 actors (%u GX calls) and name tags (%u GX calls); %u display lists, %u triangles "
  "(%u culled, %u clipped), %u decoded textures, %zu shaded pixels; digest %08x.\n",venue_calls,player_calls,hud_calls,scene.display_lists,
  scene.triangles,scene.culled,scene.clipped,scene.textures,scene.pixels,fnv(2166136261u,bgra,WIDTH*HEIGHT*4));
 memcpy(ram,pristine,RAM_BYTES);
 unsigned decoded=0;size_t decoded_pixels=0;
 if(!cross_decode(ram,&decoded,&decoded_pixels)){fprintf(stderr,"GX and MMAP texture decodes differ\n");return 10;}
 printf("Texture decode: %u captured slot textures (%zu pixels) decode identically from their GX texture/TLUT objects and their MMAP files.\n",
  decoded,decoded_pixels);
 if(argc==5){draw_state captured[STATES_MAX];unsigned n=captured_states(argv[4],captured,STATES_MAX),produced=0,known=0;if(!n)return 8;
  for(unsigned i=0;i<n;++i)for(unsigned k=0;k<enc.count;++k)if(!memcmp(captured+i,enc.seen+k,sizeof *captured)){++produced;break;}
  for(unsigned k=0;k<enc.count;++k)for(unsigned i=0;i<n;++i)if(!memcmp(captured+i,enc.seen+k,sizeof *captured)){++known;break;}
  for(unsigned k=0;k<enc.count&&known!=enc.count;++k){const uint32_t *w=enc.seen[k].w;
   fprintf(stderr,"native state %08x %08x %08x %08x %08x %08x %08x %08x %08x\n",w[0],w[1],w[2],w[3],w[4],w[5],w[6],w[7],w[8]);}
  printf("Captured GPU state: %u/%u recorded one-stage player draw states are produced by the native calls; %u/%u native one-stage states were recorded.\n",
   produced,n,known,enc.count);
  if(produced!=n||known!=enc.count)return 9;}
 free(t.calls);free(depth);free(bgra);free(pristine);free(ram);return 0;}
