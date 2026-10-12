/* Full associated frame-record and retained spatial-query oracle. */
#include <windows.h>
#include "moderngekko/module_abi.h"
#include "../ball/ball_attachment.h"
#include "../ball/ball_orientation.h"
#include "../ball/ball_frame.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned be(const uint8_t*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static void put(uint8_t*p,unsigned v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static unsigned spatial_calls;static uint32_t query_words[3];
static int call(const ModernGekkoModuleDesc*m,CPUState*c,unsigned pc){c->pc=pc;c->lr=0;c->exception=0;for(unsigned i=0;c->pc&&i<2000000;++i){if(c->pc==0x801087b8){unsigned at=c->gpr[3]-0x80000000;if(at>24*1024*1024-12)return 0;for(unsigned j=0;j<3;++j)query_words[j]=be(c->ram+at+j*4);++spatial_calls;c->gpr[3]=0x12345678;c->pc=c->lr;continue;}c->downcount=-1000000000;m->dispatch(c,c->pc);if(c->exception)return 0;}return !c->pc;}

static float real(const uint8_t*p){unsigned w=be(p);float f;memcpy(&f,&w,4);return f;}
static void putfloat(uint8_t*p,float f){unsigned w;memcpy(&w,&f,4);put(p,w);}
int main(int argc,char**argv){
 if(argc!=3)return 2;HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);if(!get)return 3;const ModernGekkoModuleDesc*m=get();
 if(m->cpu_state_size!=sizeof(CPUState))return 3;uint8_t*ram=malloc(24*1024*1024);FILE*f=fopen(argv[2],"rb");
 if(!ram||!f||fread(ram,1,24*1024*1024,f)!=24*1024*1024)return 4;fclose(f);
 if(be(ram+0x252164)!=0)return 4;unsigned actors[2]={0x3c04ec,0x3c19c0},checks=0,joins=0;
 for(unsigned actor=0;actor<2;++actor){unsigned a=actors[actor],model=be(ram+a+4)-0x80000000;
  if(model>24*1024*1024-0x68)return 4;
  unsigned cluster=be(ram+model+0x60)-0x80000000,skeleton=be(ram+model+0x64)-0x80000000;
  if(skeleton>24*1024*1024-8||memcmp(ram+skeleton,"SKL1",4))return 4;
  unsigned count=(unsigned)ram[skeleton+6]<<8|ram[skeleton+7];
  if(!count||count>256||cluster>24*1024*1024-count*64)return 4;
  int original_selector=(int)(int16_t)((unsigned)ram[model+0x32]<<8|ram[model+0x33]);
  if(original_selector<0||(unsigned)original_selector>=count)return 4;
  float base_world[12],base_root[12],base_local[3],clusters[256][12];
  for(unsigned i=0;i<12;++i)base_root[i]=real(ram+model+0x3ac+i*4);float base_scale=real(ram+model+0x18);
  for(unsigned i=0;i<12;++i)base_world[i]=real(ram+a+0x280+i*4);
  for(unsigned i=0;i<3;++i)base_local[i]=real(ram+model+0x40+i*4);
  for(unsigned row=0;row<count;++row)for(unsigned i=0;i<12;++i)clusters[row][i]=real(ram+cluster+row*64+i*4);
  for(unsigned test=0;test<4;++test)for(int selection=-2;selection<=original_selector;++selection){
   int selector=selection== -2?original_selector:selection;int32_t rotation[3];for(unsigned i=0;i<3;++i)rotation[i]=(int32_t)be(ram+model+0x50+i*4);nflstreet_ball_pose pose;float ball_rotation[4];float world[12],local[3],out[3];memcpy(world,base_world,sizeof world);memcpy(local,base_local,sizeof local);
   if(test==1){world[3]+=3.25f;world[7]-=7.5f;world[11]+=1.125f;}
   if(test==2){local[0]=0.25f;local[1]=-0.5f;local[2]=0.125f;}
   if(test==3){world[0]*=1.25f;world[5]*=0.75f;world[10]*=1.5f;}
   for(unsigned i=0;i<12;++i)putfloat(ram+a+0x280+i*4,world[i]);for(unsigned i=0;i<3;++i)putfloat(ram+model+0x40+i*4,local[i]);
   ram[model+0x32]=(unsigned)selector>>8;ram[model+0x33]=(unsigned)selector;
   CPUState c={0};c.gpr[2]=0x803b9a40;c.gpr[13]=0x803a9a40;c.gpr[1]=0x81700000;c.ram=ram;c.ram_size=24*1024*1024;c.msr=0x2000;c.hid2=0xe0000000;c.gqr[6]=0x0f070007;
   c.gpr[3]=a+0x80000000;c.gpr[4]=0x817f0000;c.gpr[5]=0x817f0010;
   if(!call(m,&c,0x80020c2c)){fprintf(stderr,"Original failed actor=%u test=%u selector=%d pc=%08x exception=%u\n",actor,test,selector,c.pc,c.exception);return 5;}if(!nflstreet_ball_attachment_position(world,clusters,(size_t)original_selector+1,selector,local,out)){fprintf(stderr,"Native rejected actor=%u test=%u selector=%d\n",actor,test,selector);return 5;}
   if(!nflstreet_ball_attachment_pose(world,clusters,(size_t)original_selector+1,selector,local,rotation,&pose)){fprintf(stderr,"Native orientation rejected actor=%u test=%u selector=%d\n",actor,test,selector);return 9;}
   for(unsigned i=0;i<4;++i){unsigned word;memcpy(&word,pose.quaternion+i,4);if(word!=be(ram+0x17f0010+i*4)){fprintf(stderr,"Orientation mismatch actor=%u test=%u selector=%d word=%u native=%08x original=%08x\n",actor,test,selector,i,word,be(ram+0x17f0010+i*4));return 9;}}

   if(memcmp(out,pose.position,sizeof out)||!nflstreet_ball_attachment_rotation(pose.quaternion,ball_rotation))return 9;
   for(unsigned i=0;i<3;++i){unsigned word;memcpy(&word,out+i,4);if(word!=be(ram+0x17f0000+i*4)){fprintf(stderr,"Attachment mismatch actor=%u test=%u selector=%d axis=%u native=%08x original=%08x\n",actor,test,selector,i,word,be(ram+0x17f0000+i*4));return 6;}}++checks;
   unsigned root=be(ram+0x3a2670),ball=root-0x80000000+8;put(ram+ball+0xb4,be(ram+a));
   unsigned visual=be(ram+ball)-0x80000000;if(visual>24*1024*1024-0x7c)return 13;
   float model_root[12];memcpy(model_root,base_root,sizeof model_root);float scale=base_scale;
   if(test==1){model_root[3]+=1.125f;model_root[7]-=3.5f;}if(test==2)scale=0.75f;if(test==3)scale=1.5f;
   for(unsigned i=0;i<12;++i)putfloat(ram+model+0x3ac+i*4,model_root[i]);putfloat(ram+model+0x18,scale);
   for(unsigned i=0;i<6;++i)putfloat(ram+ball+0x7c+i*4,(float)(i+1)*0.25f);
   memset(ram+ball+0x28,0xa5,28);
   uint8_t native_record[0xf4];memcpy(native_record,ram+ball,sizeof native_record);nflstreet_ball_pose native_model;
   const double deltas[4]={0,1,1.0/60.0,-0.5};double delta=deltas[test];
   nflstreet_ball_pose previous;for(unsigned i=0;i<3;++i)previous.position[i]=real(ram+visual+4+i*4);for(unsigned i=0;i<4;++i)previous.quaternion[i]=real(ram+visual+0x6c+i*4);
   nflstreet_ball_attachment_inputs inputs={world,model_root,scale,clusters,(size_t)original_selector+1,selector,local,rotation};nflstreet_ball_frame_output output;
   if(!nflstreet_ball_frame(native_record,sizeof native_record,delta,0,&inputs,&previous,&output))return 14;native_model=output.visual_pose;spatial_calls=0;
   c.gpr[3]=root+8;c.fpr[1]=delta;if(!call(m,&c,0x800c0fe4)){fprintf(stderr,"Original ball pose failed actor=%u test=%u selector=%d pc=%08x\n",actor,test,selector,c.pc);return 8;}
   if(spatial_calls!=1||output.spatial_query!=1||be(ram+visual+0x294)!=0x12345678)return 21;
   for(unsigned i=0;i<3;++i){unsigned word;memcpy(&word,output.spatial_query_position+i,4);if(word!=query_words[i])return 21;}
   for(unsigned i=0;i<3;++i){unsigned word;memcpy(&word,out+i,4);if(word!=be(ram+ball+12+i*4))return 8;}
   for(unsigned i=0;i<4;++i){unsigned word;memcpy(&word,ball_rotation+i,4);if(word!=be(ram+ball+0x18+i*4)){fprintf(stderr,"Ball rotation mismatch actor=%u test=%u selector=%d word=%u native=%08x original=%08x\n",actor,test,selector,i,word,be(ram+ball+0x18+i*4));return 10;}}if(memcmp(native_record,ram+ball,sizeof native_record)){fprintf(stderr,"Full record mismatch actor=%u test=%u selector=%d\n",actor,test,selector);return 15;}
   for(unsigned i=0;i<3;++i){unsigned word;memcpy(&word,native_model.position+i,4);if(word!=be(ram+visual+4+i*4)){fprintf(stderr,"Model position mismatch actor=%u test=%u selector=%d word=%u\n",actor,test,selector,i);return 16;}}
   for(unsigned i=0;i<4;++i){unsigned word;memcpy(&word,native_model.quaternion+i,4);if(word!=be(ram+visual+0x6c+i*4)){fprintf(stderr,"Model quaternion mismatch actor=%u test=%u selector=%d word=%u native=%08x original=%08x\n",actor,test,selector,i,word,be(ram+visual+0x6c+i*4));return 17;} }
   nflstreet_ball_pose render_pose;for(unsigned i=0;i<3;++i)render_pose.position[i]=real(native_record+12+i*4);for(unsigned i=0;i<4;++i)render_pose.quaternion[i]=real(native_record+0x18+i*4);float matrix[12];
   if(!nflstreet_ball_render_matrix(&render_pose,matrix))return 19;
   c.gpr[3]=0x817f2000;c.gpr[4]=root+8+0x18;c.gpr[5]=0;if(!call(m,&c,0x800c05e0))return 19;
   for(unsigned i=0;i<12;++i)putfloat(ram+0x17f2020+i*4,matrix[i]);c.gpr[3]=0x817f2020;c.gpr[4]=0x817f2000;if(!call(m,&c,0x801a3008))return 19;
   for(unsigned i=0;i<12;++i){unsigned word;memcpy(&word,matrix+i,4);if(word!=be(ram+0x17f2020+i*4)){fprintf(stderr,"Render matrix mismatch actor=%u test=%u selector=%d word=%u\n",actor,test,selector,i);return 20;}}++joins;

  }
 }
 float out[3]={17,18,19},saved[3];memcpy(saved,out,sizeof out);float world[12]={0},cluster[1][12]={{0}},local[3]={0};
 if(nflstreet_ball_attachment_position(world,cluster,1,1,local,out)||memcmp(out,saved,sizeof out))return 7;
 world[0]=NAN;if(nflstreet_ball_attachment_position(world,cluster,1,0,local,out)||memcmp(out,saved,sizeof out))return 7;
 nflstreet_ball_pose pose={{17,18,19},{20,21,22,23}},saved_pose=pose;int32_t rotation[3]={0};
 if(nflstreet_ball_attachment_pose(world,cluster,1,0,local,rotation,&pose)||memcmp(&pose,&saved_pose,sizeof pose))return 12;
 world[0]=1;world[5]=world[10]=1;cluster[0][0]=cluster[0][5]=cluster[0][10]=1;
 if(nflstreet_ball_attachment_pose(world,cluster,1,0,local,NULL,&pose)||memcmp(&pose,&saved_pose,sizeof pose))return 12;
 float invalid[4]={NAN,0,0,1},q[4]={17,18,19,20},saved_q[4];memcpy(saved_q,q,sizeof q);
 if(nflstreet_ball_attachment_rotation(invalid,q)||memcmp(q,saved_q,sizeof q))return 12;
 uint8_t guarded[0xf4]={0},saved_record[0xf4];memcpy(saved_record,guarded,sizeof guarded);
 pose=saved_pose;if(nflstreet_ball_update_attached(guarded,sizeof guarded,world,world,1,cluster,1,0,local,rotation,NAN,&pose)||memcmp(guarded,saved_record,sizeof guarded)||memcmp(&pose,&saved_pose,sizeof pose))return 18;
 float bad_root[12];memcpy(bad_root,world,sizeof bad_root);bad_root[7]=NAN;
 if(nflstreet_ball_update_attached(guarded,sizeof guarded,world,bad_root,1,cluster,1,0,local,rotation,1,&pose)||memcmp(guarded,saved_record,sizeof guarded)||memcmp(&pose,&saved_pose,sizeof pose))return 18;
 /* New caller must retain the entire record and output on late model rejection. */
 put(guarded+0xb4,0x101);memcpy(saved_record,guarded,sizeof guarded);
 nflstreet_ball_attachment_inputs inputs={world,bad_root,1,cluster,1,0,local,rotation};
 nflstreet_ball_pose previous={{1,2,3},{0,0,0,1}};nflstreet_ball_frame_output frame,saved_frame;memset(&frame,0x5a,sizeof frame);saved_frame=frame;
 if(nflstreet_ball_frame(guarded,sizeof guarded,1,0,&inputs,&previous,&frame)||memcmp(guarded,saved_record,sizeof guarded)||memcmp(&frame,&saved_frame,sizeof frame))return 22;
 if(nflstreet_ball_frame(guarded,sizeof guarded,1,0,NULL,&previous,&frame)||memcmp(guarded,saved_record,sizeof guarded)||memcmp(&frame,&saved_frame,sizeof frame))return 22;
 printf("%u original attachment positions/quaternions exact, %u complete associated frame records, visual-model poses and retained spatial-query inputs exact: two real selectors, cluster sweep, absent selector, translated roots, local offsets and scaled roots; malformed rollback pass\n",checks,joins);
 free(ram);FreeLibrary(h);return 0;
}
