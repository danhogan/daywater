#include "crowd_draw.h"
#include "scenery_shared.h"
#include "cluster_palette.h"
#include <math.h>
#define rd32 nflstreet_rd32
#define GX NFLSTREET_GX
#define LOCKED 0xe0000000u
typedef nflstreet_crowd_context context;
static float fword(const uint8_t *p){return nflstreet_as_float(rd32(p));}
static void put(uint8_t *p,float f){nflstreet_wr32(p,nflstreet_float_bits(f));}
/* RAM through the span; the locked cache from the palette context. */
static uint8_t *mem(const context *c,nflstreet_draw *d,uint32_t a,uint32_t n){
 if(a>=LOCKED)return (uint64_t)a-LOCKED+n<=sizeof c->palettes->locked?c->palettes->locked+(a-LOCKED):NULL;return nflstreet_draw_at(d,a,n);}
static int word(const context *c,nflstreet_draw *d,uint32_t a,uint32_t *v){const uint8_t *p=mem(c,d,a,4);if(!p)return 0;*v=rd32(p);return 1;}
static int fvalue(nflstreet_draw *d,uint32_t a,float *v){const uint8_t *p=nflstreet_draw_at(d,a,4);if(!p)return 0;*v=fword(p);return 1;}
static int store_word(nflstreet_draw *d,uint32_t a,uint32_t v){uint8_t *p=nflstreet_draw_at(d,a,4);if(!p)return 0;nflstreet_wr32(p,v);return 1;}
static int store_byte(nflstreet_draw *d,uint32_t a,uint8_t v){uint8_t *p=nflstreet_draw_at(d,a,1);if(!p)return 0;*p=v;return 1;}
static const nflstreet_matrix_stacks *stacks(const context *c){return &c->palettes->stacks;}
/* The stack top's 16 words (80189ED8 into a caller's local). */
static int top16(nflstreet_draw *d,const context *c,float m[16]){
 uint32_t top;const uint8_t *t;if(!nflstreet_matrix_top(d,stacks(c),&top)||!(t=nflstreet_draw_at(d,top,0x40)))return 0;
 for(unsigned i=0;i<16;++i)m[i]=fword(t+4*i);return 1;}
/* PSMTXConcat (801FFDF0) of the stack top with a local 3x4 matrix, as
   matrix_stack.c computes it for RAM matrices; columns [0, cols) written. */
static int concat_local(nflstreet_draw *d,const context *c,const float b[12],unsigned cols){
 uint32_t top;uint8_t *t;if(!nflstreet_matrix_top(d,stacks(c),&top)||!(t=nflstreet_draw_at(d,top,0x30)))return 0;
 const float *unit=c->palettes->unit;float a[12],r[12];for(unsigned i=0;i<12;++i)a[i]=fword(t+4*i);
 for(unsigned row=0;row<3;++row){const float *q=a+4*row;
  for(unsigned k=0;k<4;++k){float acc=b[k]*q[0];acc=nflstreet_fmadds(b[4+k],q[1],acc);acc=nflstreet_fmadds(b[8+k],q[2],acc);
   if(k>=2)acc=nflstreet_fmadds(unit[k-2],q[3],acc);r[4*row+k]=acc;}}
 for(unsigned row=0;row<3;++row)for(unsigned k=0;k<cols;++k)put(t+16*row+4*k,r[4*row+k]);return 1;}
/* 801897B8: the top's rotation becomes (top x Q) for the quaternion at q
   (801A3008 on an identity, translation kept). */
static int rotate_quat(nflstreet_draw *d,const context *c,uint32_t q){
 const uint8_t *p=nflstreet_draw_at(d,q,16);if(!p)return 0;
 const float x=fword(p),y=fword(p+4),z=fword(p+8),w=fword(p+12),one=c->quat_one;
 const float z2=z+z,zz=z2*z,y2=y+y,x2=x+x,yy=y2*y,xw=x2*w,xx=x2*x,zw=z2*w,xxyy=xx+yy,yw=y2*w,xy=x2*y,xz=x2*z,yz=y2*z;
 const float xxzz=xx+zz,yyzz=yy+zz;
 float m[12]={one-yyzz,xy-zw,xz+yw,0.0f, xy+zw,one-xxzz,yz-xw,0.0f, xz-yw,yz+xw,one-xxyy,0.0f};
 return concat_local(d,c,m,3);}
