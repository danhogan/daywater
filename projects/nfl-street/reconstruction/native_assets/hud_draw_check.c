/* Native HUD text draw (80186B1C) and name tags (800DADC0) on a saved
   context. The saved RAM is fixture input; the name tags are the 14 objects
   the original frame passes to 800DADC0 (located by running the original
   scene traversal). Controlled cases edit a fresh copy: tag visibility,
   position, text and font, the tag and option switches, play-state gates,
   fog, the text system flag, matrix stacks and profile buckets; and for
   direct text runs the font, text (newlines, unknown and high codes, the
   0x162-byte limit), alignment flags, wrap limit (including whole numbers a pen
   position can equal), position, text state and the last error code.
   Glyph metrics are edited too (negative advances give negative lines).
   Strings and positions for edits are written to scratch RAM (80F04000). */
#include "hud_draw.h"
#define TAG_CASES 200u
#define TEXT_CASES 400u
#define CAMERA_CASES 400u
#include "check_fixture.h"
#include "hud_fixture.h"
#define SCRATCH 0x80f04000u
static const uint32_t fonts[3]={0x8091bce0u,0x80e14ee0u,0x80e196e0u};
/* kind 0: the 14 name tags as saved; 1: edited name tags; 2: one direct
   text draw with edited inputs; 3: one screen camera view (8017D16C), the
   two saved screen cameras first, then edited angles, positions and stack
   selection. */
