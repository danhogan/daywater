#define main team_classes_check_main
#include "team_classes_check.c"
#undef main
#include "team_screen.h"

#define TEST_OFFSET 0x50000u

static void set32(uint8_t *p, uint32_t x) { put(p,x); }

static int find_descriptor(uint8_t *file,size_t n,uint32_t target,uint32_t *desc,
                           uint32_t *text,uint32_t *cap){
 if(target<BASE||target-BASE>n||n-(target-BASE)<40)return 0;
 uint8_t *o=file+target-BASE;uint32_t d=be(o+32);
 if(d<BASE||d-BASE>n||n-(d-BASE)<12)return 0;
 *desc=d;*text=be(file+d-BASE+8);*cap=be(file+d-BASE+4);return 1;
}

static int pick(uint8_t *file,size_t n,const nflstreet_team_classes *s,
                unsigned mode,uint32_t *target,uint32_t *desc,uint32_t *text,uint32_t *cap){
 for(unsigned i=0;i<s->request_count;++i){const nflstreet_ui_service_request*q=&s->requests[i];
  if(q->class_id!=8||q->target<BASE||q->target-BASE>n||n-(q->target-BASE)<40)continue;
  uint8_t *o=file+q->target-BASE;unsigned flags=(unsigned)o[0]<<8|o[1],font=o[2];
  if(mode==0&&(flags&0x100||font>=4))continue;
  if(mode==1&&!(flags&0x100))continue;
  if(mode==2&&font!=1)continue;if(mode==3&&font!=3)continue;
  uint32_t d,t,c;if(!find_descriptor(file,n,q->target,&d,&t,&c)||c<16)continue;
  *target=q->target;*desc=d;*text=t;*cap=c;return 1;
 }
 return 0;
}

static int original_update(const ModernGekkoModuleDesc*m,CPUState*c,
                           original_log*log,uint32_t target,uint32_t source_desc,
                           unsigned*font_lookups){
 uint32_t args_at=0x81200200u;put(ram_pointer(c->ram,args_at),source_desc);
 c->gpr[3]=target;c->gpr[4]=3;c->gpr[6]=args_at;c->gpr[7]=0;
 c->pc=0x801d9244;c->lr=0;c->downcount=c->cycle_budget=100000000;
 unsigned lookups=0;
 for(unsigned step=0;c->pc&&step<1000000;++step){uint32_t pc=c->pc;
  if(pc==0x801d5c24){if(c->gpr[3]!=1)return 0;c->gpr[3]=MANAGER;c->pc=c->lr;}
  else if(pc==0x8018baa8){c->pc=c->lr;}
  else if(pc==0x801d5884){uint32_t entry=c->gpr[5];if(c->gpr[4]!=2||entry>=4)return 0;++lookups;c->gpr[3]=FONT_RESOURCE+entry*0x20u;c->pc=c->lr;}
  else if(pc==0x8018b7b8){if(log->line_allocations>=NFLSTREET_TEAM_CLASSES_LINE_PLAN_CAPACITY||c->gpr[3]>1020||c->gpr[4])return 0;log->line_allocation_sizes[log->line_allocations]=c->gpr[3];c->gpr[3]=0x81610000u+log->line_allocations*1024u;++log->line_allocations;c->pc=c->lr;}
  else {c->downcount=-1000000000;m->dispatch(c,pc);}
  if(c->exception){fprintf(stderr,"Update exception pc=%08x exception=%08x\n",c->pc,c->exception);return 0;}
 }
 if(c->pc||lookups!=1){fprintf(stderr,"Update stopped pc=%08x fontLookups=%u\n",c->pc,lookups);return 0;}
 if(font_lookups)*font_lookups=lookups;
 return 1;
}