static int translate(nflstreet_draw *d,const context *c,uint32_t v){
 const uint8_t *p=nflstreet_draw_at(d,v,12);return p&&nflstreet_matrix_translate(d,stacks(c),fword(p),fword(p+4),fword(p+8));}
/* translation, quaternion, x quarter turn, offset, heading (80168234/80168E08). */
static int place(nflstreet_draw *d,const context *c,uint32_t position,uint32_t quaternion,uint32_t owner){
 uint32_t heading;
 return translate(d,c,position)&&rotate_quat(d,c,quaternion)&&nflstreet_matrix_rotate(d,stacks(c),0,0x400000u)&&
  translate(d,c,owner+0xb4)&&nflstreet_draw_word(d,owner+0xcc,&heading)&&nflstreet_matrix_rotate(d,stacks(c),1,heading);}
static int copy16(nflstreet_draw *d,uint32_t to,uint32_t from){
 uint8_t *t=nflstreet_draw_at(d,to,0x40),*f=nflstreet_draw_at(d,from,0x40);if(!t||!f)return 0;memmove(t,f,0x40);return 1;}
/* 801EC298(box, model, 0, 0): the view slot times the model (801CB470,
   into 80348B14) against the projection copy (801CB3EC); clip codes. */
static int cull(nflstreet_draw *d,const context *c,const float model[16],uint32_t *codes){
 const nflstreet_scenery_context *v=c->scene;const nflstreet_matrix_stacks *k=stacks(c);uint32_t top;
 if(!nflstreet_matrix_push_identity(d,k)||!nflstreet_matrix_load(d,k,c->view_slot)||!copy16(d,v->cull_projection,v->projection)||
    !nflstreet_matrix_push(d,k)||!concat_local(d,c,model,4)||!nflstreet_matrix_top(d,k,&top)||!copy16(d,v->cull_modelview,top)||
    !nflstreet_matrix_pop(d,k))return 0;
 const uint8_t *P=nflstreet_draw_at(d,v->cull_projection,0x40),*M=nflstreet_draw_at(d,v->cull_modelview,0x30),*B=nflstreet_draw_at(d,c->cull_box,0x20);
 if(!P||!M||!B)return 0;float proj[16],m[12],box[8],pts[24];
 for(unsigned i=0;i<16;++i)proj[i]=fword(P+4*i);for(unsigned i=0;i<12;++i)m[i]=fword(M+4*i);for(unsigned i=0;i<8;++i)box[i]=fword(B+4*i);
 nflstreet_box_corners(box,m,pts);*codes=nflstreet_clip_codes(pts,8,proj);return nflstreet_matrix_pop(d,k);}
/* 8017CDC8 (as the original compares) for "SPECTATOR". */
static int named(nflstreet_draw *d,uint32_t s,int *found){
 static const char pattern[]="SPECTATOR";*found=0;
 for(unsigned guard=0;guard<0x1000;++guard,++s){const uint8_t *ch=nflstreet_draw_at(d,s,1);if(!ch)return 0;if(!*ch)return 1;
  if(*ch!=(uint8_t)pattern[0])continue;
  for(unsigned i=0;;){if(!pattern[i+1]){*found=1;return 1;}++i;const uint8_t *q=nflstreet_draw_at(d,s+i,1);if(!q)return 0;if(*q!=(uint8_t)pattern[i])break;}}
 return 0;}
/* 8016870C/801687FC: a spectator on the reference player's end of the
   field (same sign of y, both beyond the limit) is hidden in modes 4/7. */