typedef struct {unsigned kind;uint32_t font,text,flags,position,camera;float limit;} hud_case;
static uint32_t random_text(uint8_t *ram,uint32_t at){
 static const char letters[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 .,'-!?:";
 unsigned n=rnd()%20==0?0x160u+rnd()%6:rnd()%40;uint8_t *p=ram_at(ram,at,n+1);
 for(unsigned i=0;i<n;++i){unsigned r=rnd()%40;p[i]=(uint8_t)(r==0?10:r==1?0x80+rnd()%0x80:r==2?1+rnd()%31:letters[rnd()%(sizeof letters-1)]);}
 p[n]=0;return at;}
static void random_vector(uint8_t *ram,uint32_t at,float x,float y){
 wr32(ram_at(ram,at,4),fbits(rndf(-x,x)));wr32(ram_at(ram,at+4,4),fbits(rndf(-y,y)));wr32(ram_at(ram,at+8,4),fbits(rndf(-1.0f,1.0f)));}
/* Shared edits: switches, gates, fog, text system, stacks and profile. */
static void edit_common(uint8_t *ram){
 if(rnd()%12==0)ram_at(ram,R13-0x6650u,1)[0]=0;if(rnd()%2)wr32(ram_at(ram,R13-0x64bcu,4),rnd());
 if(rnd()%3==0)ram_at(ram,R13-0x4ee8u,1)[0]=(uint8_t)(rnd()%2?0:3);
 if(rnd()%3==0)wr32(ram_at(ram,R13-0x65e8u,4),ram32(ram,R13-0x65f0u)+12u*(rnd()%4));
 uint32_t stack=ram32(ram,R13-0x65f0u)+12u*ram_at(ram,R13-0x4ee8u,1)[0],top=ram32(ram,stack+8)+0x40u*ram32(ram,stack+4);
 if(rnd()%3==0)for(unsigned i=0;i<12;++i)if(rnd()%2)wr32(ram_at(ram,top+4*i,4),fbits(rndf(-600.0f,600.0f)));
 if(rnd()%4==0){uint32_t base=ram32(ram,R13-0x6608u);wr32(ram_at(ram,R13-0x6618u,4),rnd()%4==0?0:base+0x7cu*(rnd()%16));}
 if(rnd()%3==0)ram_at(ram,R13-0x6208u,1)[0]=(uint8_t)(rnd()%2);
 if(rnd()%3==0){uint8_t *t=ram_at(ram,0x8039c480u,0x30);wr32(t,rnd()&0x7fu);
  for(unsigned k=0;k<8;++k)if(rnd()%2)wr32(t+4+4*k,fbits(rndf(-0.2f,1.2f)));for(unsigned k=0;k<3;++k)if(rnd()%2)wr32(t+0x24+4*k,fbits(rndf(-6.0f,6.0f)));}}
static void edit_tags(uint8_t *ram){
 for(unsigned i=0;i<HUD_NAME_TAG_COUNT;++i){uint8_t *t=ram_at(ram,hud_name_tag(i),0x24);
  if(rnd()%2)t[0x1c]=(uint8_t)(rnd()%2);
  if(rnd()%2){wr32(t+4,fbits(rndf(0.0f,640.0f)));wr32(t+8,fbits(rndf(0.0f,480.0f)));wr32(t+0xc,fbits(rndf(-1.0f,1.0f)));}
  if(rnd()%3==0)wr32(t+0x20,fbits(rndf(-2.0f,2.0f)));
  if(rnd()%4==0)wr32(t+0x18,rnd()%2?ram32(ram,hud_name_tag(rnd()%HUD_NAME_TAG_COUNT)+0x18):random_text(ram,SCRATCH+0x200u*i));
  if(rnd()%5==0)wr32(t+0x14,rnd()%4==0?0:fonts[rnd()%3]);}
 if(rnd()%10==0)ram_at(ram,R13-0x7240u,1)[0]=0;if(rnd()%4==0)ram_at(ram,R13-0x723fu,1)[0]=(uint8_t)(rnd()%2);
 if(rnd()%10==0)wr32(ram_at(ram,0x8030b2acu+0x120u,4),0);
 if(rnd()%6==0){ram_at(ram,R13-0x6334u,1)[0]=(uint8_t)(rnd()%2);wr32(ram_at(ram,R13-0x6ff0u,4),rnd()%8);}
 if(rnd()%10==0){uint8_t *p=ram_at(ram,ram32(ram,R13-0x7f40u)+0xb60,8);if(rnd()%2)wr32(p,4);else wr32(p+4,rd32(p+4)^(0x80u<<(rnd()%4*4)));}
 if(rnd()%8==0)wr32(ram_at(ram,ram32(ram,R13-0x7cf8u),4),rnd()%3==0?4u:rnd()%2?7u:rnd()%10);
 if(rnd()%10==0)wr32(ram_at(ram,0x80308560u,4),rnd()%3-1u);
 edit_common(ram);}
static void edit_text(uint8_t *ram,hud_case *c){
 c->font=fonts[rnd()%3];uint32_t header=ram32(ram,c->font+4);
 if(rnd()%3==0)wr32(ram_at(ram,header+0xc,4),ram32(ram,header+0xc)^4u);if(rnd()%6==0)wr32(ram_at(ram,c->font+0xc,4),0);
 /* Glyph metrics: signed advances and offsets, so lines can be negative. */
 if(rnd()%4==0){uint32_t glyphs=ram32(ram,c->font+8);for(unsigned k=0;k<40;++k){uint8_t *g=ram_at(ram,glyphs+12u*(rnd()%96),12);
  g[8]=(uint8_t)(rnd()%41-30);if(rnd()%2)g[9]=(uint8_t)(rnd()%21-10);if(rnd()%2)g[10]=(uint8_t)(rnd()%21-10);}}
 c->text=random_text(ram,SCRATCH);c->flags=rnd()%8;c->position=SCRATCH+0x400u;random_vector(ram,c->position,320.0f,240.0f);
 unsigned r=rnd()%8;c->limit=r<2?0.0f:r==2?-rndf(1.0f,50.0f):r<5?(float)(int)rndf(10.0f,300.0f):rndf(10.0f,300.0f);
 edit_common(ram);}
static void edit_camera(uint8_t *ram,unsigned index,hud_case *c){
 static const uint32_t special[6]={0,0x400000u,0x800000u,0xc00000u,0xffffffu,0x7fffffu};
 c->camera=hud_cameras[1+index%2];if(index<2)return;
 uint8_t *v=ram_at(ram,ram32(ram,c->camera+0x40),0x20);
 for(unsigned k=0;k<3;++k){if(rnd()%2)wr32(v+4+4*k,fbits(rndf(-1000.0f,1000.0f)));
  if(rnd()%2)wr32(v+0x14+4*k,rnd()%4==0?special[rnd()%6]:rnd()%3==0?rnd()<<8^rnd():rnd()&0xffffffu);}
 if(rnd()%3==0)ram_at(ram,R13-0x4ee8u,1)[0]=(uint8_t)(rnd()%4);
 uint32_t stack=ram32(ram,R13-0x65f0u)+12u*ram_at(ram,R13-0x4ee8u,1)[0];if(rnd()%3==0)wr32(ram_at(ram,stack+4,4),rnd()%4);}
static void prepare(uint8_t *ram,unsigned index,hud_case *c){
 memset(c,0,sizeof *c);if(!index)return;
 if(index<=TAG_CASES){c->kind=1;edit_tags(ram);return;}
 if(index>TAG_CASES+TEXT_CASES){c->kind=3;edit_camera(ram,index-1-TAG_CASES-TEXT_CASES,c);return;}
 c->kind=2;edit_text(ram,c);}
static int run_native(uint8_t *ram,const hud_case *c,trace *t,nflstreet_render_state *s){
 nflstreet_player_draw_context x;nflstreet_scenery_context sc=scenery_context(ram,&x,t);nflstreet_hud_context h=hud_context(ram,&sc);
 t->count=0;saved_state(ram,s);
 if(c->kind==3)return nflstreet_camera_view(&h,c->camera,s);
 if(c->kind==2)return nflstreet_text_draw(&h,c->font,c->text,c->flags,c->limit,c->position,s);
 for(unsigned i=0;i<HUD_NAME_TAG_COUNT;++i)if(!nflstreet_name_tag_draw(&h,hud_name_tag(i),s))return 0;
 return 1;}
#ifdef NFLSTREET_HUD_ORACLE
static unsigned oracle_runs,oracle_calls;
static int initialize_oracle(const char *dll);
static int original_case(const uint8_t *before,const hud_case *c,const trace *t,uint8_t *after,const nflstreet_render_state *s);
#endif
typedef struct {unsigned cases,drawn,calls,primitives,writes;} totals;
static void count(const trace *t,totals *n){++n->cases;n->calls+=t->count;unsigned p=0;
 for(unsigned i=0;i<t->count;++i){p+=t->calls[i].op==NFLSTREET_GX_BEGIN;n->writes+=t->calls[i].op==NFLSTREET_GX_WRITE;}
 n->primitives+=p;n->drawn+=p!=0;}
int main(int argc,char **argv){
#ifdef NFLSTREET_HUD_ORACLE
 if(argc!=3||!initialize_oracle(argv[2]))return 2;
#else
 if(argc!=2)return 2;
#endif
 uint8_t *pristine=load_ram(argv[1]),*ram=malloc(RAM_BYTES),*before=malloc(RAM_BYTES);trace t={malloc(CALLS_MAX*sizeof(nflstreet_gx_call)),0};
 if(!pristine||!ram||!before||!t.calls)return 3;
 if(!venue_present(pristine)||!hud_present(pristine))return 4;
 for(unsigned i=0;i<3;++i)if(!hud_font_present(pristine,fonts[i]))return 4;
 totals live={0},tags={0},texts={0},cameras={0};uint32_t h=2166136261u;
 for(unsigned index=0;index<1+TAG_CASES+TEXT_CASES+CAMERA_CASES;++index){
  memcpy(ram,pristine,RAM_BYTES);hud_case c;prepare(ram,index,&c);memcpy(before,ram,RAM_BYTES);
  nflstreet_render_state s;if(!run_native(ram,&c,&t,&s)){fprintf(stderr,"native HUD rejected case %u (kind %u) after %u calls\n",index,c.kind,t.count);return 5;}
#ifdef NFLSTREET_HUD_ORACLE
  if(!original_case(before,&c,&t,ram,&s)){const uint8_t *text=c.text?ram_at(before,c.text,1):NULL;
   fprintf(stderr,"original HUD mismatch case %u (kind %u font %08x flags %u limit %g text length %u ready %u)\n",index,c.kind,c.font,c.flags,
    (double)c.limit,text?(unsigned)strlen((const char *)text):0u,ram_at(before,R13-0x6650u,1)[0]);return 7;}
#endif
  count(&t,c.kind==0?&live:c.kind==1?&tags:c.kind==2?&texts:&cameras);h=fnv(h,t.calls,t.count*sizeof *t.calls);h=fnv(h,&s,sizeof s);}
 printf("HUD draw: name tags as saved (%u GX calls, %u glyph quads, %u pipe writes); %u edited name-tag frames (%u drawing, %u GX calls, "
  "%u glyph quads, %u pipe writes); %u direct texts (%u drawing, %u GX calls, %u glyph quads, %u pipe writes); %u screen camera views; digest %08x.\n",
  live.calls,live.primitives,live.writes,tags.cases,tags.drawn,tags.calls,tags.primitives,tags.writes,
  texts.cases,texts.drawn,texts.calls,texts.primitives,texts.writes,cameras.cases,h);
#ifdef NFLSTREET_HUD_ORACLE
 printf("Original HUD draw: %u runs of 800DADC0, 80186B1C and 8017D16C match every GX call and pipe write (%u), the render state and all RAM outside the oracle stack.\n",
  oracle_runs,oracle_calls);
#endif
 free(t.calls);free(before);free(ram);free(pristine);return 0;}
