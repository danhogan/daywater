#include "draw_engine.h"
/* Player-specific part of 80161E10; the shared render engine is in draw_engine.c. */
/* 801EC0D4/801EB12C/801CD484: rebuild an instance's chunk list from its
   group selectors (+1C, 0xFF hides a group) when +14 or +18 is set. */
static int refresh(nflstreet_draw *d,uint32_t instance){
 uint8_t *in=nflstreet_draw_at(d,instance,0xe0);if(!in)return 0;uint32_t model=nflstreet_rd32(in+0xc);
 if(!model||!(nflstreet_rd32(in+0x14)|nflstreet_rd32(in+0x18)))return 1;
 uint32_t groups,count=0,n=0,list;if(!nflstreet_draw_word(d,model+0x14,&groups))return 0;
 if(groups&&!nflstreet_draw_word(d,groups,&count))return 0;if(count>0x3f)count=0x40;
 for(uint32_t i=0;i<count;++i){uint8_t sel=in[0x1c+i];uint32_t e;if(sel==0xff)continue;
  if(!nflstreet_draw_variant(d,groups,(int32_t)i,sel,&e))return 0;if(!e)continue;const uint8_t *p=nflstreet_draw_at(d,e,0x14);if(!p||!nflstreet_draw_word(d,groups+0xc,&list))return 0;
  uint32_t k=nflstreet_rd16(p+0x12);if(n+k>64)return 0;
  for(uint32_t j=0;j<k;++j){const uint8_t *q=nflstreet_draw_at(d,list+2u*(nflstreet_rd16(p+0x10)+j),2);if(!q)return 0;nflstreet_wr16(in+0x60+2u*(n+j),nflstreet_rd16(q));}
  n+=k;}
 nflstreet_wr32(in+0x5c,n);nflstreet_wr32(in+4,nflstreet_rd32(in+4)|1);nflstreet_wr32(in+8,nflstreet_rd32(in+8)|1);nflstreet_wr32(in+0x18,0);nflstreet_wr32(in+0x14,0);return 1;}
/* 801EC140(0, instance, 0): instance {-, flags +04/+08, model +0C,
   selectors, chunk count +5C, chunk list +60, matrix palette +E0, materials +E8}. */
static int draw_instance(nflstreet_draw *d,uint32_t instance){
 uint32_t model,materials,pal,count;
 if(!refresh(d,instance)||!nflstreet_draw_word(d,instance+0xc,&model)||!nflstreet_draw_select_model(d,model)||!nflstreet_draw_word(d,instance+0xe8,&materials))return 0;
 d->s->materials=materials;
 return nflstreet_draw_word(d,instance+0xe0,&pal)&&nflstreet_draw_palette(d,pal)&&nflstreet_draw_word(d,instance+0x5c,&count)&&nflstreet_draw_list(d,instance+0x60,count);}

/* 800D5800 linear remap with clamping (NaN takes the high value). */
static float remap(float x,float hi,float at_hi,float lo,float at_lo){
 if(!(x<=hi))return at_hi;if(!(x>=lo))return at_lo;if(hi==lo)return 0.0f;return nflstreet_fmadds((x-lo)/(hi-lo),at_hi-at_lo,at_lo);}
/* 8016373C: eye texture offsets from the skeleton's eye angles (+A8..+B0
   halfwords, 24-bit angle units in degrees) when actor flag bit 7 is set. */
static int eye_offsets(nflstreet_draw *d,uint32_t actor,float a[2],float b[2]){
 uint32_t flags,skeleton;if(!nflstreet_draw_word(d,actor+0x14,&flags))return 0;
 if(!(flags&0x80)){a[0]=a[1]=b[0]=b[1]=0.0f;return 1;}
 const uint8_t *e;if(!nflstreet_draw_word(d,actor+0x5c,&skeleton)||!(e=nflstreet_draw_at(d,skeleton+0xa8,10)))return 0;
 /* Halfwords +A8, +AA, (+AC unused), +AE, +B0. */
 float deg[5];for(unsigned i=0;i<5;++i)deg[i]=(float)(int32_t)((uint32_t)(int32_t)(int16_t)nflstreet_rd16(e+2*i)<<8)*2.1457672119140625e-05f;
 b[0]=remap(deg[3],45.619f,-0.03f,-45.619f,0.03f);a[0]=remap(deg[0],45.619f,-0.03f,-45.619f,0.03f);
 b[1]=-remap(deg[4],47.028f,-0.1f,-47.028f,0.1f);a[1]=-remap(deg[1],47.028f,-0.1f,-47.028f,0.1f);
 return 1;}
