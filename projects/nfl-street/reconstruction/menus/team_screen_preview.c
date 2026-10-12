/* Render post-startup team-menu 2D source objects to a private BMP.
   The host rasterizer is diagnostic; it is not a claim of original GX output. */
#include <windows.h>
#include "team_screen.h"
#include "screen.h"
#include "text.h"
#include "team_player_resolve.h"
#include "team_effects.h"
#include "team_player_selected_head.h"
#include "team_player_selected_pair.h"
#include "texture.h"
#include <windows.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BASE 0x81100000u
#define IMAGE_CAPACITY 1024u
#define DEFAULT_CAPTURE_DIR "W:\\Daywater\\research-temp\\nflstreet-team-screen-preview-20261011"
typedef struct { unsigned archive,entry; nflstreet_texture image; } image_entry;
typedef struct { float a,b,c,d,x,y,color[8]; } transform;
typedef struct { float x,y,u,v,color[4]; } vertex;
static uint8_t *screen_bytes;
static size_t screen_size;
static nflstreet_team_screen *team_state;
static nflstreet_texture canvas,font_atlas[4][2];
static unsigned head_inspection,player_inspection;
static uint8_t *plamodel_bytes;static size_t plamodel_size;
static uint8_t *face_bytes,*hair_bytes;
static nflstreet_team_player_resolver player_resolver;
static nflstreet_texture head_images[14];
static uint32_t head_player_ids[14],head_team_ids[14];
static unsigned heads_ready,head_visible_slot=14;
static unsigned inspect_mode,live_mode,live_check,live_failed,live_ticks,live_paints;
static unsigned source_controls;
static unsigned dispatcher_context;
static unsigned quick_game_start;
static unsigned source_audit,source_audit_index;
static const char *source_events;
static unsigned focus_node=UINT32_MAX;
static const char *focus_nodes;
static int inspect_host(nflstreet_team_screen*s){
 if(!inspect_mode&&!source_controls&&!source_events&&!quick_game_start&&focus_node==UINT32_MAX&&!focus_nodes)return 1;uint32_t map[10];for(unsigned i=0;i<10;++i)map[i]=UINT32_MAX;map[0]=0;
 const nflstreet_team_device_record record={0,0};
 return nflstreet_team_screen_devices(s,&record,1,map);
}
static int inspect_controls(uint8_t*b,size_t n,nflstreet_team_screen*s){
 if(!inspect_mode)return 1;
 if(!nflstreet_team_screen_input(b,n,BASE,s,0,6,1.0f))return 0;
 for(unsigned i=0;i<60;++i)if(!nflstreet_team_screen_advance(b,n,BASE,s,16))return 0;
 if(inspect_mode==2){
  if(!nflstreet_team_screen_input(b,n,BASE,s,0,7,1.0f))return 0;
  for(unsigned i=0;i<60;++i)if(!nflstreet_team_screen_advance(b,n,BASE,s,16))return 0;
 }
 unsigned selected[2]={0};for(unsigned side=0;side<2;++side)for(unsigned i=0;i<s->activation.sides[side].count;++i)selected[side]+=s->activation.sides[side].roster[i].selected>=0;
 printf("Team source inspection mode%u: selected rows%u/%u; preview requests%u; logical device supplied; no physical input claim.\n",inspect_mode,selected[0],selected[1],s->player_spawn_count);
 return 1;
}
/* Host F2..F7 controls expose translated source events explicitly. They do
   not assert an original GameCube-to-keyboard mapping or complete navigation. */
