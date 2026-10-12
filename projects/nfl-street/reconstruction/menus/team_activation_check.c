#include <windows.h>
#include "moderngekko/module_abi.h"
#include "team_activation.h"
#include "team_formation.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM_SIZE (24u*1024u*1024u)
#define R13 0x803a9a40u
#define ROSTER_BASE 0x802f50d4u
#define LAST_PLAYER 0x802f51c4u
#define FORM_SOURCE 0x802a4430u
#define FORM_DEST 0x802a43c0u
#define POSE_BASE 0x8028271cu
#define SELECTED_AT 0x817d6000u
#define STACK 0x817df800u
#define NONE 0x7fffu
#define WEIGHTED_ROWS 0x817d0000u

static uint32_t be(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void put(uint8_t*p,uint32_t v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static uint8_t*load_file(const char*path,size_t*size,size_t max){FILE*f=fopen(path,"rb");if(!f)return NULL;if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}long z=ftell(f);if(z<1||(size_t)z>max||fseek(f,0,SEEK_SET)){fclose(f);return NULL;}uint8_t*b=malloc((size_t)z);if(!b){fclose(f);return NULL;}if(fread(b,1,(size_t)z,f)!=(size_t)z){free(b);fclose(f);return NULL;}fclose(f);*size=(size_t)z;return b;}
static uint8_t*ptr(uint8_t*ram,uint32_t address){if(address<0x80000000||address-0x80000000>=RAM_SIZE)return NULL;return ram+address-0x80000000;}
static uint32_t r13addr(int32_t offset){return R13+(uint32_t)offset;}
static uint32_t raw_stride(unsigned side){return ROSTER_BASE+side*0xf4u;}
static uint32_t form_side(unsigned side){return FORM_SOURCE+side*56u;}
static uint32_t form_out_side(unsigned side){return FORM_DEST+side*56u;}
static uint32_t pose_side(unsigned side){return POSE_BASE+side*35u*4u;}

typedef struct {uint32_t kind,index,words[5];} captured_effect;
typedef struct {
 const nflstreet_team_activation_side*input;
 unsigned side,builder_calls,effect_count,rebuild_expected;
 captured_effect effects[24];
} hooks;
static void event(hooks*h,uint32_t kind,uint32_t index,const uint32_t*w){if(h->effect_count>=24)return;captured_effect*e=&h->effects[h->effect_count++];memset(e,0,sizeof *e);e->kind=kind;e->index=index;if(w)memcpy(e->words,w,20);}
static int run_original(const ModernGekkoModuleDesc*m,CPUState*c,hooks*h){
 c->exception=0;c->downcount=c->cycle_budget=10000000;
 for(unsigned step=0;c->pc&&step<1000000;++step){uint32_t pc=c->pc;
  if(pc==0x80042568){
   if(c->gpr[3]!=h->input->formation_value||!c->gpr[4]||c->gpr[5]){fprintf(stderr,"80042568 args mode/list/output %08x/%08x/%08x\n",c->gpr[3],c->gpr[4],c->gpr[5]);return 0;}
   uint8_t*list=ptr(c->ram,c->gpr[4]);if(!list)return 0;uint32_t ids[21];unsigned count=0;
   for(;count<20;++count){ids[count]=be(list+count*4);if(ids[count]==NONE)break;}
   if(count>=20||be(list+count*4)!=NONE)return 0;
   if((h->rebuild_expected&&h->builder_calls==0&&count!=h->input->count)||count<7||count>h->input->weighted_count){fprintf(stderr,"80042568 input list count%u call%u\n",count,h->builder_calls);return 0;}
   if(!ptr(c->ram,WEIGHTED_ROWS))return 0;uint8_t*out=ptr(c->ram,WEIGHTED_ROWS);put(ptr(c->ram,r13addr(-0x7bb0)),WEIGHTED_ROWS);put(ptr(c->ram,r13addr(-0x50b0)),count);
   for(unsigned i=0;i<count;++i){const nflstreet_team_lineup_player*w=NULL;for(unsigned j=0;j<h->input->weighted_count;++j)if(h->input->weighted_rows[j].player_id==ids[i]){w=&h->input->weighted_rows[j];break;}if(!w){fprintf(stderr,"No supplied weighted row for ID%u\n",ids[i]);return 0;}
    uint8_t*r=out+i*0x44;put(r,w->player_id);for(unsigned k=0;k<14;++k)put(r+4+k*4,w->scores[k]);r[0x3c]=w->flags;put(r+0x40,w->role);}
   if(h->rebuild_expected&&h->builder_calls==0){uint32_t w[5]={h->side,count,h->input->weighted_count,0,0};event(h,NFLSTREET_TEAM_ACTIVATION_REBUILD,h->side,w);}
   else {uint32_t w[5]={h->side,h->input->formation_value,h->input->count,count,h->input->weighted_count};event(h,NFLSTREET_TEAM_ACTIVATION_BUILD,h->side,w);}
   ++h->builder_calls;c->pc=c->lr;
  }else if(pc==0x8003f6b8&&h->input->dirty&&!h->input->rebuild&&!h->input->weighted_count){
   if(c->gpr[3]!=h->side||c->gpr[5]!=h->input->formation_value)return 0;uint8_t*list=ptr(c->ram,c->gpr[4]);if(!list)return 0;unsigned j=0;
   for(unsigned i=0;i<h->input->count;++i)if(h->input->roster[i].selected>=0){if(be(list+j*4)!=h->input->roster[i].id)return 0;++j;}
   if(be(list+j*4)!=NONE)return 0;uint32_t w[5]={h->side,h->input->formation_value,h->input->count,j,0};event(h,NFLSTREET_TEAM_ACTIVATION_BUILD,h->side,w);
   uint8_t*dst=ptr(c->ram,form_side(h->side));for(unsigned i=0;i<14;++i)put(dst+i*4,h->input->formation_words[i]);c->pc=c->lr;
  }else if(pc==0x80042514){
   uint8_t*list=ptr(c->ram,c->gpr[3]);if(!list)return 0;uint8_t*rows=ptr(c->ram,WEIGHTED_ROWS);uint32_t count=be(ptr(c->ram,r13addr(-0x50b0)));
   for(unsigned i=0;i<count;++i){int found=0;for(unsigned j=0;j<21;++j){uint32_t id=be(list+j*4);if(id==NONE)break;if(id==be(rows+i*0x44)){found=1;break;}}rows[i*0x44+0x3c]=found?0:1;}c->pc=c->lr;
  }else if(pc==0x8010b60c){c->gpr[3]=6;c->pc=c->lr;
  }else if(pc==0x80042748){c->pc=c->lr;
  }else if(pc==0x80128cf4){
   unsigned index=c->gpr[3];
   if(index>=14||index/7u!=h->side){fprintf(stderr,"Position index mismatch regs=%08x/%08x/%08x/%08x/%08x/%08x/%08x/%08x fprs=%.8g/%.8g/%.8g side%u\n",c->gpr[3],c->gpr[4],c->gpr[5],c->gpr[6],c->gpr[7],c->gpr[8],c->gpr[9],c->gpr[10],c->fpr[1],c->fpr[2],c->fpr[3],h->side);return 0;}unsigned k=index%7u;const nflstreet_team_activation_pose*p=&h->input->poses[k];
   float f[3];for(unsigned i=0;i<3;++i)memcpy(&f[i],&p->position[i],4);
   if(c->fpr[1]!=(double)f[0]||c->fpr[2]!=(double)f[1]||c->fpr[3]!=(double)f[2]){fprintf(stderr,"Position float args differ side%u slot%u\n",h->side,k);return 0;}
   uint32_t w[5]={p->position[0],p->position[1],p->position[2],p->rotation,p->visibility};event(h,NFLSTREET_TEAM_ACTIVATION_POSITION,index,w);c->pc=c->lr;
  }else if(pc==0x80128e44){unsigned index=c->gpr[3];if(index>=14||index/7u!=h->side){fprintf(stderr,"Rotation index mismatch gpr3=%u side%u\n",index,h->side);return 0;}const nflstreet_team_activation_pose*p=&h->input->poses[index%7u];float f;memcpy(&f,&p->rotation,4);if(c->fpr[1]!=(double)f){fprintf(stderr,"Rotation float mismatch index%u fpr%.9g expected%.9g\n",index,c->fpr[1],(double)f);return 0;}uint32_t w[5]={p->position[0],p->position[1],p->position[2],p->rotation,p->visibility};event(h,NFLSTREET_TEAM_ACTIVATION_ROTATION,index,w);c->pc=c->lr;
  }else if(pc==0x80129068){unsigned index=c->gpr[3];if(index>=14||index/7u!=h->side){fprintf(stderr,"Visibility args mismatch gpr3=%u gpr4=%u side%u\n",index,c->gpr[4],h->side);return 0;}const nflstreet_team_activation_pose*p=&h->input->poses[index%7u];if(c->gpr[4]!=p->visibility){fprintf(stderr,"Visibility word mismatch index%u got%08x expected%08x\n",index,c->gpr[4],p->visibility);return 0;}uint32_t w[5]={p->position[0],p->position[1],p->position[2],p->rotation,p->visibility};event(h,NFLSTREET_TEAM_ACTIVATION_VISIBILITY,index,w);c->pc=c->lr;
  }else{c->downcount=-1000000000;m->dispatch(c,pc);}
  if(c->exception){fprintf(stderr,"Original activation exception pc=%08x ex=%08x\n",c->pc,c->exception);return 0;}
 }
 if(c->pc||c->gpr[1]!=STACK){fprintf(stderr,"Original activation stopped pc=%08x sp=%08x builder=%u effects=%u lr=%08x\n",c->pc,c->gpr[1],h->builder_calls,h->effect_count,c->lr);return 0;}return 1;
}

static void seed(uint8_t*ram,const nflstreet_team_activation_side*x,unsigned side,uint32_t selected_ptr){
 ptr(ram,r13addr(-0x6e34)+(uint32_t)side)[0]=x->dirty;ptr(ram,r13addr(-0x6e30)+(uint32_t)side)[0]=x->rebuild;
 put(ptr(ram,r13addr(-0x6e2c)+(uint32_t)side*4),x->formation_value);put(ptr(ram,r13addr(-0x5a34)+(uint32_t)side*4),x->count);
 uint8_t*rows=ptr(ram,raw_stride(side));for(unsigned i=0;i<20;++i){const nflstreet_team_activation_row*r=&x->roster[i];put(rows+i*12,r->id);put(rows+i*12+4,(uint32_t)r->selected);put(rows+i*12+8,r->auxiliary);}
 put(ptr(ram,LAST_PLAYER+side*0xf4u),0xccccccccu);
 uint8_t*formation=ptr(ram,form_side(side));for(unsigned i=0;i<14;++i)put(formation+i*4,x->formation_words[i]);
 /* These are the first seven initialized DOL bytes at 803A1E94 and 803A1E9C,
    addressed by the original as r13-0x7BAC/-0x7BA4. */
 memcpy(ptr(ram,r13addr(-0x7bac)),x->request_order[0],7);memcpy(ptr(ram,r13addr(-0x7ba4)),x->request_order[1],7);
 for(unsigned i=0;i<7;++i){uint8_t*p=ptr(ram,pose_side(side)+i*20);for(unsigned j=0;j<3;++j)put(p+j*4,x->poses[i].position[j]);put(p+12,x->poses[i].rotation);put(p+16,x->poses[i].visibility);}
 if(selected_ptr)for(unsigned i=0;i<7;++i)put(ptr(ram,selected_ptr)+i*4,0xa5a5a5a5u);
}

static void fill_case(nflstreet_team_activation_side*x,unsigned side,unsigned count,uint8_t dirty,uint8_t rebuild,unsigned salt){
 memset(x,0,sizeof *x);x->count=count;x->dirty=dirty;x->rebuild=rebuild;x->formation_value=0x100u+side*3u+salt;
 for(unsigned i=0;i<20;++i){x->roster[i].id=1000u+side*100u+salt*20u+i;x->roster[i].selected=(i<count&&i==1u)?-1:(int32_t)i;x->roster[i].auxiliary=0xffu;}
 for(unsigned i=0;i<14;++i)x->formation_words[i]=0x44000000u|side<<16|salt<<8|i;
 x->weighted_count=dirty?count:0;
 {static const uint8_t order[2][7]={{0,1,2,4,3,6,5},{5,0,1,4,6,2,3}};memcpy(x->request_order,order,sizeof order);}
 for(unsigned i=0;i<x->weighted_count;++i){nflstreet_team_lineup_player*w=&x->weighted_rows[i];memset(w,0,sizeof *w);w->player_id=x->roster[i].id;w->role=(i+salt)%14;for(unsigned j=0;j<14;++j)w->scores[j]=((count-i)*17u+(j*11u)+(salt*3u))%100u+1u;}
 for(unsigned i=0;i<7;++i){x->poses[i].position[0]=0x3f000000u+i*0x10000u;x->poses[i].position[1]=0xbf000000u+i*0x10000u;x->poses[i].position[2]=0x40000000u+i*0x10000u;x->poses[i].rotation=0x3f800000u+i*0x10000u;x->poses[i].visibility=i&1u;}
}

static int compare_case(const ModernGekkoModuleDesc*m,uint8_t*baseline,unsigned side,unsigned count,uint8_t dirty,uint8_t rebuild,uint8_t weights,uint32_t output){
 uint8_t*ram=malloc(RAM_SIZE);if(!ram)return 0;memcpy(ram,baseline,RAM_SIZE);nflstreet_team_activation native={0};fill_case(&native.sides[side],side,count,dirty,rebuild,count+side);if(!weights)native.sides[side].weighted_count=0;nflstreet_team_activation_side input=native.sides[side];
 uint32_t selected[7];for(unsigned i=0;i<7;++i)selected[i]=0xa5a5a5a5u;uint32_t*destination=output?selected:NULL;
 if(!nflstreet_team_activation_apply(&native,side,destination,output?7:0)){fprintf(stderr,"Native activation rejected side%u count%u flags%u/%u\n",side,count,dirty,rebuild);free(ram);return 0;}
 uint32_t selected_ptr=output?SELECTED_AT:0;seed(ram,&input,side,selected_ptr);
 /* The output list is separately reset because model apply has already written its copy. */
 if(output)for(unsigned i=0;i<7;++i)selected[i]=0xa5a5a5a5u;
 hooks h={0};h.input=&input;h.side=side;h.rebuild_expected=dirty&&rebuild;
 CPUState c={0};c.ram=ram;c.ram_size=RAM_SIZE;c.msr=0x2000;c.gpr[1]=STACK;c.gpr[2]=0x803b9a40;c.gpr[13]=R13;c.gpr[3]=side;c.gpr[4]=selected_ptr;c.pc=0x8011e460;
 if(!run_original(m,&c,&h)){free(ram);return 0;}
 unsigned want_rebuild=dirty&&rebuild,want_builder=dirty&&input.weighted_count?(1u+want_rebuild):0u;
 if(h.builder_calls!=want_builder){fprintf(stderr,"Boundary counts side%u dirty%u rebuild%u builder%u\n",side,dirty,rebuild,h.builder_calls);free(ram);return 0;}
 uint8_t*dst=ptr(ram,form_out_side(side));for(unsigned i=0;i<14;++i)if(be(dst+i*4)!=native.sides[side].active_formation[i]){fprintf(stderr,"Formation copy side%u word%u original%08x native%08x\n",side,i,be(dst+i*4),native.sides[side].active_formation[i]);free(ram);return 0;}
 if(be(ptr(ram,r13addr(-0x7bf0)+(uint32_t)side*4))!=7){fprintf(stderr,"Formation count side%u differs\n",side);free(ram);return 0;}
 if(ptr(ram,r13addr(-0x6e30)+(uint32_t)side)[0]!=(dirty&&rebuild?0:rebuild)){fprintf(stderr,"Rebuild flag side%u differs\n",side);free(ram);return 0;}
 if(output){for(unsigned i=0;i<7;++i){uint32_t expected=i<native.sides[side].selected_count?native.sides[side].selected_ids[i]:0xa5a5a5a5u;if(be(ptr(ram,selected_ptr)+i*4)!=expected){fprintf(stderr,"Selected output side%u word%u differs got%08x wanted%08x\n",side,i,be(ptr(ram,selected_ptr)+i*4),expected);free(ram);return 0;}}}
 unsigned expected_events=21u+(dirty?1u:0u)+(want_rebuild?1u:0u);if(h.effect_count!=expected_events||native.effect_count!=expected_events){fprintf(stderr,"Pose event count side%u native%u original%u expected%u\n",side,native.effect_count,h.effect_count,expected_events);free(ram);return 0;}
 for(unsigned i=0;i<expected_events;++i){const nflstreet_team_activation_effect*a=&native.effects[i];const captured_effect*b=&h.effects[i];if(a->kind!=b->kind||a->index!=b->index||memcmp(a->words,b->words,sizeof a->words)){fprintf(stderr,"Ordered activation event differs side%u event%u\n",side,i);free(ram);return 0;}}
 if(dirty){const nflstreet_team_activation_effect*e=&native.effects[want_rebuild?1:0];if(e->kind!=NFLSTREET_TEAM_ACTIVATION_BUILD||e->words[0]!=side||e->words[1]!=native.sides[side].formation_value||e->words[3]!=native.sides[side].builder_count){fprintf(stderr,"Native builder request malformed side%u\n",side);return 0;}}
 if(dirty){for(unsigned i=0;i<input.count;++i){uint8_t*r=ptr(ram,raw_stride(side)+i*12);if(be(r)!=native.sides[side].roster[i].id||be(r+4)!=(uint32_t)native.sides[side].roster[i].selected||be(r+8)!=native.sides[side].roster[i].auxiliary){fprintf(stderr,"E7EC roster mismatch side%u row%u\n",side,i);free(ram);return 0;}}}
 if(want_rebuild&&be(ptr(ram,LAST_PLAYER+side*0xf4u))!=native.sides[side].last_seventh_player_id){fprintf(stderr,"E7EC last seventh ID mismatch side%u original%08x native%08x\n",side,be(ptr(ram,LAST_PLAYER+side*0xf4u)),native.sides[side].last_seventh_player_id);free(ram);return 0;}
 free(ram);return 1;
}

static int load_snapshot(const char*path,uint8_t**out){size_t n=0;uint8_t*p=load_file(path,&n,RAM_SIZE);if(!p||n!=RAM_SIZE){free(p);return 0;}*out=p;return 1;}
static uint64_t hash_bytes(const uint8_t*p,size_t n){uint64_t h=1469598103934665603ull;for(size_t i=0;i<n;++i){h^=p[i];h*=1099511628211ull;}return h;}

int main(int argc,char**argv){
 if(argc!=3){fprintf(stderr,"Usage: team-activation-check.exe <original-DLL> <snapshot.mem1>\n");return 2;}
 HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;const ModernGekkoModuleDesc*m=get();if(!m||m->cpu_state_size!=sizeof(CPUState))return 3;
 uint8_t*baseline=NULL;if(!load_snapshot(argv[2],&baseline))return 4;uint64_t before=hash_bytes(baseline,RAM_SIZE);
 {static const uint8_t expected[2][7]={{0,1,2,4,3,6,5},{5,0,1,4,6,2,3}};if(memcmp(ptr(baseline,0x803a1e94u),expected[0],7)||memcmp(ptr(baseline,0x803a1e9cu),expected[1],7)){fprintf(stderr,"Saved-context initialized 8004278C request-order bytes differ from DOL values\n");return 4;}}
 const struct{unsigned side,count;uint8_t dirty,rebuild,weights;uint32_t output;}cases[]={{0,9,1,1,1,1},{1,12,1,1,1,0},{0,9,1,0,1,1},{0,9,1,0,0,1},{0,9,0,1,0,1},{1,12,0,0,0,0}};
 for(unsigned i=0;i<sizeof cases/sizeof *cases;++i)if(!compare_case(m,baseline,cases[i].side,cases[i].count,cases[i].dirty,cases[i].rebuild,cases[i].weights,cases[i].output)){fprintf(stderr,"Activation case%u failed\n",i);return 5;}
 uint8_t*after=NULL;if(!load_snapshot(argv[2],&after)||hash_bytes(after,RAM_SIZE)!=before){fprintf(stderr,"Saved context changed during verification\n");return 6;}
 printf("Team activation/rebuild original comparison passes: %u controlled side cases; original E460/E7EC/E3E8/F4EC and 4278C execute when weighted rows are supplied; the legacy dirty-without-rebuild supplied-formation boundary is retained. Rebuilt roster triples, 14 formation words/count7, optional selected IDs, and 21 ordered position/rotation/visibility calls match. Initialized DOL order bytes and fixed pose constants are exercised; database weighting remains supplied. Baseline FNV64 %016llx unchanged.\n",(unsigned)(sizeof cases/sizeof *cases),(unsigned long long)before);
 free(after);free(baseline);FreeLibrary(h);return 0;
}