static int eye_matrix(nflstreet_draw *d,float tx,float ty){
 uint32_t m[8]={nflstreet_float_bits(1.0f),0,0,nflstreet_float_bits(tx),0,nflstreet_float_bits(1.0f),0,nflstreet_float_bits(ty)};
 return nflstreet_draw_gx(d,NFLSTREET_GX_LOAD_TEX_MTX,2,(uint32_t[]){0x1e,1},m,8)&&NFLSTREET_GX(d,NFLSTREET_GX_SET_TEX_COORD_GEN,0,1,4,0x1e,0,0x7d);}
/* 80162220: head. The draw struct at actor +4A8 is {actor, flags +04, LOD
   +08, -, LOD model +10}. */
static int draw_head(nflstreet_draw *d,uint32_t actor){
 uint32_t st,lod,flags,model;int compiled=0;
 if(!nflstreet_draw_word(d,actor+0x4a8,&st)||!nflstreet_draw_word(d,st+8,&lod))return 0;
 if(lod)return draw_instance(d,actor+0x59c);
 if(!nflstreet_draw_word(d,st+4,&flags))return 0;
 if((flags&8)&&(!d->x->face||!d->x->face(d->x->face_context,actor,&compiled)))return 0;
 if(compiled)model=d->x->shared_model;else{if(!nflstreet_draw_word(d,st+0x10,&model))return 0;model+=0x10;}
 float a[2],b[2];
 if(!nflstreet_draw_select_model(d,model))return 0;d->s->materials=actor+0xfec;
 return nflstreet_draw_palette(d,actor+0x1014)&&nflstreet_draw_blend(d,1)&&NFLSTREET_GX(d,NFLSTREET_GX_SET_CULL_MODE,0)&&eye_offsets(d,actor,a,b)&&
  eye_matrix(d,b[0],b[1])&&nflstreet_draw_group(d,3,0)&&eye_matrix(d,a[0],a[1])&&nflstreet_draw_group(d,4,0)&&
  NFLSTREET_GX(d,NFLSTREET_GX_SET_TEX_COORD_GEN,0,1,4,0x3c,0,0x7d)&&nflstreet_draw_group(d,2,0);}
/* 80161E10 light color: actor +3EC (target RGB at +0C..+14) is approached
   over 15 frames from +1A74..+1A7C toward +1A64..+1A6C; +1A88 fades by
   (2 - t/30) for 30 frames, otherwise +1A4C == 1 applies +1A50..+1A58. */
static int light_color(nflstreet_draw *d,uint8_t *a,float rgb[3]){
 const float k=0.06666667014360428f;uint32_t target=nflstreet_rd32(a+0x3ec);rgb[0]=rgb[1]=rgb[2]=1.0f;
#define F(o) nflstreet_as_float(nflstreet_rd32(a+(o)))
#define SETF(o,v) nflstreet_wr32(a+(o),nflstreet_float_bits(v))
 if(target){const uint8_t *t=nflstreet_draw_at(d,target+0xc,12);if(!t)return 0;
  if(!(nflstreet_as_float(nflstreet_rd32(t))==F(0x1a64))||!(nflstreet_as_float(nflstreet_rd32(t+4))==F(0x1a68))||!(nflstreet_as_float(nflstreet_rd32(t+8))==F(0x1a6c))){
   float n=(float)nflstreet_rd32(a+0x1a84),r0=F(0x1a74),g0=F(0x1a78),b0=F(0x1a7c);
   float dr=(F(0x1a64)-r0)*k,dg=(F(0x1a68)-g0)*k,db=(F(0x1a6c)-b0)*k;
   SETF(0x1a7c,nflstreet_fmadds(db,n,b0));SETF(0x1a74,nflstreet_fmadds(dr,n,r0));SETF(0x1a78,nflstreet_fmadds(dg,n,g0));
   nflstreet_wr32(a+0x1a64,nflstreet_rd32(t));nflstreet_wr32(a+0x1a68,nflstreet_rd32(t+4));nflstreet_wr32(a+0x1a84,0);nflstreet_wr32(a+0x1a6c,nflstreet_rd32(t+8));}
  uint32_t step=nflstreet_rd32(a+0x1a84);
  if(step<=14){float n=(float)step;nflstreet_wr32(a+0x1a84,step+1);
   rgb[2]=nflstreet_fmadds((F(0x1a6c)-F(0x1a7c))*k,n,F(0x1a7c));rgb[0]=nflstreet_fmadds((F(0x1a64)-F(0x1a74))*k,n,F(0x1a74));
   rgb[1]=nflstreet_fmadds((F(0x1a68)-F(0x1a78))*k,n,F(0x1a78));}
  else{rgb[0]=F(0x1a64);rgb[1]=F(0x1a68);rgb[2]=F(0x1a6c);}}
 if(nflstreet_rd32(a+0x1a88)){uint32_t step=nflstreet_rd32(a+0x1a8c);
  if(step>0x1d){nflstreet_wr32(a+0x1a8c,0);nflstreet_wr32(a+0x1a88,0);return 1;}
  float w=2.0f-(float)step*0.03333333507180214f;nflstreet_wr32(a+0x1a8c,step+1);
  float sb=F(0x1a58)*w,sr=F(0x1a50)*w,sg=F(0x1a54)*w;rgb[0]*=sr;rgb[1]*=sg;rgb[2]*=sb;}
 else if(a[0x1a4c]==1){rgb[0]*=F(0x1a50);rgb[1]*=F(0x1a54);rgb[2]*=F(0x1a58);}
#undef F
#undef SETF
 return 1;}