static int audit_source(unsigned event,const char*stage){
 if(!source_audit)return 1;
 const char*prefix=getenv("DAYWATER_TEAM_CAPTURE_PREFIX");char path[4096];
 if(!prefix||strncmp(prefix,"W:\\Daywater\\",12))return 0;
 int z=snprintf(path,sizeof path,"%s-event%u-%u-%s.bin",prefix,source_audit_index,event,stage);
 if(z<0||z>=(int)sizeof path)return 0;FILE*f=fopen(path,"wb");if(!f)return 0;
 int ok=fwrite(screen_bytes,1,screen_size,f)==screen_size;return fclose(f)==0&&ok;
}
static int source_input(unsigned event){
 if(!audit_source(event,"before"))return 0;
 int accepted=dispatcher_context?
  nflstreet_team_screen_input_context(screen_bytes,screen_size,BASE,team_state,0,(uint16_t)event,1.0f,7,UINT32_MAX,0x817f2380u):
  nflstreet_team_screen_input(screen_bytes,screen_size,BASE,team_state,0,(uint16_t)event,1.0f);
 if(!accepted){
  printf("Source event%u rejected; screen state retained.\n",event);fflush(stdout);return 0;
 }
 if(!audit_source(event,"after"))return 0;++source_audit_index;
 printf("Source event%u accepted; selected team IDs%u/%u; pending focus%u; athlete requests%u.\n",event,team_state->game.startup.selected_id[0],team_state->game.startup.selected_id[1],team_state->classes.shared.pending_count,team_state->player_spawn_count);
 printf("Source carousel windows: %u/%u/%u and %u/%u/%u.\n",team_state->game.startup.carousel.windows[0][0],team_state->game.startup.carousel.windows[0][1],team_state->game.startup.carousel.windows[0][2],team_state->game.startup.carousel.windows[1][0],team_state->game.startup.carousel.windows[1][1],team_state->game.startup.carousel.windows[1][2]);
 fflush(stdout);return 1;
}
static int replay_source_inputs(void){
 if(focus_node!=UINT32_MAX){nflstreet_ui_focus_effects effects={0};
  if(!nflstreet_team_screen_focus_context(screen_bytes,screen_size,BASE,team_state,focus_node,1,0,dispatcher_context?7:0,dispatcher_context?UINT32_MAX:7,0x817f2380u,0,&effects))return 0;
  printf("Supplied focus node%u: %u ordered source scripts, VM current%08x/link%08x.\n",focus_node,effects.count,dispatcher_context?7:0,dispatcher_context?UINT32_MAX:7);
 }
 if(focus_nodes){const char*p=focus_nodes;unsigned count=0;
  do{char*end;unsigned long node=strtoul(p,&end,10);if(end==p||node>=1024||++count>32)return 0;
   nflstreet_ui_focus_effects effects={0};
   if(!nflstreet_team_screen_focus_context(screen_bytes,screen_size,BASE,team_state,(unsigned)node,1,0,dispatcher_context?7:0,dispatcher_context?UINT32_MAX:7,0x817f2380u,0,&effects))return 0;
   printf("Supplied focus node%lu: %u ordered source scripts, VM current%08x/link%08x.\n",node,effects.count,dispatcher_context?7:0,dispatcher_context?UINT32_MAX:7);
   if(!*end)break;if(*end!=','||!end[1])return 0;p=end+1;
  }while(*p);
 }
 if(!source_events)return 1;const char*p=source_events;unsigned count=0;
 while(*p){if(*p<'2'||*p>'7'||++count>32)return 0;unsigned event=(unsigned)(*p++-'0');
  if(*p&&*p++!=',')return 0;if(!source_input(event))return 0;
  for(unsigned i=0;i<60;++i)if(!nflstreet_team_screen_advance(screen_bytes,screen_size,BASE,team_state,16))return 0;
  if(!*p&&p[-1]==',')return 0;
 }
 printf("Source replay after updates: selected team IDs%u/%u; lineup sizes%u/%u; athlete requests%u.\n",team_state->game.startup.selected_id[0],team_state->game.startup.selected_id[1],team_state->activation.sides[0].selected_count,team_state->activation.sides[1].selected_count,team_state->player_spawn_count);
 return count!=0;
}
static image_entry images[IMAGE_CAPACITY];
static unsigned image_count,budget;
static const char *unpacked_root;

static unsigned be32(const uint8_t*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static unsigned be16(const uint8_t*p){return (unsigned)p[0]<<8|p[1];}
static float fl(const uint8_t*p){unsigned w=be32(p);float f;memcpy(&f,&w,4);return f;}
static uint8_t *ptr(unsigned a,size_t z){if(a<BASE||(size_t)(a-BASE)>screen_size||z>screen_size-(a-BASE))return NULL;return screen_bytes+a-BASE;}
static uint8_t *read_file(const char*p,size_t*n){FILE*f=fopen(p,"rb");if(!f)return NULL;if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}long z=ftell(f);if(z<1||z>128*1024*1024||fseek(f,0,SEEK_SET)){fclose(f);return NULL;}uint8_t*b=malloc((size_t)z);if(!b||fread(b,1,(size_t)z,f)!=(size_t)z){free(b);fclose(f);return NULL;}fclose(f);*n=(size_t)z;return b;}

static const char *archive_folder(unsigned a,unsigned *count){
 switch(a){
 case 1:*count=16;return "uis_comn"; case 3:*count=13;return "uis_butt";
 case 4:*count=102;return "uis_tmlo";
 case 7:*count=3;return "uis_bgmp"; case 8:*count=25;return "uis_bgpl";
 case 9:*count=4;return "uis_ctrl"; case 10:*count=11;return "uis_fron";
 case 11:*count=41;return "uis_fsel"; case 13:*count=549;return "uis_port";
 case 14:*count=11;return "uis_gabr"; case 15:*count=18;return "uis_ingm";
 case 16:*count=56;return "uis_crtm"; case 18:*count=19;return "uis_chal";
 case 20:*count=22;return "uis_movi"; default:*count=0;return NULL;
 }
}
static const nflstreet_texture *image_for(unsigned a,unsigned e){for(unsigned i=0;i<image_count;++i)if(images[i].archive==a&&images[i].entry==e)return &images[i].image;return NULL;}
/* Preview lifetime cache: releases retire a request, while decoded images
   remain resident until shutdown. This is a host ownership policy, not the
   original archive reference-count implementation. */