static int hidden(nflstreet_draw *d,const context *c,uint32_t owner,float y,float sign,int *out){
 uint32_t mode,play,side;int found;*out=0;
 if(!nflstreet_draw_word(d,c->game_mode,&mode))return 0;if(mode!=4&&mode!=7)return 1;
 if(!named(d,owner+0x150,&found))return 0;if(!found)return 1;
 static const uint32_t plays[4]={2,3,4,7};unsigned i=0;
 for(;i<4;++i){if(!nflstreet_play_mode(d,c->scene,&play))return 0;if(play==plays[i])break;}
 if(i==4)return 1;
 float ref,limit;if(!nflstreet_draw_word(d,c->sides+8,&side)||!fvalue(d,side+8,&ref))return 0;
 if(!(sign<ref*y))return 1;
 if(!fvalue(d,c->hide_limit,&limit))return 0;if(!(limit<fabsf(ref)))return 1;
 if(!fvalue(d,c->hide_limit,&limit))return 0;*out=limit<fabsf(y);return 1;}
/* 801ED434/801ED6A8 through 801EDC60 and the channel/alpha presets. */
static int lights(nflstreet_draw *d,uint32_t source,float scale){
 float rgb[3]={scale,scale,scale};
 if(source){const uint8_t *p=nflstreet_draw_at(d,source+0xc,12);if(!p)return 0;for(unsigned i=0;i<3;++i)rgb[i]=fword(p+4*i);}
 if(!nflstreet_draw_light_set(d,0,rgb[0],rgb[1],rgb[2]))return 0;
 return nflstreet_draw_gx(d,NFLSTREET_GX_SET_CHAN_MAT_COLOR,1,(uint32_t[]){4},(uint32_t[]){0xffffffffu},1)&&GX(d,NFLSTREET_GX_SET_NUM_CHANS,1)&&
  GX(d,NFLSTREET_GX_SET_CHAN_CTRL,4,1,0,0,d->s->light_mask,2,2)&&GX(d,NFLSTREET_GX_SET_ALPHA_COMPARE,6,0x10,0,7,0)&&GX(d,NFLSTREET_GX_SET_Z_COMP_LOC,0);}
static int load_matrix(nflstreet_draw *d,const float m[16]){
 uint32_t p[12],n[9];for(unsigned i=0;i<12;++i)p[i]=nflstreet_float_bits(m[i]);
 for(unsigned r=0;r<3;++r)for(unsigned k=0;k<3;++k)n[3*r+k]=p[4*r+k];
 return nflstreet_draw_gx(d,NFLSTREET_GX_LOAD_POS_MTX,1,(uint32_t[]){0},p,12)&&nflstreet_draw_gx(d,NFLSTREET_GX_LOAD_NRM_MTX,1,(uint32_t[]){0},n,9);}
/* 801EB204(instance + 0x14, 0, value): a changed selector marks the list stale. */
static int select_group(nflstreet_draw *d,uint32_t base,uint32_t value){
 uint8_t *b=nflstreet_draw_at(d,base+8,1);uint8_t *w=nflstreet_draw_at(d,base,4);if(!b||!w)return 0;
 if(*b==value)return 1;*b=(uint8_t)value;nflstreet_wr32(w,rd32(w)|1u);return 1;}
/* 801EC0D4/801EB12C/801CD484 (as player_draw.c): rebuild an instance's
   chunk list from its group selectors (+1C, 0xFF hides a group) when +14
   or +18 is set. */
static int refresh(nflstreet_draw *d,uint32_t instance){
 uint8_t *in=nflstreet_draw_at(d,instance,0xe0);if(!in)return 0;uint32_t model=rd32(in+0xc);
 if(!model||!(rd32(in+0x14)|rd32(in+0x18)))return 1;
 uint32_t groups,count=0,n=0,list;if(!nflstreet_draw_word(d,model+0x14,&groups))return 0;
 if(groups&&!nflstreet_draw_word(d,groups,&count))return 0;if(count>0x3f)count=0x40;
 for(uint32_t i=0;i<count;++i){uint8_t sel=in[0x1c+i];uint32_t e;if(sel==0xff)continue;
  if(!nflstreet_draw_variant(d,groups,(int32_t)i,sel,&e))return 0;if(!e)continue;const uint8_t *p=nflstreet_draw_at(d,e,0x14);if(!p||!nflstreet_draw_word(d,groups+0xc,&list))return 0;
  uint32_t k=nflstreet_rd16(p+0x12);if(n+k>64)return 0;
  for(uint32_t j=0;j<k;++j){const uint8_t *q=nflstreet_draw_at(d,list+2u*(nflstreet_rd16(p+0x10)+j),2);if(!q)return 0;nflstreet_wr16(in+0x60+2u*(n+j),nflstreet_rd16(q));}
  n+=k;}
 nflstreet_wr32(in+0x5c,n);nflstreet_wr32(in+4,rd32(in+4)|1);nflstreet_wr32(in+8,rd32(in+8)|1);nflstreet_wr32(in+0x18,0);nflstreet_wr32(in+0x14,0);return 1;}