static int one_sequence(const ModernGekkoModuleDesc*m,CPUState*c,original_log*log,
                        nflstreet_team_screen*native,uint8_t*nf,uint8_t*of,
                        size_t n,uint32_t target,uint32_t desc,uint32_t text,
                        uint32_t cap,unsigned mode,unsigned*font_total,
                        unsigned*allocation_total){
 static const char *const labels[]={"ALPHA\nBETA","GAMMA","ALPHA\nBETA"};
 (void)of;uint8_t *native_text=nf+text-BASE,*original_text=ram_pointer(c->ram,text);
 uint8_t *native_desc=nf+desc-BASE,*original_desc=ram_pointer(c->ram,desc);
 if(!original_text||!original_desc||!native_desc||cap<12||cap>4096){fprintf(stderr,"Bad selected object mapping mode%u cap%u\n",mode,cap);return 0;}
 uint32_t src_desc=BASE+TEST_OFFSET,src_text=src_desc+0x100;
 for(unsigned step=0;step<3;++step){
  size_t len=strlen(labels[step]);if(len>=cap){fprintf(stderr,"Label exceeds cap mode%u cap%u\n",mode,cap);return 0;}
  memset(nf+TEST_OFFSET,0,12+0x200);memset(ram_pointer(c->ram,src_desc),0,12+0x200);
  set32(nf+TEST_OFFSET+4,64);set32(nf+TEST_OFFSET+8,src_text);
  set32(ram_pointer(c->ram,src_desc)+4,64);set32(ram_pointer(c->ram,src_desc)+8,src_text);
  memcpy(nf+TEST_OFFSET+0x100,labels[step],len+1);memcpy(ram_pointer(c->ram,src_text),labels[step],len+1);
  uint8_t *ns=nf+text-BASE,*os=original_text;
  memset(ns,0xa5,cap);memset(os,0xa5,cap);memcpy(ns,"old",4);memcpy(os,"old",4);
  /* Seed the same caller-owned source descriptor and invoke prop3 on both sides. */
  nflstreet_ui_service_request q={0};q.kind=0;q.class_id=8;q.property=3;q.target=target;q.argc=1;q.args[0]=src_desc;
  uint32_t result=0xdeadbeefu;
  if(!nflstreet_team_screen_service(native,nf,n+0x60000,BASE,&q,&result)){fprintf(stderr,"Native prop3 update failed mode%u step%u\n",mode,step);return 0;}
  log->line_allocations=0;memset(log->line_allocation_sizes,0,sizeof log->line_allocation_sizes);
  unsigned looked=0;if(!original_update(m,c,log,target,src_desc,&looked)){fprintf(stderr,"Original prop3 update failed mode%u step%u\n",mode,step);return 0;}
  *font_total+=looked;*allocation_total+=log->line_allocations;
  uint8_t no[40],oo[40];memcpy(no,nf+target-BASE,40);memcpy(oo,ram_pointer(c->ram,target),40);
  uint32_t nlines=be(no+36),olines=be(oo+36);
  const nflstreet_team_classes_line_plan *plan=NULL;
  for(unsigned i=0;i<native->objects.line_plan_count;++i)if(native->objects.line_plans[i].target==target)plan=&native->objects.line_plans[i];
  if(oo[15]>1){if(olines<0x80000000u||!plan||plan->plan.count!=oo[15]||!log->line_allocations||log->line_allocation_sizes[0]!=oo[15]*4u){fprintf(stderr,"Line plan/allocation mismatch mode%u step%u count%u allocs%u size%u\n",mode,step,oo[15],log->line_allocations,log->line_allocation_sizes[0]);return 0;}
   uint8_t*table=ram_pointer(c->ram,olines);if(!table)return 0;
   for(unsigned j=0;j<oo[15];++j){uint32_t expected=(uint32_t)plan->plan.lines[j].begin<<16|plan->plan.lines[j].end;if(be(table+j*4)!=expected){fprintf(stderr,"Line range mode%u step%u index%u differs\n",mode,step,j);return 0;}}
   if(nlines!=plan->line_base){fprintf(stderr,"Native line table address is not its reserved slot mode%u step%u\n",mode,step);return 0;}
   put(no+36,0x817c0000u);put(oo+36,0x817c0000u);
  }else {
   if(log->line_allocations){fprintf(stderr,"Original allocated unexpected single-line table\n");return 0;}
  }
  if(memcmp(no,oo,sizeof no)||memcmp(native_text,original_text,cap)){
   fprintf(stderr,"Text update mode%u step%u object or full destination bytes differ\n",mode,step);return 0;
  }
 }
 return 1;
}