static int acquire_image(unsigned a,unsigned e){
 unsigned count=0;const char*folder=archive_folder(a,&count);
 if(!folder||e>=count){fprintf(stderr,"Unsupported team UI archive=%u entry=%u\n",a,e);return 0;}
 if(image_for(a,e))return 1;if(image_count==IMAGE_CAPACITY)return 0;
 char path[4096];int z=snprintf(path,sizeof path,"%s/%s/%05u.bin",unpacked_root,folder,e);if(z<0||z>=(int)sizeof path)return 0;
 size_t raw_size=0;uint8_t*raw=read_file(path,&raw_size);image_entry*dst=&images[image_count];
 if(!raw||!nflstreet_texture_open(raw,raw_size,&dst->image)){fprintf(stderr,"Cannot decode team UI image %u/%u\n",a,e);free(raw);return 0;}
 free(raw);dst->archive=a;dst->entry=e;++image_count;return 1;
}
static int load_images(void){
 nflstreet_team_classes*o=&team_state->objects;
 for(unsigned i=0;i<o->request_count;++i){const nflstreet_ui_service_request*q=&o->requests[i];if(q->kind||q->class_id||q->property!=UINT32_MAX)continue;
  uint8_t*object=ptr(q->target,36);if(!object)return 0;unsigned flags=be16(object),e=be16(object+4),a=be16(object+6);
  if(!(flags&2)||e==65535||a==65535)continue;if(!acquire_image(a,e))return 0;
 }
 nflstreet_team_effect_batch batch;nflstreet_team_effect_ack ack={0};
 if(!nflstreet_team_effects_capture(team_state,&batch))return 0;
 for(unsigned i=0;i<batch.counts[NFLSTREET_TEAM_EFFECT_RESOURCES];++i){
  const nflstreet_ui_button_effect*q=batch.resources+i;unsigned count=0;
  if(q->kind>1||!archive_folder(q->archive,&count)||q->entry>=count)return 0;
  if(q->kind==1&&!acquire_image(q->archive,q->entry))return 0;
 }
 if(batch.counts[NFLSTREET_TEAM_EFFECT_RESOURCES]&&(source_controls||source_events))
  printf("Host UI lifetime cache: consumed%u runtime image requests; retained%u decoded images.\n",batch.counts[NFLSTREET_TEAM_EFFECT_RESOURCES],image_count);
 ack.count[NFLSTREET_TEAM_EFFECT_RESOURCES]=batch.counts[NFLSTREET_TEAM_EFFECT_RESOURCES];
 return nflstreet_team_effects_ack(team_state,&batch,&ack);
}
static int load_heads(void){
 if(!head_inspection)return 1;
 if(!heads_ready){
  char path[4096];size_t fn=0,hn=0;
  int z=snprintf(path,sizeof path,"%s/../assets/plaface.dat",unpacked_root);
  if(z<0||z>=(int)sizeof path)return 0;face_bytes=read_file(path,&fn);
  z=snprintf(path,sizeof path,"%s/../assets/plarosterhair.dat",unpacked_root);
  if(z<0||z>=(int)sizeof path)return 0;hair_bytes=read_file(path,&hn);
  if(!face_bytes||!hair_bytes||!nflstreet_team_player_resolver_open(face_bytes,fn,hair_bytes,hn,&player_resolver))return 0;
  if(player_inspection){
   z=snprintf(path,sizeof path,"%s/../assets/plamodel.dat",unpacked_root);
   if(z<0||z>=(int)sizeof path)return 0;plamodel_bytes=read_file(path,&plamodel_size);if(!plamodel_bytes)return 0;
  }
  heads_ready=1;
 }
 for(unsigned i=0;i<team_state->player_spawn_count;++i){
  const nflstreet_team_player_spawn_request*q=&team_state->player_spawns[i];unsigned slot=q->actor_index;
  if(slot>=14)return 0;if(head_images[slot].bgra&&head_player_ids[slot]==q->player_id&&head_team_ids[slot]==q->team_id)continue;
  unsigned db=32;for(unsigned k=0;k<team_state->supplied_roster_count;++k)if(team_state->supplied_rosters[k].id==q->team_id)db=k;
  nflstreet_team_player_manifest m;nflstreet_team_player_head head={0};nflstreet_texture image={0};
  if(db==32||!nflstreet_team_player_resolve(&player_resolver,team_state->roster_databases[db],team_state->roster_database_sizes[db],q->team_id,q->player_id,&m)||
     !m.custom_name_branch||!nflstreet_team_player_head_open(&player_resolver.face,(unsigned)m.head_runtime_index,&head)||
     !nflstreet_team_player_head_render(&head,256,256,0.6f,&image)){
   nflstreet_team_player_head_close(&head);nflstreet_texture_close(&image);return 0;
  }
  printf("Source preview slot%u PLAY%u head index%d triangles%zu; candidate stored-pose inspection.\n",slot,q->player_id,m.head_runtime_index,head.mesh.count);
  if(player_inspection){
   nflstreet_team_player_selected_pair pair={0};nflstreet_texture combined={0};
   if(!nflstreet_team_player_selected_pair_open(plamodel_bytes,plamodel_size,m.tbyp,&player_resolver.face,(unsigned)m.head_runtime_index,&pair)||
      !nflstreet_team_player_selected_pair_render_combined(&pair,256,320,0.6f,&combined)){
    nflstreet_team_player_selected_pair_close(&pair);nflstreet_team_player_head_close(&head);nflstreet_texture_close(&image);return 0;
   }
   printf("Source preview slot%u PLAY%u body key%u entry%u triangles%zu; common stored-pose candidate, no material/gear fidelity.\n",slot,q->player_id,pair.body_binding.pool_key,pair.body_binding.archive_index,pair.body_mesh.count);
   nflstreet_team_player_selected_pair_close(&pair);nflstreet_texture_close(&image);image=combined;
  }
  nflstreet_team_player_head_close(&head);nflstreet_texture_close(head_images+slot);head_images[slot]=image;head_player_ids[slot]=q->player_id;head_team_ids[slot]=q->team_id;
 }
 nflstreet_team_effect_batch batch;nflstreet_team_effect_ack ack={0};
 if(!nflstreet_team_effects_capture(team_state,&batch))return 0;
 ack.count[NFLSTREET_TEAM_EFFECT_PLAYER_SPAWNS]=batch.counts[NFLSTREET_TEAM_EFFECT_PLAYER_SPAWNS];
 /* This optional host has consumed factory descriptions into exact head
    inspection owners. It does not acknowledge unfinished body/audio effects. */
 return nflstreet_team_effects_ack(team_state,&batch,&ack);
}
static void close_heads(void){
 for(unsigned i=0;i<14;++i)nflstreet_texture_close(head_images+i);
 free(face_bytes);free(hair_bytes);free(plamodel_bytes);face_bytes=hair_bytes=plamodel_bytes=NULL;heads_ready=0;plamodel_size=0;
}
static transform combine(transform p,const uint8_t*r){
 float angle=fl(r+40)*0.017453292519943295f,sx=fl(r+44),sy=fl(r+48),cs=cosf(angle),sn=sinf(angle),x=fl(r+8),y=fl(r+12);transform t=p;
 t.a=p.a*cs*sx+p.c*sn*sx;t.b=p.b*cs*sx+p.d*sn*sx;t.c=-p.a*sn*sy+p.c*cs*sy;t.d=-p.b*sn*sy+p.d*cs*sy;t.x=p.x+p.a*x+p.c*y;t.y=p.y+p.b*x+p.d*y;
 for(unsigned i=0;i<4;++i){t.color[i]=p.color[i]*fl(r+60+4*i);t.color[4+i]=p.color[4+i]+fl(r+76+4*i);}return t;
}
static void blend(int x,int y,unsigned src,const float*c){
 if(x<0||y<0||(unsigned)x>=canvas.width||(unsigned)y>=canvas.height)return;float alpha=(src>>24)/255.0f*c[3];if(!isfinite(alpha)||alpha<=0)return;if(alpha>1)alpha=1;
 uint32_t*dst=canvas.bgra+(size_t)y*canvas.width+x,v=0xff000000u;for(unsigned k=0;k<3;++k){float s=((src>>(k*8))&255)*c[2-k];if(s<0)s=0;if(s>255)s=255;v|=(unsigned)(s*alpha+((*dst>>(k*8))&255)*(1-alpha))<<(k*8);}*dst=v;
}
static void sprite(const nflstreet_texture*tex,float x,float y,unsigned u,unsigned v,unsigned w,unsigned h,const float*c,transform t){
 if(!tex||!tex->bgra||!w||!h)return;float det=t.a*t.d-t.b*t.c;if(!isfinite(det)||fabsf(det)<1e-8f)return;float xx[4],yy[4];
 for(unsigned i=0;i<4;++i){float lx=x+(i&1?w:0),ly=y+(i&2?h:0);xx[i]=t.x+t.a*lx+t.c*ly;yy[i]=t.y+t.b*lx+t.d*ly;}
 float minx=xx[0],maxx=xx[0],miny=yy[0],maxy=yy[0];for(unsigned i=1;i<4;++i){minx=fminf(minx,xx[i]);maxx=fmaxf(maxx,xx[i]);miny=fminf(miny,yy[i]);maxy=fmaxf(maxy,yy[i]);}
 int x0=(int)fmaxf(0,floorf(minx)),x1=(int)fminf(canvas.width,ceilf(maxx)),y0=(int)fmaxf(0,floorf(miny)),y1=(int)fminf(canvas.height,ceilf(maxy));
 for(int py=y0;py<y1;++py)for(int px=x0;px<x1;++px){float dx=px+.5f-t.x,dy=py+.5f-t.y,lx=(t.d*dx-t.c*dy)/det-x,ly=(-t.b*dx+t.a*dy)/det-y;
  if(lx>=0&&ly>=0&&lx<w&&ly<h&&u+(unsigned)lx<tex->width&&v+(unsigned)ly<tex->height)blend(px,py,tex->bgra[(v+(unsigned)ly)*tex->width+u+(unsigned)lx],c);}
}
static void triangle(const nflstreet_texture*tex,const vertex*a,const vertex*b,const vertex*c){
 float den=(b->y-c->y)*(a->x-c->x)+(c->x-b->x)*(a->y-c->y);if(!isfinite(den)||fabsf(den)<1e-8f)return;
 int x0=(int)fmaxf(0,floorf(fminf(a->x,fminf(b->x,c->x)))),x1=(int)fminf(canvas.width,ceilf(fmaxf(a->x,fmaxf(b->x,c->x))));
 int y0=(int)fmaxf(0,floorf(fminf(a->y,fminf(b->y,c->y)))),y1=(int)fminf(canvas.height,ceilf(fmaxf(a->y,fmaxf(b->y,c->y))));
 for(int y=y0;y<y1;++y)for(int x=x0;x<x1;++x){float p=((b->y-c->y)*(x+.5f-c->x)+(c->x-b->x)*(y+.5f-c->y))/den,q=((c->y-a->y)*(x+.5f-c->x)+(a->x-c->x)*(y+.5f-c->y))/den,r=1-p-q;if(p<0||q<0||r<0)continue;
  float color[4];for(unsigned j=0;j<4;++j)color[j]=p*a->color[j]+q*b->color[j]+r*c->color[j];unsigned source=0xffffffffu;
  if(tex){float u=p*a->u+q*b->u+r*c->u,v=p*a->v+q*b->v+r*c->v;unsigned tx=(unsigned)fmaxf(0,fminf(tex->width-1,u*tex->width)),ty=(unsigned)fmaxf(0,fminf(tex->height-1,v*tex->height));source=tex->bgra[(size_t)ty*tex->width+tx];}blend(x,y,source,color);}
}
static int draw_text(unsigned target,transform t){
 uint8_t*o=ptr(target,40);if(!o)return 0;unsigned fi=o[2];if(fi>=4)return 0;nflstreet_font*f=&team_state->objects.fonts[fi];if(!f->bytes)return 0;
 uint8_t*d=ptr(be32(o+32),12),*text=d?ptr(be32(d+8),1):NULL;if(!text||!memchr(text,0,screen_size-(size_t)(text-screen_bytes)))return 0;
 if(!nflstreet_team_classes_prepare_text(&team_state->objects,screen_bytes,screen_size,BASE,target))return 0;
 nflstreet_ui_text_plan p;nflstreet_glyph_box glyphs[354];unsigned count;if(!nflstreet_ui_text_draw(o,(char*)text,t.color,&p)||!nflstreet_font_layout(f,(char*)text,p.flags,0,glyphs,354,&count))return 0;
 if(p.visible)for(unsigned pass=0;pass<2;++pass)for(unsigned i=0;i<count;++i){nflstreet_glyph_box*g=glyphs+i;sprite(&font_atlas[fi][pass],p.position[0]+g->x+(pass?0:p.shadow_offset[0]),p.position[1]+g->y+(pass?0:p.shadow_offset[1]),g->u,g->v,g->width,g->height,p.color[pass?0:1],t);}return 1;
}
static int draw_image(unsigned target,transform t){
 uint8_t*o=ptr(target,76);if(!o)return 0;unsigned flags=be16(o);if(flags&0x8000)return 0;const nflstreet_texture*tex=NULL;
 if(flags&2){unsigned a=be16(o+6),e=be16(o+4);tex=image_for(a,e);if(!tex){fprintf(stderr,"Missing image %u/%u\n",a,e);return 0;}}
 vertex v[4];for(unsigned i=0;i<4;++i){float x=(int16_t)be16(o+52+i*6)/16.0f,y=(int16_t)be16(o+54+i*6)/16.0f;v[i].x=t.x+t.a*x+t.c*y;v[i].y=t.y+t.b*x+t.d*y;v[i].u=(int16_t)be16(o+36+i*4)/256.0f;v[i].v=(int16_t)be16(o+38+i*4)/256.0f;for(unsigned j=0;j<4;++j)v[i].color[j]=(o[20+i*4+j]+t.color[4+j])*.003921568859368563f*t.color[j];}
 triangle(tex,v,v+1,v+2);triangle(tex,v,v+2,v+3);return 1;
}
static int select_head(unsigned target){
 uint8_t*o=ptr(target,20);if(!o)return 0;unsigned slot=be32(o);
 if(slot<14&&head_images[slot].bgra&&head_team_ids[slot]==team_state->game.startup.selected_id[slot/7])head_visible_slot=slot;
 /* Original801289A4/8012888C do not interpret the object's four float
    words as a 2D viewport. Keep this host inspection in a separate pane. */
 return 1;
}
static int node(unsigned index,transform parent){
 if(!budget--||index>=be32(screen_bytes))return 0;uint8_t*rows=ptr(be32(screen_bytes+4),(size_t)be32(screen_bytes)*20);if(!rows)return 0;
 uint8_t*r=rows+index*20,*root=ptr(be32(r),100);if(!root)return 0;if(!be32(root))return 1;transform t=combine(parent,root);for(unsigned i=0;i<8;++i)if(!isfinite(t.color[i]))return 0;
 unsigned count=be32(r+4);uint8_t*groups=ptr(be32(r+8),(size_t)count*4);if(!groups)return 0;for(unsigned i=0;i<count;++i){uint8_t*g=ptr(be32(groups+4*i),12),*gr=g?ptr(be32(g),4):NULL;if(!gr)return 0;if(!be32(gr))continue;unsigned links=be32(g+4);uint8_t*l=ptr(be32(g+8),(size_t)links*8);if(!l)return 0;
  for(unsigned j=0;j<links;++j){unsigned cls=be16(l+8*j);if(cls==65535){if(!node(be32(l+8*j+4),t))return 0;}else if(be16(l+8*j+2)&&(cls==0||cls==8||(cls==3&&head_inspection))){uint8_t*cell=ptr(be32(l+8*j+4),4);if(!cell||be32(cell)>UINT32_MAX-BASE)return 0;unsigned target=BASE+be32(cell);if(cls==0?!draw_image(target,t):cls==3?!select_head(target):!draw_text(target,t)){fprintf(stderr,"Unsupported team source object class%u target%x\n",cls,target);return 0;}}}
 }return 1;
}
static int render(void){head_visible_slot=14;for(size_t i=0;i<(size_t)canvas.width*canvas.height;++i)canvas.bgra[i]=0xff202020u;transform t={0};t.a=t.d=1;for(unsigned i=0;i<4;++i)t.color[i]=1;budget=8192;return node(0,t);}
static LRESULT CALLBACK live_proc(HWND window,UINT message,WPARAM key,LPARAM detail){
 (void)detail;
 if(message==WM_DESTROY){PostQuitMessage(0);return 0;}
 if(message==WM_KEYDOWN&&key==VK_ESCAPE){DestroyWindow(window);return 0;}
 if(message==WM_KEYDOWN&&source_controls&&key>=VK_F2&&key<=VK_F7){
  if(detail&((LPARAM)1<<30))return 0; /* One source event per host press. */
  if(source_input((unsigned)(key-VK_F2)+2)){
   if(!load_images()||!load_heads()||!render()){live_failed=1;DestroyWindow(window);return 0;}
   InvalidateRect(window,NULL,FALSE);
  }return 0;
 }
 if(message==WM_TIMER){
  if(!nflstreet_team_screen_advance(screen_bytes,screen_size,BASE,team_state,16)||!load_images()||!load_heads()||!render()){
   live_failed=1;KillTimer(window,1);DestroyWindow(window);return 0;
  }
  ++live_ticks;InvalidateRect(window,NULL,FALSE);
  if(live_check&&live_ticks==60){KillTimer(window,1);DestroyWindow(window);}
  return 0;
 }
 if(message==WM_PAINT){
  PAINTSTRUCT paint;HDC dc=BeginPaint(window,&paint);RECT rect;GetClientRect(window,&rect);
  BITMAPINFO info={0};info.bmiHeader.biSize=sizeof info.bmiHeader;
  info.bmiHeader.biWidth=(LONG)canvas.width;info.bmiHeader.biHeight=-(LONG)canvas.height;
  info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
  int pane=head_inspection?256:0,menu_width=rect.right-pane;
  if(menu_width<1)menu_width=1;
  int painted=StretchDIBits(dc,0,0,menu_width,rect.bottom,0,0,(int)canvas.width,(int)canvas.height,canvas.bgra,&info,DIB_RGB_COLORS,SRCCOPY);
  if(head_inspection){RECT pane_rect={menu_width,0,rect.right,rect.bottom};FillRect(dc,&pane_rect,(HBRUSH)(COLOR_WINDOW+1));}
  if(head_inspection&&head_visible_slot<14){
   info.bmiHeader.biWidth=256;info.bmiHeader.biHeight=-(LONG)head_images[head_visible_slot].height;
   int head_painted=StretchDIBits(dc,menu_width,48,256,(int)head_images[head_visible_slot].height,0,0,256,(int)head_images[head_visible_slot].height,head_images[head_visible_slot].bgra,&info,DIB_RGB_COLORS,SRCCOPY);
   if(head_painted==0||head_painted==(int)GDI_ERROR)live_failed=1;
   const char*label=player_inspection?"Selected player geometry":"Selected head inspection";TextOutA(dc,menu_width+8,16,label,(int)strlen(label));
  }
  if(painted==0||painted==(int)GDI_ERROR)live_failed=1;else ++live_paints;
  EndPaint(window,&paint);return 0;
 }
 return DefWindowProcA(window,message,key,detail);
}
static int live_window(void){
 HINSTANCE instance=GetModuleHandleA(NULL);WNDCLASSA cls={0};cls.lpfnWndProc=live_proc;
 cls.hInstance=instance;cls.lpszClassName="NFLStreetTeamSource";cls.hCursor=LoadCursor(NULL,IDC_ARROW);
 if(!RegisterClassA(&cls))return 0;
 HWND window=CreateWindowA(cls.lpszClassName,player_inspection?"NFL Street native team screen - player geometry inspection":head_inspection?"NFL Street native team screen - selected head inspection":"NFL Street native team screen",WS_OVERLAPPEDWINDOW,
  CW_USEDEFAULT,CW_USEDEFAULT,head_inspection?1156:900,player_inspection?780:720,NULL,NULL,instance,NULL);
 if(!window){UnregisterClassA(cls.lpszClassName,instance);return 0;}
 if(source_controls)printf("Host inspection controls: F2..F7 send source events2..7 on supplied logical port0; Escape closes. Original physical mapping and complete navigation remain unverified.\n");
 ShowWindow(window,SW_SHOW);UpdateWindow(window);
 if(!SetTimer(window,1,16,NULL)){DestroyWindow(window);live_failed=1;}
 MSG message;int result;
 while((result=GetMessageA(&message,NULL,0,0))>0){TranslateMessage(&message);DispatchMessageA(&message);}
 UnregisterClassA(cls.lpszClassName,instance);
 if(result<0)live_failed=1;
 printf("Native team window: %u source updates; %u successful paints; original artwork; navigation pending.\n",live_ticks,live_paints);
 return !live_failed&&live_paints&&(!live_check||live_ticks==60);
}
static int capture(uint8_t*b,size_t n,nflstreet_team_screen*s,const char*unpacked){
 screen_bytes=b;screen_size=n;team_state=s;unpacked_root=unpacked;if(!b||!s||!unpacked||n<32||!be32(b)||!replay_source_inputs())return 0;
 canvas.width=854;canvas.height=640;canvas.bgra=malloc((size_t)canvas.width*canvas.height*4);if(!canvas.bgra)return 0;
 for(unsigned i=0;i<4;++i)for(unsigned pass=0;pass<2;++pass)if(!nflstreet_font_atlas(&s->objects.fonts[i],pass,&font_atlas[i][pass]))goto fail;
 if(!load_images()||!load_heads()||!render()){fputs("Team source 2D renderer rejected an object, text layout, or image.\n",stderr);goto fail;}
 char path[4096];const char*prefix=getenv("DAYWATER_TEAM_CAPTURE_PREFIX");
 int z=prefix&&*prefix?snprintf(path,sizeof path,"%s.bmp",prefix):snprintf(path,sizeof path,"%s/team-screen-startup.bmp",DEFAULT_CAPTURE_DIR);
 if(z<0||z>=(int)sizeof path||!nflstreet_texture_bmp(&canvas,path)){fputs("Cannot write team-screen BMP.\n",stderr);goto fail;}
 if(head_inspection&&head_visible_slot<14){
  if(!prefix||!*prefix)goto fail;char head_path[4096];int head_z=snprintf(head_path,sizeof head_path,player_inspection?"%s-selected-player.bmp":"%s-selected-head.bmp",prefix);
  if(head_z<0||head_z>=(int)sizeof head_path||!nflstreet_texture_bmp(head_images+head_visible_slot,head_path))goto fail;
  printf("Separate selected-%s inspection: slot%u PLAY%u; source actor link; host pane, candidate stored pose.\n",player_inspection?"player":"head",head_visible_slot,head_player_ids[head_visible_slot]);
 }
 printf("2D team-menu startup capture: %s (%ux%u; source objects/fonts/images; no selected-player 3D claim).\n",path,canvas.width,canvas.height);
 if(live_mode&&!live_window())goto fail;
 for(unsigned i=0;i<image_count;++i)nflstreet_texture_close(&images[i].image);image_count=0;free(canvas.bgra);canvas.bgra=NULL;close_heads();return 1;
fail:for(unsigned i=0;i<image_count;++i)nflstreet_texture_close(&images[i].image);image_count=0;free(canvas.bgra);canvas.bgra=NULL;close_heads();return 0;
}

