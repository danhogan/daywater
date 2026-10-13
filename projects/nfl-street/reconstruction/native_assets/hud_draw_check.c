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
#define SCREEN_CASES 201u
#include "check_fixture.h"
#include "hud_fixture.h"
#define SCRATCH 0x80f04000u
static const uint32_t fonts[3]={0x8091bce0u,0x80e14ee0u,0x80e196e0u};
/* kind 0: the 14 name tags as saved; 1: edited name tags; 2: one direct
   text draw with edited inputs; 3: one screen camera view (8017D16C), the
   two saved screen cameras first, then edited angles, positions and stack
   selection; 4: the in-match uis screens through 8012AB5C with no ticks,
   as saved, then with edited nodes (visibility, transforms, tints),
   images (flags, colors, positions, texture coordinates, pending ids),
   text objects (flags, positions, colors, shadows, lines) and switches. */
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
/* The two in-match uis screens 8012AB5C draws (r13-6C84 and r13-6C94,
   enabled by r13-6C98 flags 8 and 1). */
static uint32_t hud_screen(uint8_t *ram,unsigned i){return ram32(ram,i?R13-0x6c94u:R13-0x6c84u);}
/* uis screen trees: nodes and their image (class 0) and text (class 8)
   objects. */
typedef struct {uint32_t nodes[96],images[160],texts[96],text_offsets[96],text_data[96],groups[160];
 unsigned node_count,image_count,text_count,group_count;} uis_tree;
static void collect(uint8_t *ram,uint32_t root,uint32_t index,uis_tree *t,unsigned depth){
 uint32_t data=ram32(ram,root+0x10);if(!data||ram32(ram,root+0xc)||depth>16)return;
 uint32_t slot=ram32(ram,data+4)+0x14u*index,n=ram32(ram,slot);if(t->node_count<96)t->nodes[t->node_count++]=n;
 for(uint32_t g=0;g<ram32(ram,slot+4);++g){uint32_t gp=ram32(ram,ram32(ram,slot+8)+4*g);if(t->group_count<160)t->groups[t->group_count++]=ram32(ram,gp);
  for(uint32_t k=0;k<ram32(ram,gp+4);++k){uint32_t e=ram32(ram,gp+8)+8*k,cls=rd32(ram_at(ram,e,4))>>16,ptr=ram32(ram,e+4);
   if(cls==0xffffu){collect(ram,root,ptr,t,depth+1);continue;}
   if(rnd()%10==0)ram_at(ram,e+2,2)[1]=0; /* entry draw flag */
   if(!ptr)continue;uint32_t obj=data+ram32(ram,ptr);
   if(cls==0&&t->image_count<160)t->images[t->image_count++]=obj;
   if(cls==8&&t->text_count<96){t->text_offsets[t->text_count]=ptr;t->text_data[t->text_count]=data;t->texts[t->text_count++]=obj;}}}}