/* 801EC140(-, instance, 0). */
static int draw_instance(nflstreet_draw *d,uint32_t instance){
 uint32_t model,materials,pal,count;
 if(!refresh(d,instance)||!nflstreet_draw_word(d,instance+0xc,&model)||!nflstreet_draw_select_model(d,model)||!nflstreet_draw_word(d,instance+0xe8,&materials))return 0;
 d->s->materials=materials;
 return nflstreet_draw_word(d,instance+0xe0,&pal)&&nflstreet_draw_palette(d,pal)&&nflstreet_draw_word(d,instance+0x5c,&count)&&nflstreet_draw_list(d,instance+0x60,count);}
/* 801F0DD4: the locked-cache DMA copies whole 32-byte blocks. */
static int dma(const context *c,nflstreet_draw *d,uint32_t to,uint32_t from,uint32_t bytes){
 const uint32_t n=(bytes+0x1fu)&~0x1fu;if(!n)return 1;uint8_t *t=mem(c,d,to,n);const uint8_t *f=mem(c,d,from,n);
 if(!t||!f)return 0;memmove(t,f,n);return 1;}
/* 801EAC78(model view, descriptor, pose) as actor_palettes.c builds a body:
   801EDE38 locked-cache enable, 801CA4B0 pool allocation (an even count),
   801CA4E8 the stack top then each cluster, into the scratch (then DMA) or
   the pool. */
static int palette(const context *c,nflstreet_draw *d,uint32_t view,uint32_t desc,uint32_t pose){
 const nflstreet_actor_palette_context *a=c->palettes;uint32_t skel,clst,scratch,count,out,used,index,base;
 if(!word(c,d,view+0xc,&clst)||!word(c,d,view+0x10,&skel))return 0;if(!skel||!clst)return 1;
 if(!word(c,d,desc+4,&scratch))return 0;
 if(scratch&&!store_byte(d,a->cache_on,1))return 0;
 uint8_t *u=nflstreet_draw_at(d,a->pool_used,4),*p=nflstreet_draw_at(d,desc,12);
 if(!u||!p||!word(c,d,clst+4,&count)||!word(c,d,a->pool_index,&index)||!word(c,d,a->pool+4*index,&base))return 0;
 used=rd32(u);const uint32_t even=(count+1u)&~1u;nflstreet_wr32(u,used+even);nflstreet_wr32(p+8,even);nflstreet_wr32(p,base+used*0x30u);out=rd32(p);
 uint32_t top,bones,translations,rotations,rows,clusters;const uint8_t *t,*poly;const uint32_t to=scratch?scratch:out;
 if(!nflstreet_matrix_top(d,&a->stacks,&top)||!(t=mem(c,d,top,0x40))||!word(c,d,skel,&bones)||!word(c,d,skel+8,&translations)||
    !word(c,d,skel+12,&rotations)||!word(c,d,clst,&rows)||!word(c,d,clst+4,&clusters)||!(poly=mem(c,d,a->stacks.sine,24)))return 0;
 if(!bones||bones>256||!clusters||clusters>256)return 0;
 const uint8_t *tr=mem(c,d,translations,bones*16u),*ro=mem(c,d,rotations,bones*6u),*rw=mem(c,d,rows,clusters*4u),*ps=pose?mem(c,d,pose,bones*6u):NULL;
 uint8_t *o=mem(c,d,to,clusters*0x30u+0x10u);if(!tr||!ro||!rw||(pose&&!ps)||!o)return 0;
 float root[12],coefficients[6],matrices[256][12];int16_t angles[256*3];
 for(unsigned i=0;i<12;++i)root[i]=fword(t+4*i);for(unsigned i=0;i<6;++i)coefficients[i]=fword(poly+4*i);
 if(ps)for(unsigned i=0;i<bones*3u;++i)angles[i]=(int16_t)(uint16_t)(ps[2*i]<<8|ps[2*i+1]);
 if(!nflstreet_cluster_palette_rows(tr,ro,rw,bones,clusters,ps?angles:NULL,root,coefficients,matrices))return 0;
 memmove(o,t,0x40);for(unsigned i=1;i<clusters;++i)for(unsigned w=0;w<12;++w)put(o+0x30u*i+4*w,matrices[i][w]);
 return !scratch||dma(c,d,out,scratch,clusters*0x30u);}