int nflstreet_player_draw(const nflstreet_player_draw_context *x,uint32_t actor,nflstreet_render_state *s){
 if(!x||!x->span||!x->emit||!s||!actor)return 0;
 nflstreet_draw d={x,s};uint8_t *a=nflstreet_draw_at(&d,actor,0x1a90);if(!a)return 0;
 nflstreet_draw_reset(&d);
 float dim=(nflstreet_rd32(a+0x14)&8)?x->dim[1]:x->dim[0],far_z=1.0f-nflstreet_as_float(nflstreet_rd32(a+0x1084))*dim,rgb[3];
 if(!light_color(&d,a,rgb)||!nflstreet_draw_light_set(&d,1,rgb[0],rgb[1],rgb[2]))return 0;
 static const uint16_t before[3]={0x4ac,0xd1c,0xe0c},after[2]={0xb3c,0xc2c},late[2]={0x77c,0xefc};
 if(!nflstreet_draw_gx(&d,NFLSTREET_GX_SET_CHAN_MAT_COLOR,1,(uint32_t[]){4},(uint32_t[]){0xffffffffu},1)||!NFLSTREET_GX(&d,NFLSTREET_GX_SET_NUM_CHANS,1)||
    !NFLSTREET_GX(&d,NFLSTREET_GX_SET_CHAN_CTRL,4,1,0,0,s->light_mask,2,2)||!NFLSTREET_GX(&d,NFLSTREET_GX_SET_ALPHA_COMPARE,7,0,0,7,0)||
    !NFLSTREET_GX(&d,NFLSTREET_GX_SET_CULL_MODE,1)||!nflstreet_draw_blend(&d,1))return 0;
 for(unsigned i=0;i<3;++i)if(!draw_instance(&d,actor+before[i]))return 0;
 if(!draw_head(&d,actor))return 0;
 for(unsigned i=0;i<2;++i)if(!draw_instance(&d,actor+after[i]))return 0;
 if(!nflstreet_draw_depth_range(&d,0.0f,far_z))return 0;
 for(unsigned i=0;i<2;++i)if(!draw_instance(&d,actor+late[i]))return 0;
 if(!NFLSTREET_GX(&d,NFLSTREET_GX_SET_ALPHA_COMPARE,4,0,0,7,0)||!nflstreet_draw_blend(&d,2)||!draw_instance(&d,actor+0x95c))return 0;
 /* 800DEB30: record +65 = 0 (headwear) draws instance +68C with blend preset 1. */
 const uint8_t *record=nflstreet_draw_at(&d,x->player_records+0x68u*a[0x1363]+0x65u,1);
 if(!record||(!*record&&!nflstreet_draw_blend(&d,1)))return 0;
 return draw_instance(&d,actor+0x68c)&&nflstreet_draw_blend(&d,2)&&draw_instance(&d,actor+0xa4c)&&draw_instance(&d,actor+0x86c)&&
  nflstreet_draw_blend(&d,1)&&nflstreet_draw_depth_range(&d,0.0f,1.0f)&&NFLSTREET_GX(&d,NFLSTREET_GX_SET_CULL_MODE,0);}