static void edit_screens(uint8_t *ram){
 uis_tree t;memset(&t,0,sizeof t);
 for(unsigned i=0;i<2;++i){uint32_t sc=hud_screen(ram,i);for(uint32_t r=0;r<ram32(ram,sc+0x38);++r)collect(ram,ram32(ram,sc+0x3c)+0x14u*r,0,&t,0);}
 /* Hidden nodes may be shown: their textured flag-0x10 images get no
    palette (the acquisition path is not modeled). */
 for(unsigned i=0;i<t.image_count;++i){uint8_t *o=ram_at(ram,t.images[i],0x4c);if((rd32(o)>>16&0x12u)==0x12u)memset(o+8,0xff,4);
  else if(rnd()%6==0){o[8]=0;o[9]=0;o[0xa]=0;o[0xb]=6;}} /* the cached palette (6, 0) */
 for(unsigned i=0;i<t.node_count;++i){uint8_t *n=ram_at(ram,t.nodes[i],0x5c);
  if(rd32(n)?rnd()%8==0:rnd()%3==0)wr32(n,rd32(n)?0:1);
  if(rnd()%4==0)for(unsigned k=0;k<3;++k)wr32(n+8+4*k,fbits(rd32(n+8+4*k)?rndf(-100.0f,600.0f):rndf(-20.0f,20.0f)));
  if(rnd()%6==0)for(unsigned k=0;k<3;++k)wr32(n+0x14+4*k,fbits(rndf(-50.0f,50.0f)));
  if(rnd()%5==0)for(unsigned k=0;k<3;++k)wr32(n+0x20+4*k,fbits(rnd()%3?0.0f:rndf(-360.0f,360.0f)));
  if(rnd()%5==0)for(unsigned k=0;k<3;++k)wr32(n+0x2c+4*k,fbits(rndf(-2.0f,2.0f)));
  if(rnd()%4==0)for(unsigned k=0;k<8;++k)wr32(n+0x3c+4*k,fbits(rndf(-0.5f,1.5f)));}
 for(unsigned i=0;i<t.image_count;++i){uint8_t *o=ram_at(ram,t.images[i],0x4c);uint32_t f=rd32(o)>>16;
  if(rnd()%6==0)f^=1u<<(rnd()%2?0:2);if(rnd()%8==0&&!(f&0x10))f^=2u;o[0]=(uint8_t)(f>>8);o[1]=(uint8_t)f;
  if(rnd()%4==0)for(unsigned k=0;k<4;++k)wr32(o+0x14+4*k,rnd()<<8^rnd());
  if(rnd()%4==0)for(unsigned k=0;k<12;++k){uint16_t v=(uint16_t)(rnd()%8000-2000);o[0x34+2*k]=(uint8_t)(v>>8);o[0x35+2*k]=(uint8_t)v;}
  if(rnd()%4==0)for(unsigned k=0;k<8;++k){uint16_t v=(uint16_t)(rnd()%0x300);o[0x24+2*k]=(uint8_t)(v>>8);o[0x25+2*k]=(uint8_t)v;}
  if(rnd()%5==0&&(int16_t)rd32(o+4)!=-1){if(rnd()%2){memcpy(o+0xc,o+4,2);memcpy(o+0xe,o+6,2);}else{memset(o+0xc,0xff,4);}}}
 for(unsigned i=0;i<t.text_count;++i){uint8_t *o=ram_at(ram,t.texts[i],0x28);uint32_t f=rd32(o)>>16;
  if(rnd()%3==0){static const uint16_t bits_[8]={1,2,0x10,0x20,0x40,0x80,0x100,0x200};f^=bits_[rnd()%8];}
  if((f&3)==3)f&=~2u;o[0]=(uint8_t)(f>>8);o[1]=(uint8_t)f;
  if(rnd()%4==0)for(unsigned k=0;k<3;++k){uint16_t v=(uint16_t)(rnd()%600-150);o[6+2*k]=(uint8_t)(v>>8);o[7+2*k]=(uint8_t)v;}
  if(rnd()%4==0)for(unsigned k=0;k<2;++k){uint16_t v=(uint16_t)(rnd()%200-60);o[0x14+2*k]=(uint8_t)(v>>8);o[0x15+2*k]=(uint8_t)v;}
  if(rnd()%4==0)for(unsigned k=0;k<2;++k)wr32(o+0x18+4*k,rnd()<<8^rnd());
  if(rnd()%4==0)for(unsigned k=0;k<3;++k)o[0xc+k]=(uint8_t)(rnd()%13-6);
  if(rnd()%5==0)o[0x10]=(uint8_t)(rnd()%(o[0xf]+1u));}
 for(unsigned i=0;i<t.group_count;++i)if(rnd()%4==0)wr32(ram_at(ram,t.groups[i],4),ram32(ram,t.groups[i])?0:1);
 /* A multi-line copy of a text object in scratch RAM (lines over its own
    string, wrap clipping at whole multiples of the line height), linked
    in place of the original. */
 if(t.text_count&&rnd()%2==0){unsigned i=rnd()%t.text_count;const uint8_t *o=ram_at(ram,t.texts[i],0x28);
  uint32_t end=rd32(ram_at(ram,t.texts[i]+0x24,4))&0xffffu,copy=SCRATCH+0x1000u,spans=SCRATCH+0x1100u,lines=2+rnd()%3;
  uint32_t rec=ram32(ram,ram32(ram,ram32(ram,ram32(ram,R13-0x63e0u+4)+0xc)+0x24u*(uint32_t)(int8_t)o[3]+0x20)+8u*(uint32_t)(int8_t)o[2]+4);
  if(end&&rec&&!ram32(ram,rec+4)){uint32_t lh=ram_at(ram,ram32(ram,ram32(ram,rec)+4)+0x13,1)[0];uint8_t *c=ram_at(ram,copy,0x28);memcpy(c,o,0x28);
   c[0xf]=(uint8_t)lines;c[0x10]=(uint8_t)(rnd()%2);wr32(c+0x24,spans);c[0]|=(uint8_t)(rnd()%2?2:0);c[1]=(uint8_t)(c[1]&~3u);
   uint16_t limit=(uint16_t)(lh*(1+rnd()%lines)+(rnd()%3==0?rnd()%5:0));c[0x16]=(uint8_t)(limit>>8);c[0x17]=(uint8_t)limit;
   for(unsigned k=0;k<lines;++k){uint32_t a=rnd()%(end+1),b=a+rnd()%(end-a+1);wr32(ram_at(ram,spans+4*k,4),a<<16|b);}
   wr32(ram_at(ram,t.text_offsets[i],4),copy-t.text_data[i]);}}
 if(rnd()%10==0)wr32(ram_at(ram,R13-0x6c98u,4),rnd()%16);if(rnd()%2)wr32(ram_at(ram,R13-0x4f80u,4),rnd()|1u);
 if(rnd()%4==0)wr32(ram_at(ram,0x80263b98u,4),fbits(rndf(-6.0f,12.0f)));if(rnd()%12==0)wr32(ram_at(ram,R13-0x6cd8u,4),0);
 if(rnd()%6==0)ram_at(ram,R13-0x63bcu,1)[0]=(uint8_t)(rnd()%2);if(rnd()%6==0)ram_at(ram,R13-0x6cf0u,1)[0]=(uint8_t)(rnd()%2);
 if(rnd()%10==0)ram_at(ram,R13-0x6538u,1)[0]=0;
 if(rnd()%4==0){uint32_t obj=ram32(ram,ram32(ram,ram32(ram,R13-0x63e0u+4)+0xc)+0x24+0x14);uint8_t *b=ram_at(ram,ram32(ram,obj+0x78)+13,1);b[0]^=0x10;}
 if(rnd()%4==0)for(unsigned k=0;k<8;++k)wr32(ram_at(ram,0x8034cdd8u+4*k,4),fbits(rndf(-0.5f,1.5f)));
 if(rnd()%3==0)ram_at(ram,R13-0x4ee8u,1)[0]=(uint8_t)(rnd()%2?0:3);
 if(rnd()%3==0)ram_at(ram,R13-0x6208u,1)[0]=(uint8_t)(rnd()%2);if(rnd()%2)wr32(ram_at(ram,R13-0x64bcu,4),rnd());}