/* 801CABC4(scale, descriptor): the 3x3 of every allocated matrix scaled in
   the scratch (or in place), then the DMA (or a flush). */
static int scale_palette(const context *c,nflstreet_draw *d,uint32_t desc,float k){
 uint32_t out,scratch,count;if(!word(c,d,desc,&out)||!word(c,d,desc+4,&scratch)||!word(c,d,desc+8,&count))return 0;
 const uint32_t base=scratch?scratch:out;
 for(uint32_t i=0;i<count;++i){uint8_t *m=mem(c,d,base+0x30u*i,0x30);if(!m)return 0;
  for(unsigned r=0;r<3;++r)for(unsigned j=0;j<3;++j)put(m+16*r+4*j,fword(m+16*r+4*j)*k);}
 return !scratch||dma(c,d,out,scratch,count*0x30u);}
/* 801C6344: the next n bytes of the vertex scratch ring (restarting when
   fewer than n + 1 remain), zero-padding the cursor to 32 bytes. */
static int scratch_alloc(nflstreet_draw *d,const context *c,uint32_t n,uint32_t *out){
 uint32_t start,cur,end;if(!nflstreet_draw_word(d,c->scratch,&start)||!nflstreet_draw_word(d,c->scratch+4,&cur)||!nflstreet_draw_word(d,c->scratch+8,&end))return 0;
 const uint32_t r=end-cur>n?cur:start;cur=r+n;
 for(;cur&0x1fu;++cur)if(!store_byte(d,cur,0))return 0;
 *out=r;return store_word(d,c->scratch+4,cur);}
/* Paired-single quantized stores to s8 (GQR4 type 6, scale 0): clamped, truncated. */
static uint8_t quantize8(float v){if(v<-128.0f)v=-128.0f;if(v>127.0f)v=127.0f;return (uint8_t)(int8_t)(int32_t)v;}
/* 801C7FB4 for one chunk: S16 positions (attribute 9, type 3) through
   PSMTXMultVec and S8 normals (10, type 1) through PSMTXMultVecSR (times the
   normal scale), each vertex by its matrix-index entry, into new scratch
   buffers that become the chunk arrays' overrides. GQR5/GQR4 hold S16/S8
   with scale 0 (set by 8015668C). The stack gets an identity pushed and
   popped around each array, as the original does. */