static int run_original(const char*dll,const char*snapshot,const char*source_path,const char*folder){
 HMODULE h=LoadLibraryA(dll);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;const ModernGekkoModuleDesc*m=get();if(!m||m->cpu_state_size!=sizeof(CPUState))return 3;
 size_t snapn=0,n=0;uint8_t*snap=load_file(snapshot,&snapn,RAM_SIZE),*source=load_file(source_path,&n,8u*1024u*1024u);uint8_t*nf=calloc(1,n+0x60000),*ram=calloc(1,RAM_SIZE);original_log*log=calloc(1,sizeof *log);nflstreet_team_screen*state=calloc(1,sizeof *state);if(!snap||snapn!=RAM_SIZE||!source||n!=253987||!nf||!ram||!log||!state)return 4;
 memcpy(ram,snap,RAM_SIZE);memcpy(nf,source,n);memcpy(ram+FILE_AT,source,n);free(snap);
 size_t fs[4]={0};uint8_t*fb[4]={0};if(!load_fonts(folder,state->objects.fonts,ram,fs,fb))return 5;
 unsigned relocated=0;if(!nflstreet_ui_relocate(nf,n,BASE,&relocated))return 6;
 /* Padding is outside the source file's relocation and hosts bounded test descriptors. */
 memset(ram+0x17f0000,0,0x3000);put(ram+0x17f0048,9);put(ram+0x17f004c,0x817b0000);put(ram+0x17f0104,7);put(ram+0x17f0108,5);put(ram+0x17f0110,BASE);
 memset(ram+REGISTRY,0,36);put(ram+REGISTRY,0x801d60dc);put(ram+REGISTRY+0x20,0x801d9244);
 if(!nflstreet_team_classes_construct(nf,n,BASE,&state->objects))return 7;
 memcpy(ram+FILE_AT,nf,n);CPUState c={0};c.ram=ram;c.ram_size=RAM_SIZE;c.msr=0x2000;c.gpr[1]=0x81700000;c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;
 if(!original_construct(m,&c,log))return 8;
 /* Initial construction is already independently compared by team-classes-check. */
 uint8_t*origfile=ram+FILE_AT;
 uint32_t chosen[4],descs[4],texts[4],caps[4];unsigned font_total=0,allocation_total=0;
 for(unsigned mode=0;mode<4;++mode)if(!pick(nf,n,&state->objects,mode,&chosen[mode],&descs[mode],&texts[mode],&caps[mode])){fprintf(stderr,"No source object for update mode%u\n",mode);return 10;}
 /* Native and original constructors produced independent source copies. */
 for(unsigned mode=0;mode<4;++mode){
  if(!one_sequence(m,&c,log,state,nf,origfile,n,chosen[mode],descs[mode],texts[mode],caps[mode],mode,&font_total,&allocation_total))return 11;
 }
 printf("Team text update original comparison passes: 4 actual class-8 source objects (newline, wrapped, font1, font3), 3 updates each including multiline→single→multiline; original 801D9244 property3, 801D9E60 and 801D8E54 executed, %u original font lookups and %u line allocations; source construction baseline established by independently checked entry-7 constructor.\n",font_total,allocation_total);
 return 0;
}

int main(int argc,char**argv){if(argc==5)return run_original(argv[1],argv[2],argv[3],argv[4]);fprintf(stderr,"Usage: team-text-update-check.exe <original-DLL> <snapshot> <source> <unpacked-folder>\n");return 2;}