static void prepare(uint8_t *ram,unsigned index,hud_case *c){
 memset(c,0,sizeof *c);if(!index)return;
 if(index<=TAG_CASES){c->kind=1;edit_tags(ram);return;}
 if(index>TAG_CASES+TEXT_CASES+CAMERA_CASES){c->kind=4;if(index>TAG_CASES+TEXT_CASES+CAMERA_CASES+1)edit_screens(ram);return;}
 if(index>TAG_CASES+TEXT_CASES){c->kind=3;edit_camera(ram,index-1-TAG_CASES-TEXT_CASES,c);return;}
 c->kind=2;edit_text(ram,c);}
static int run_native(uint8_t *ram,const hud_case *c,trace *t,nflstreet_render_state *s){
 nflstreet_player_draw_context x;nflstreet_scenery_context sc=scenery_context(ram,&x,t);nflstreet_hud_context h=hud_context(ram,&sc);
 t->count=0;saved_state(ram,s);
 if(c->kind==4){nflstreet_uis_context u=uis_context(ram,&h);return nflstreet_hud_screens_draw(&u,ram32(ram,R13-0x6cd8u),s);}
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
 totals live={0},tags={0},texts={0},cameras={0},screen_live={0},screens={0};uint32_t h=2166136261u;
 for(unsigned index=0;index<1+TAG_CASES+TEXT_CASES+CAMERA_CASES+SCREEN_CASES;++index){
  memcpy(ram,pristine,RAM_BYTES);hud_case c;prepare(ram,index,&c);memcpy(before,ram,RAM_BYTES);
  nflstreet_render_state s;if(!run_native(ram,&c,&t,&s)){fprintf(stderr,"native HUD rejected case %u (kind %u) after %u calls\n",index,c.kind,t.count);return 5;}
#ifdef NFLSTREET_HUD_ORACLE
  if(!original_case(before,&c,&t,ram,&s)){const uint8_t *text=c.text?ram_at(before,c.text,1):NULL;
   fprintf(stderr,"original HUD mismatch case %u (kind %u font %08x flags %u limit %g text length %u ready %u)\n",index,c.kind,c.font,c.flags,
    (double)c.limit,text?(unsigned)strlen((const char *)text):0u,ram_at(before,R13-0x6650u,1)[0]);return 7;}
#endif
  count(&t,c.kind==0?&live:c.kind==1?&tags:c.kind==2?&texts:c.kind==3?&cameras:
   index==1+TAG_CASES+TEXT_CASES+CAMERA_CASES?&screen_live:&screens);h=fnv(h,t.calls,t.count*sizeof *t.calls);h=fnv(h,&s,sizeof s);}
 printf("HUD draw: name tags as saved (%u GX calls, %u glyph quads, %u pipe writes); %u edited name-tag frames (%u drawing, %u GX calls, "
  "%u glyph quads, %u pipe writes); %u direct texts (%u drawing, %u GX calls, %u glyph quads, %u pipe writes); %u screen camera views; uis screens as saved (%u GX calls, %u quads, %u pipe writes) and %u edited (%u drawing, %u GX calls, "
  "%u quads, %u pipe writes); digest %08x.\n",
  live.calls,live.primitives,live.writes,tags.cases,tags.drawn,tags.calls,tags.primitives,tags.writes,
  texts.cases,texts.drawn,texts.calls,texts.primitives,texts.writes,cameras.cases,screen_live.calls,screen_live.primitives,
  screen_live.writes,screens.cases,screens.drawn,screens.calls,screens.primitives,screens.writes,h);
#ifdef NFLSTREET_HUD_ORACLE
 printf("Original HUD draw: %u runs of 800DADC0, 80186B1C, 8017D16C and 8012AB5C match every GX call and pipe write (%u), the render state and all RAM outside the oracle stack.\n",
  oracle_runs,oracle_calls);
#endif
 free(t.calls);free(before);free(ram);free(pristine);return 0;}