static int skin_chunk(const context *c,nflstreet_draw *d,uint32_t chunk){
 nflstreet_render_state *s=d->s;uint32_t out,arrays,midx;if(!s->palette)return 1;
 if(!word(c,d,s->palette,&out))return 0;if(!out)return 1;
 const uint8_t *ch=nflstreet_draw_at(d,chunk,0x18);if(!ch||!nflstreet_draw_word(d,s->chunks+8,&arrays))return 0;
 const uint32_t n=ch[0x10],first=nflstreet_rd16(ch+0xe);
 for(uint32_t i=0;i<n;++i){const uint8_t *rec=nflstreet_draw_at(d,arrays+0xcu*(first+i),0xc);if(!rec)return 0;
  const unsigned attr=rec[6],type=rec[9];const uint32_t count=nflstreet_rd16(rec+4),src=rd32(rec);
  const int pos=attr==9&&type==3,nrm=attr==10&&type==1;if(!pos&&!nrm)continue;
  uint32_t buf;if(!s->overrides)return 0; /* the original would store through a null override array */
  if(!nflstreet_draw_word(d,arrays+0xcu*nflstreet_rd16(ch+0x16),&midx))return 0;
  if(!scratch_alloc(d,c,pos?count*12u:count<<3,&buf)||!store_word(d,s->overrides+4u*(first+i),buf))return 0;
  if(!nflstreet_matrix_push_identity(d,stacks(c)))return 0;
  for(uint32_t k=0;k<count;++k){const uint8_t *ix=nflstreet_draw_at(d,midx+2u*k,2);if(!ix)return 0;
   const uint8_t *m=mem(c,d,out+(uint32_t)((int32_t)(int16_t)nflstreet_rd16(ix)*0x30),0x30);if(!m)return 0;float M[12];for(unsigned w=0;w<12;++w)M[w]=fword(m+4*w);
   if(pos){const uint8_t *v=nflstreet_draw_at(d,src+6u*k,6);uint8_t *o=nflstreet_draw_at(d,buf+12u*k,12);if(!v||!o)return 0;
    const float x=(float)(int16_t)nflstreet_rd16(v),y=(float)(int16_t)nflstreet_rd16(v+2),z=(float)(int16_t)nflstreet_rd16(v+4);
    for(unsigned r=0;r<3;++r){const float *q=M+4*r;put(o+4*r,nflstreet_fmadds(q[2],z,q[0]*x)+nflstreet_fmadds(q[3],1.0f,q[1]*y));}}
   else{const uint8_t *v=nflstreet_draw_at(d,src+3u*k,3);uint8_t *o=nflstreet_draw_at(d,buf+3u*k,3);if(!v||!o)return 0;
    const float x=(float)(int8_t)v[0],y=(float)(int8_t)v[1],z=(float)(int8_t)v[2];
    for(unsigned r=0;r<3;++r){const float *q=M+4*r;o[r]=quantize8(nflstreet_fmadds(q[2],z,q[0]*x+q[1]*y)*c->normal_scale);}}}
  if(!nflstreet_matrix_pop(d,stacks(c)))return 0;}
 return 1;}
/* 801C83A0: every chunk of the current model. */
static int skin_model(const context *c,nflstreet_draw *d){
 uint32_t chunks,total;if(!d->s->chunks)return 1;
 if(!nflstreet_draw_word(d,d->s->chunks+4,&total))return 0;if(!total)return 1;if(!nflstreet_draw_word(d,d->s->chunks,&chunks))return 0;
 for(uint32_t i=0;i<total;++i)if(!skin_chunk(c,d,chunks+0x18u*i))return 0;
 return 1;}
/* The CPU-skinned path shared by 80168234 and 80168E08 (owner: the object
   whose +134 marks this frame's skinning and whose +DC is the pose). */
static int draw_skinned(const context *c,nflstreet_draw *d,uint32_t owner,uint32_t m,uint32_t view,const float modelview[16],float scale){
 uint32_t materials,pal,pose,count;uint8_t *done=nflstreet_draw_at(d,owner+0x134,1);
 if(!done||!nflstreet_draw_word(d,m+0xf4,&materials))return 0;d->s->materials=materials;
 if(!refresh(d,m+0xc))return 0;
 if(!*done){uint32_t overrides;*done=1;if(!nflstreet_draw_word(d,view+0x1c,&overrides)||!overrides)return 0; /* 801C6BF4 */
  if(!nflstreet_draw_select_model(d,view)||!nflstreet_draw_word(d,m+0xec,&pal)||!nflstreet_draw_palette(d,pal)||
     !nflstreet_matrix_push_identity(d,stacks(c))||!nflstreet_draw_word(d,owner+0xdc,&pose)||!palette(c,d,view,m+0xfc,pose)||
     !scale_palette(c,d,m+0xfc,scale)||!nflstreet_matrix_pop(d,stacks(c))||!skin_model(c,d))return 0;}
 return GX(d,NFLSTREET_GX_SET_CURRENT_MTX,0)&&load_matrix(d,modelview)&&GX(d,NFLSTREET_GX_SET_NUM_TEX_GENS,1)&&GX(d,NFLSTREET_GX_SET_NUM_TEV_STAGES,1)&&
  GX(d,NFLSTREET_GX_SET_TEV_ORDER,0,0,0,4)&&nflstreet_draw_word(d,m+0x68,&count)&&nflstreet_draw_list(d,m+0x6c,count);}