#define DAYWATER_TEAM_STARTUP_CAPTURE(b,n,s,unpacked) capture((b),(n),(s),(unpacked))
#define DAYWATER_TEAM_STARTUP_HOST inspect_host
#define DAYWATER_TEAM_STARTUP_CONTROLS inspect_controls
static int preview_start(uint8_t*b,size_t n,uint32_t base,nflstreet_team_screen*s){
 if(!quick_game_start)return nflstreet_team_screen_start(b,n,base,s);
 const uint32_t args[2]={1,0x69};
 return nflstreet_team_screen_start_context(b,n,base,s,7,5,0x817c0000u,UINT32_MAX,2,args);
}
#define nflstreet_team_screen_start preview_start
#define main team_screen_probe_main
#include "team_screen_probe.c"
#undef main
#undef nflstreet_team_screen_start
int main(int argc,char**argv){
 while(argc>1){
  if(argc>=3&&!strcmp(argv[argc-2],"--focus-nodes")){
   if(focus_nodes||focus_node!=UINT32_MAX)return 2;focus_nodes=argv[argc-1];argc-=2;
  }else if(argc>=3&&!strcmp(argv[argc-2],"--focus-node")){
   char*end;unsigned long node=strtoul(argv[argc-1],&end,10);
   if(focus_nodes||focus_node!=UINT32_MAX||!*argv[argc-1]||*end||node>=1024)return 2;
   focus_node=(unsigned)node;argc-=2;
  }else if(argc>=3&&!strcmp(argv[argc-2],"--source-events")){
   if(source_events)return 2;source_events=argv[argc-1];argc-=2;
  }else if(!strcmp(argv[argc-1],"--dispatcher-context")){
   if(dispatcher_context)return 2;dispatcher_context=1;--argc;
  }else if(!strcmp(argv[argc-1],"--quick-game-start")){
   if(quick_game_start)return 2;quick_game_start=1;--argc;
  }else if(!strcmp(argv[argc-1],"--source-audit")){
   if(source_audit)return 2;source_audit=1;--argc;
  }else if(!strcmp(argv[argc-1],"--source-controls")){
   if(source_controls)return 2;source_controls=1;--argc;
  }else if(argc>=3&&!strcmp(argv[argc-2],"--capture")){
   if(strncmp(argv[argc-1],"W:\\Daywater\\",12)||_putenv_s("DAYWATER_TEAM_CAPTURE_PREFIX",argv[argc-1]))return 2;argc-=2;
  }else if(!strcmp(argv[argc-1],"--inspect-roster")||!strcmp(argv[argc-1],"--inspect-confirmation")){
   if(inspect_mode)return 2;inspect_mode=!strcmp(argv[argc-1],"--inspect-roster")?1:2;--argc;
  }else if(!strcmp(argv[argc-1],"--player-inspection")){
   if(head_inspection)return 2;head_inspection=player_inspection=1;--argc;
  }else if(!strcmp(argv[argc-1],"--head-inspection")){
   if(head_inspection)return 2;head_inspection=1;--argc;
  }else if(!strcmp(argv[argc-1],"--live")||!strcmp(argv[argc-1],"--live-check")){
   if(live_mode)return 2;live_mode=1;live_check=!strcmp(argv[argc-1],"--live-check");--argc;
  }else break;
 }
 if(focus_nodes){const char*p=focus_nodes;unsigned count=0;
  do{char*end;unsigned long node=strtoul(p,&end,10);if(end==p||node>=1024||++count>32)return 2;
   if(!*end)break;if(*end!=','||!end[1])return 2;p=end+1;
  }while(*p);
 }
 if(source_events){const char*p=source_events;unsigned count=0;
  do{if(*p<'2'||*p>'7'||++count>32)return 2;++p;if(!*p)break;if(*p++!=','||!*p)return 2;}while(*p);
 }
 return team_screen_probe_main(argc,argv);
}