/* The first chunk of the model view needs CPU skinning (flags bit 3). */
static int cpu_skinned(nflstreet_draw *d,uint32_t view,int *out){
 uint32_t table,chunks;const uint8_t *f;if(!nflstreet_draw_word(d,view+8,&table)||!nflstreet_draw_word(d,table,&chunks)||!(f=nflstreet_draw_at(d,chunks+0x11,1)))return 0;
 *out=(*f&8)!=0;return 1;}
/* 801EC048..801EC310 on a local instance record: only 801EC06C touches RAM
   (the stack slot above the top receives top x model). */
static int local_instance(nflstreet_draw *d,const context *c,const float model[16]){
 return nflstreet_matrix_push(d,stacks(c))&&concat_local(d,c,model,4)&&nflstreet_matrix_pop(d,stacks(c));}
static int draw_copy(const context *c,nflstreet_draw *d,uint32_t copy,unsigned depth);
/* 80168234(object): a spectator or prop and its copies. */
static int draw_model(const context *c,nflstreet_draw *d,uint32_t o){
 const nflstreet_matrix_stacks *k=stacks(c);uint32_t m,view,source,codes,flags,first,child;float model[16],modelview[16],y;int hide,held,skinned;
 if(!nflstreet_draw_word(d,o+0x128,&m)||!nflstreet_draw_word(d,m+0x18,&view)||!nflstreet_draw_word(d,o+0x144,&source))return 0;
 if(!lights(d,source,c->light_default[0])||!GX(d,NFLSTREET_GX_SET_Z_MODE,1,3,1)||!GX(d,NFLSTREET_GX_SET_CULL_MODE,1)||!nflstreet_draw_blend(d,2))return 0;
 if(!nflstreet_matrix_select(d,k)||!nflstreet_matrix_push(d,k)||!nflstreet_matrix_push_identity(d,k)||!place(d,c,o+4,o+0x18,o)||!top16(d,c,model)||
    !nflstreet_matrix_pop(d,k)||!place(d,c,o+4,o+0x18,o)||!top16(d,c,modelview))return 0;
 if(!load_matrix(d,modelview)||!GX(d,NFLSTREET_GX_SET_CURRENT_MTX,0)||!cull(d,c,model,&codes)||!fvalue(d,o+8,&y)||!hidden(d,c,o,y,c->hide_sign[0],&hide))return 0;
 if(hide)codes|=0x3f;
 if(!store_byte(d,o+0x134,0))return 0;
 if(!(codes&0x3f)){uint8_t *fade;uint32_t selector;
  if(!local_instance(d,c,model)||!nflstreet_play_held(d,c->scene,&held))return 0;
  if(!held&&(!nflstreet_draw_word(d,o+0x138,&selector)||!select_group(d,m+0x20,selector)))return 0;
  if(!(fade=nflstreet_draw_at(d,o+0x13c,1))||*fade)return 0; /* the distance fade is not modeled */
  if(!store_word(d,m+0x68,1)||!nflstreet_draw_word(d,o+0xe8,&flags))return 0;
  if(flags&1){
   if(!nflstreet_draw_word(d,o+0x12c,&first))return 0;
   if(!first){if(!draw_instance(d,m+0xc))return 0;}
   else{if(!cpu_skinned(d,view,&skinned))return 0;
    if(skinned){if(!draw_skinned(c,d,o,m,view,modelview,c->palette_scale[0]))return 0;}
    else{uint32_t pose;if(!store_byte(d,o+0x134,1)||!nflstreet_draw_word(d,o+0xdc,&pose)||!palette(c,d,view,m+0xfc,pose)||!draw_instance(d,m+0xc))return 0;}}}}
 if(!nflstreet_matrix_pop(d,k)||!GX(d,NFLSTREET_GX_SET_CULL_MODE,0)||!nflstreet_draw_word(d,o+0x1d8,&child))return 0;
 return !child||draw_copy(c,d,child,0);}
/* 80168E08(copy): another placement of its owner's (+50) model, at its own
   translation +30 and quaternion +3C, sharing the owner's skinning. */
static int draw_copy(const context *c,nflstreet_draw *d,uint32_t o,unsigned depth){
 const nflstreet_matrix_stacks *k=stacks(c);uint32_t owner,m,view,source,codes,flags,next;float model[16],modelview[16],y;int hide,skinned;
 if(depth>=64)return 0;
 if(!nflstreet_draw_word(d,o+0x50,&owner)||!nflstreet_draw_word(d,owner+0x128,&m)||!nflstreet_draw_word(d,m+0x18,&view)||!nflstreet_draw_word(d,o+0x58,&source))return 0;
 if(!lights(d,source,c->light_default[1])||!GX(d,NFLSTREET_GX_SET_CULL_MODE,1)||!nflstreet_draw_blend(d,1))return 0;
 if(!nflstreet_matrix_select(d,k)||!nflstreet_matrix_push(d,k)||!nflstreet_matrix_push_identity(d,k)||!place(d,c,o+0x30,o+0x3c,owner)||!top16(d,c,model)||
    !nflstreet_matrix_pop(d,k)||!nflstreet_matrix_push(d,k)||!place(d,c,o+0x30,o+0x3c,owner)||!top16(d,c,modelview)||!cull(d,c,model,&codes)||
    !nflstreet_matrix_pop(d,k)||!fvalue(d,o+0x34,&y)||!hidden(d,c,owner,y,c->hide_sign[1],&hide))return 0;
 if(hide)codes|=0x3f;
 if(!(codes&0x3f)){
  if(!local_instance(d,c,model)||!store_word(d,m+0x68,1)||!nflstreet_draw_word(d,owner+0xe8,&flags))return 0;
  if(flags&1){if(!cpu_skinned(d,view,&skinned))return 0;
   if(skinned){if(!draw_skinned(c,d,owner,m,view,modelview,c->palette_scale[1]))return 0;}
   else{uint8_t *done=nflstreet_draw_at(d,owner+0x134,1);uint32_t pose;if(!done)return 0;
    if(!*done){uint32_t top;if(!nflstreet_matrix_top(d,k,&top))return 0;uint8_t *t=nflstreet_draw_at(d,top,0x40);if(!t)return 0;
     for(unsigned i=0;i<16;++i)put(t+4*i,modelview[i]);*done=1; /* 8018962C(model-view) */
     if(!nflstreet_draw_word(d,owner+0xdc,&pose)||!palette(c,d,view,m+0xfc,pose))return 0;}
    if(!draw_instance(d,m+0xc))return 0;}}}
 if(!nflstreet_matrix_pop(d,k)||!GX(d,NFLSTREET_GX_SET_CULL_MODE,0)||!nflstreet_draw_word(d,o+0x5c,&next))return 0;
 return !next||draw_copy(c,d,next,depth+1);}
int nflstreet_crowd_draw(const context *c,uint32_t object,nflstreet_render_state *s){
 if(!c||!c->scene||!c->scene->draw||!c->palettes||!s||!object)return 0;
 nflstreet_draw d={c->scene->draw,s};const uint8_t *on=nflstreet_draw_at(&d,c->enable,1);uint32_t hook;
 if(!on)return 0;if(!*on)return 1;
 if(!nflstreet_draw_word(&d,object+0x14,&hook))return 0;
 if(hook==0x800d2860u)return 1; /* li r3,0; blr */
 if(hook==0x80168234u)return draw_model(c,&d,object);
 return 0;}
