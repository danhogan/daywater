/* Bounded raster preview of the original field-selection source entry.
   Source assets and captures stay in the private unpacked tree. The output
   is a host diagnostic raster, not a GX render or a claim of loaded fields. */
#include "team_classes.h"
#include "field_screen_preview.h"
#include "construct.h"
#include "screen.h"
#include "text.h"
#include "texture.h"
#include <windows.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BASE 0x81100000u
#define MAX_SOURCE (8u * 1024u * 1024u)
#define MAX_FILE (128u * 1024u * 1024u)
#define MAX_IMAGES 1024u

typedef struct { unsigned archive, entry; nflstreet_texture texture; } image_entry;
typedef struct { float a,b,c,d,x,y,color[8]; } transform;
typedef struct { float x,y,u,v,color[4]; } vertex;

static uint8_t *source;
static size_t source_size;
static const char *unpacked;
static nflstreet_team_classes classes;
static nflstreet_texture canvas, atlas[4][2];
static nflstreet_team_classes *live_classes;
static nflstreet_field_preview_event_fn live_event;
static nflstreet_field_preview_tick_fn live_tick;
static nflstreet_field_preview_painted_fn live_after_paint;
static void *live_context;
static unsigned live_failed,live_updates,live_paints;
static const unsigned *live_test_events;
static unsigned live_test_event_count,live_test_event_index;
static unsigned live_test_settled_ticks,live_test_ticks_since_event;
static unsigned live_test_waiting,live_test_complete,live_test_elapsed_ticks;
static image_entry images[MAX_IMAGES];
static unsigned image_count, node_budget;

static unsigned be(const uint8_t *p) { return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3]; }
static unsigned sh(const uint8_t *p) { return (unsigned)p[0]<<8|p[1]; }
static float fl(const uint8_t *p) { unsigned w=be(p); float f; memcpy(&f,&w,4); return f; }
static int private_w_path(const char *p) { return p && !strncmp(p,"W:\\Daywater\\",12); }
static uint8_t *ptr(unsigned a,size_t z) {
 if(a<BASE||(size_t)(a-BASE)>source_size||z>source_size-(a-BASE))return NULL;
 return source+a-BASE;
}
static uint8_t *read_file(const char *path,size_t *size,size_t limit) {
 FILE *f=fopen(path,"rb");if(!f)return NULL;
 if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}long n=ftell(f);
 if(n<1||(size_t)n>limit||fseek(f,0,SEEK_SET)){fclose(f);return NULL;}
 uint8_t *p=malloc((size_t)n);if(!p||fread(p,1,(size_t)n,f)!=(size_t)n){free(p);fclose(f);return NULL;}
 if(fclose(f)){free(p);return NULL;}*size=(size_t)n;return p;
}
static const char *archive_folder(unsigned archive,unsigned *entries) {
 switch(archive){
 case 1:*entries=16;return "uis_comn"; case 3:*entries=13;return "uis_butt";
 case 4:*entries=102;return "uis_tmlo"; case 7:*entries=3;return "uis_bgmp";
 case 8:*entries=25;return "uis_bgpl"; case 9:*entries=4;return "uis_ctrl";
 case 10:*entries=11;return "uis_fron"; case 11:*entries=41;return "uis_fsel";
 case 13:*entries=549;return "uis_port"; case 14:*entries=11;return "uis_gabr";
 case 15:*entries=18;return "uis_ingm"; case 16:*entries=56;return "uis_crtm";
 case 18:*entries=19;return "uis_chal"; case 20:*entries=22;return "uis_movi";
 default:*entries=0;return NULL;
 }
}
int nflstreet_field_screen_preview_consume_resource_effects(const char *root,
        const nflstreet_field_resource_effect *effects,unsigned count) {
 if(!private_w_path(root)||count>128||(count&&!effects))return 0;
 for(unsigned i=0;i<count;++i){const nflstreet_field_resource_effect *effect=&effects[i];
  if(effect->acquire>1)return 0;
  /* This rasterizer does not retain image references between captures. Its
     normal render cleanup already releases the previous snapshot; only new
     acquisitions need a source archive entry decoded before the prefix is
     considered handled. */
  if(!effect->acquire)continue;
  unsigned entries=0;const char *folder=archive_folder(effect->archive,&entries);
  if(!folder||effect->entry>=entries)return 0;
  char path[4096];int n=snprintf(path,sizeof path,"%s/%s/%05u.bin",root,folder,effect->entry);
  if(n<0||n>=(int)sizeof path)return 0;
  size_t size=0;uint8_t *bytes=read_file(path,&size,MAX_FILE);if(!bytes)return 0;
  nflstreet_texture texture={0};int ok=nflstreet_texture_open(bytes,size,&texture);
  free(bytes);nflstreet_texture_close(&texture);if(!ok)return 0;
 }
 return 1;
}
static const nflstreet_texture *image_for(unsigned archive,unsigned entry) {
 for(unsigned i=0;i<image_count;++i)if(images[i].archive==archive&&images[i].entry==entry)return &images[i].texture;
 return NULL;
}
static int acquire_image(unsigned archive,unsigned entry) {
 unsigned count=0;const char *folder=archive_folder(archive,&count);
 if(!folder||entry>=count)return 0;if(image_for(archive,entry))return 1;
 if(image_count>=MAX_IMAGES)return 0;
 char path[4096];int n=snprintf(path,sizeof path,"%s/%s/%05u.bin",unpacked,folder,entry);
 if(n<0||n>=(int)sizeof path)return 0;
 size_t bytes_n=0;uint8_t *bytes=read_file(path,&bytes_n,MAX_FILE);
 if(!bytes)return 0;image_entry *image=&images[image_count];
 if(!nflstreet_texture_open(bytes,bytes_n,&image->texture)){free(bytes);return 0;}
 free(bytes);image->archive=archive;image->entry=entry;++image_count;return 1;
}
static transform combine(transform p,const uint8_t *r) {
 float angle=fl(r+40)*0.017453292519943295f,sx=fl(r+44),sy=fl(r+48),cs=cosf(angle),sn=sinf(angle);
 float x=fl(r+8),y=fl(r+12);transform t=p;
 t.a=p.a*cs*sx+p.c*sn*sx;t.b=p.b*cs*sx+p.d*sn*sx;
 t.c=-p.a*sn*sy+p.c*cs*sy;t.d=-p.b*sn*sy+p.d*cs*sy;
 t.x=p.x+p.a*x+p.c*y;t.y=p.y+p.b*x+p.d*y;
 for(unsigned i=0;i<4;++i){t.color[i]=p.color[i]*fl(r+60+4*i);t.color[4+i]=p.color[4+i]+fl(r+76+4*i);}
 return t;
}
static void blend(int x,int y,unsigned src,const float *c) {
 if(x<0||y<0||(unsigned)x>=canvas.width||(unsigned)y>=canvas.height)return;
 float alpha=(src>>24)/255.0f*c[3];if(!isfinite(alpha)||alpha<=0)return;if(alpha>1)alpha=1;
 uint32_t *dst=canvas.bgra+(size_t)y*canvas.width+x,value=0xff000000u;
 for(unsigned k=0;k<3;++k){float s=((src>>(k*8))&255)*c[2-k];if(s<0)s=0;if(s>255)s=255;
  value|=(unsigned)(s*alpha+((*dst>>(k*8))&255)*(1-alpha))<<(k*8);}
 *dst=value;
}
static void sprite(const nflstreet_texture *tex,float x,float y,unsigned u,unsigned v,unsigned w,unsigned h,const float *color,transform t) {
 if(!tex||!tex->bgra||!w||!h)return;float det=t.a*t.d-t.b*t.c;
 if(!isfinite(det)||fabsf(det)<1e-8f)return;float xx[4],yy[4];
 for(unsigned i=0;i<4;++i){float lx=x+(i&1?w:0),ly=y+(i&2?h:0);xx[i]=t.x+t.a*lx+t.c*ly;yy[i]=t.y+t.b*lx+t.d*ly;}
 float minx=xx[0],maxx=xx[0],miny=yy[0],maxy=yy[0];
 for(unsigned i=1;i<4;++i){minx=fminf(minx,xx[i]);maxx=fmaxf(maxx,xx[i]);miny=fminf(miny,yy[i]);maxy=fmaxf(maxy,yy[i]);}
 int x0=(int)fmaxf(0,floorf(minx)),x1=(int)fminf(canvas.width,ceilf(maxx));
 int y0=(int)fmaxf(0,floorf(miny)),y1=(int)fminf(canvas.height,ceilf(maxy));
 for(int py=y0;py<y1;++py)for(int px=x0;px<x1;++px){float dx=px+.5f-t.x,dy=py+.5f-t.y;
  float lx=(t.d*dx-t.c*dy)/det-x,ly=(-t.b*dx+t.a*dy)/det-y;
  if(lx>=0&&ly>=0&&lx<w&&ly<h&&u+(unsigned)lx<tex->width&&v+(unsigned)ly<tex->height)
   blend(px,py,tex->bgra[(v+(unsigned)ly)*tex->width+u+(unsigned)lx],color);
 }
}
static void triangle(const nflstreet_texture *tex,const vertex *a,const vertex *b,const vertex *c) {
 float den=(b->y-c->y)*(a->x-c->x)+(c->x-b->x)*(a->y-c->y);if(!isfinite(den)||fabsf(den)<1e-8f)return;
 int x0=(int)fmaxf(0,floorf(fminf(a->x,fminf(b->x,c->x)))),x1=(int)fminf(canvas.width,ceilf(fmaxf(a->x,fmaxf(b->x,c->x))));
 int y0=(int)fmaxf(0,floorf(fminf(a->y,fminf(b->y,c->y)))),y1=(int)fminf(canvas.height,ceilf(fmaxf(a->y,fmaxf(b->y,c->y))));
 for(int y=y0;y<y1;++y)for(int x=x0;x<x1;++x){float p=((b->y-c->y)*(x+.5f-c->x)+(c->x-b->x)*(y+.5f-c->y))/den;
  float q=((c->y-a->y)*(x+.5f-c->x)+(a->x-c->x)*(y+.5f-c->y))/den,r=1-p-q;if(p<0||q<0||r<0)continue;
  float color[4];for(unsigned j=0;j<4;++j)color[j]=p*a->color[j]+q*b->color[j]+r*c->color[j];unsigned src=0xffffffffu;
  if(tex){float u=p*a->u+q*b->u+r*c->u,v=p*a->v+q*b->v+r*c->v;
   unsigned tx=(unsigned)fmaxf(0,fminf(tex->width-1,u*tex->width)),ty=(unsigned)fmaxf(0,fminf(tex->height-1,v*tex->height));src=tex->bgra[(size_t)ty*tex->width+tx];}
  blend(x,y,src,color);
 }
}
static int find_lines(unsigned target,const nflstreet_ui_text_lines **lines,nflstreet_ui_text_lines *inline_line) {
 for(unsigned i=0;i<classes.line_plan_count;++i)if(classes.line_plans[i].target==target){*lines=&classes.line_plans[i].plan;return 1;}
 uint8_t *o=ptr(target,40);if(!o||o[15]!=1)return 0;
 unsigned first=sh(o+36),last=sh(o+38);if(first>last||last-first>354)return 0;
 inline_line->count=1;inline_line->lines[0]=(nflstreet_ui_text_line){(uint16_t)first,(uint16_t)last};*lines=inline_line;return 1;
}
static int draw_text(unsigned target,transform t) {
 uint8_t *o=ptr(target,40);if(!o)return 0;unsigned fi=o[2];if(fi>=4||!classes.fonts[fi].bytes)return 0;
 uint8_t *d=ptr(be(o+32),12),*text=d?ptr(be(d+8),1):NULL;
 if(!text||!memchr(text,0,source_size-(size_t)(text-source)))return 0;
 const nflstreet_ui_text_lines *lines=NULL;nflstreet_ui_text_lines inline_line={0};
 if(!find_lines(target,&lines,&inline_line))return 0;const nflstreet_font *font=&classes.fonts[fi];unsigned length=(unsigned)strlen((char*)text);
 uint8_t object[40];memcpy(object,o,40);unsigned flags=sh(o);object[0]=(uint8_t)((flags&~0x8300)>>8);object[1]=(uint8_t)(flags&~0x8300);object[15]=1;object[36]=object[37]=0;
 float x=(int16_t)sh(o+6),y=(int16_t)sh(o+8);if(flags&0x100)x+=flags&1?((int16_t)sh(o+20)>>1):flags&2?(int16_t)sh(o+20):0;
 for(unsigned line=o[16];line<lines->count;++line){if((flags&0x200)&&y+font->line_height>(int16_t)sh(o+8)+(int16_t)sh(o+22))break;
  unsigned first=lines->lines[line].begin,last=lines->lines[line].end;if(first>last||last>length||last-first>354)return 0;
  char value[355];memcpy(value,text+first,last-first);value[last-first]=0;object[38]=(uint8_t)((last-first)>>8);object[39]=(uint8_t)(last-first);
  object[6]=(uint8_t)((uint16_t)(int16_t)x>>8);object[7]=(uint8_t)x;object[8]=(uint8_t)((uint16_t)(int16_t)y>>8);object[9]=(uint8_t)y;
  nflstreet_ui_text_plan plan;nflstreet_glyph_box glyphs[354];unsigned count;
  if(!nflstreet_ui_text_draw(object,value,t.color,&plan)||!nflstreet_font_layout(font,value,plan.flags,0,glyphs,354,&count))return 0;
  for(unsigned pass=0;pass<2;++pass)for(unsigned i=0;i<count;++i){nflstreet_glyph_box *g=glyphs+i;
   sprite(&atlas[fi][pass],plan.position[0]+g->x+(pass?0:plan.shadow_offset[0]),plan.position[1]+g->y+(pass?0:plan.shadow_offset[1]),g->u,g->v,g->width,g->height,plan.color[pass?0:1],t);}
  y+=(int)font->line_height;
 }
 return 1;
}
static int draw_image(unsigned target,transform t) {
 uint8_t *o=ptr(target,76);if(!o)return 0;unsigned flags=sh(o);if(flags&0x8000)return 0;
 const nflstreet_texture *tex=NULL;if(flags&2){unsigned archive=sh(o+6),entry=sh(o+4);tex=image_for(archive,entry);if(!tex){if(!acquire_image(archive,entry))return 0;tex=image_for(archive,entry);}if(!tex)return 0;}
 vertex v[4];for(unsigned i=0;i<4;++i){float x=(int16_t)sh(o+52+i*6)/16.0f,y=(int16_t)sh(o+54+i*6)/16.0f;
  v[i].x=t.x+t.a*x+t.c*y;v[i].y=t.y+t.b*x+t.d*y;v[i].u=(int16_t)sh(o+36+i*4)/256.0f;v[i].v=(int16_t)sh(o+38+i*4)/256.0f;
  for(unsigned j=0;j<4;++j)v[i].color[j]=(o[20+i*4+j]+t.color[4+j])*.003921568859368563f*t.color[j];}
 triangle(tex,v,v+1,v+2);triangle(tex,v,v+2,v+3);return 1;
}
static int scan_images(void) {
 for(unsigned i=0;i<classes.request_count;++i){const nflstreet_ui_service_request *q=&classes.requests[i];if(q->class_id!=0||q->kind||q->property!=UINT32_MAX)continue;
  uint8_t *o=ptr(q->target,36);if(!o)return 0;unsigned flags=sh(o);if(!(flags&2))continue;
  if(!acquire_image(sh(o+6),sh(o+4)))return 0;
 }
 return 1;
}
static int node(unsigned index,transform parent) {
 if(!node_budget--||index>=be(source))return 0;uint8_t *rows=ptr(be(source+4),(size_t)be(source)*20);if(!rows)return 0;
 uint8_t *row=rows+index*20,*root=ptr(be(row),100);if(!root)return 0;if(!be(root))return 1;transform t=combine(parent,root);
 for(unsigned i=0;i<8;++i)if(!isfinite(t.color[i]))return 0;unsigned count=be(row+4);uint8_t *groups=ptr(be(row+8),(size_t)count*4);if(!groups)return 0;
 for(unsigned i=0;i<count;++i){uint8_t *g=ptr(be(groups+4*i),12),*gr=g?ptr(be(g),4):NULL;if(!gr)return 0;if(!be(gr))continue;
  unsigned links=be(g+4);uint8_t *list=ptr(be(g+8),(size_t)links*8);if(!list)return 0;
  for(unsigned j=0;j<links;++j){unsigned cls=sh(list+j*8);if(cls==65535){if(!node(be(list+j*8+4),t))return 0;}
   else if(sh(list+j*8+2)&&(cls==0||cls==8)){uint8_t *cell=ptr(be(list+j*8+4),4);if(!cell||be(cell)>UINT32_MAX-BASE)return 0;
    unsigned target=BASE+be(cell);if(!(cls==0?draw_image(target,t):draw_text(target,t)))return 0;}}
 }
 return 1;
}
static int render(void) {
 if(live_classes)classes=*live_classes;
 for(size_t i=0;i<(size_t)canvas.width*canvas.height;++i)canvas.bgra[i]=0xff202020u;
 transform t={0};t.a=t.d=1;for(unsigned i=0;i<4;++i)t.color[i]=1;node_budget=8192;return node(0,t);
}
static LRESULT CALLBACK live_proc(HWND window,UINT message,WPARAM key,LPARAM detail) {
 if(message==WM_DESTROY){PostQuitMessage(0);return 0;}
 if(message==WM_KEYDOWN&&key==VK_ESCAPE){DestroyWindow(window);return 0;}
 if(message==WM_KEYDOWN&&key>=VK_F2&&key<=VK_F7){
  if(detail&((LPARAM)1<<30))return 0;
  unsigned event=(unsigned)(key-VK_F2)+2;
  if(live_test_events&&(!live_test_waiting||live_test_event_index>=live_test_event_count||
      event!=live_test_events[live_test_event_index]))return 0;
  if(!live_event||!live_event(live_context,event)||!render()){
   live_failed=1;SetWindowTextA(window,"NFL Street field source picker - source event unavailable");
   if(live_test_events)PostQuitMessage(1);return 0;
  }
  if(live_test_events){++live_test_event_index;live_test_ticks_since_event=0;live_test_waiting=0;}
  InvalidateRect(window,NULL,FALSE);UpdateWindow(window);return 0;
 }
 if(message==WM_TIMER){
  /* This is a host elapsed value forwarded to the source adapter; it is not
     asserted to be a physical game frame or an original clock unit. */
  if(!live_tick||!live_tick(live_context,16)||!render()){
   live_failed=1;KillTimer(window,1);
   SetWindowTextA(window,"NFL Street field source picker - source update unavailable");
   if(live_test_events)PostQuitMessage(1);return 0;
  }
  ++live_updates;InvalidateRect(window,NULL,FALSE);UpdateWindow(window);
  if(live_test_events&&!live_test_complete){
   ++live_test_elapsed_ticks;
   if(live_test_elapsed_ticks>600){live_failed=1;KillTimer(window,1);PostQuitMessage(1);return 0;}
   if(live_test_event_index<live_test_event_count&&!live_test_waiting&&
      ++live_test_ticks_since_event>=live_test_settled_ticks){
    unsigned event=live_test_events[live_test_event_index];live_test_waiting=1;
    if(event<2||event>7||!PostMessageA(window,WM_KEYDOWN,VK_F2+(event-2),0)){
     live_failed=1;KillTimer(window,1);PostQuitMessage(1);return 0;
    }
   }
  }
  return 0;
 }
 if(message==WM_PAINT){
  PAINTSTRUCT paint;HDC dc=BeginPaint(window,&paint);RECT rect;GetClientRect(window,&rect);
  BITMAPINFO info={0};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
  info.bmiHeader.biWidth=(LONG)canvas.width;info.bmiHeader.biHeight=-(LONG)canvas.height;
  info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
  int result=StretchDIBits(dc,0,0,rect.right,rect.bottom,0,0,
    (int)canvas.width,(int)canvas.height,canvas.bgra,&info,DIB_RGB_COLORS,SRCCOPY);
  if(result==0||result==(int)GDI_ERROR){live_failed=1;if(live_test_events)PostQuitMessage(1);}
  else {++live_paints;if(live_after_paint){int handled=live_after_paint(live_context);
    if(!handled){live_failed=1;SetWindowTextA(window,"NFL Street field source picker - effect handling unavailable");if(live_test_events)PostQuitMessage(1);}
    else if(handled>1&&live_test_events){live_test_complete=1;PostQuitMessage(0);}}}
  EndPaint(window,&paint);return 0;
 }
 return DefWindowProcA(window,message,key,detail);
}
static int run_live_window(void) {
 HINSTANCE instance=GetModuleHandleA(NULL);WNDCLASSA cls={0};cls.lpfnWndProc=live_proc;
 cls.hInstance=instance;cls.lpszClassName="NFLStreetFieldSourceInspection";
 cls.hCursor=LoadCursorA(NULL,IDC_ARROW);if(!RegisterClassA(&cls))return 0;
 RECT rect={0,0,1024,768};AdjustWindowRect(&rect,WS_OVERLAPPEDWINDOW,FALSE);
 HWND window=CreateWindowA(cls.lpszClassName,
  live_test_events?"NFL Street field source picker - deterministic source self-test":"NFL Street field source picker - F2..F7 send source events 2..7; Escape closes",
  WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,rect.right-rect.left,
  rect.bottom-rect.top,NULL,NULL,instance,NULL);
 if(!window){UnregisterClassA(cls.lpszClassName,instance);return 0;}
 ShowWindow(window,SW_SHOW);UpdateWindow(window);
 if(!SetTimer(window,1,16,NULL)){DestroyWindow(window);UnregisterClassA(cls.lpszClassName,instance);return 0;}
 MSG message;int result;while((result=GetMessageA(&message,NULL,0,0))>0){TranslateMessage(&message);DispatchMessageA(&message);}
 UnregisterClassA(cls.lpszClassName,instance);
 printf("Field source window: %u host updates, %u successful paints; F2..F7 were numeric source events 2..7, not a verified physical controller mapping.%s\n",live_updates,live_paints,live_test_events?(live_test_complete?" Self-test completed.":" Self-test incomplete."):"");
 return result>=0&&!live_failed&&live_paints>0&&(!live_test_events||live_test_complete);
}
static void cleanup(int release_inputs) {
 for(unsigned i=0;i<4;++i){nflstreet_texture_close(&atlas[i][0]);nflstreet_texture_close(&atlas[i][1]);if(release_inputs)free((void*)classes.fonts[i].bytes);}
 for(unsigned i=0;i<image_count;++i)nflstreet_texture_close(&images[i].texture);
 image_count=0;free(canvas.bgra);canvas.bgra=NULL;
 if(release_inputs){free(source);source=NULL;source_size=0;}
}
int nflstreet_field_screen_preview_render(uint8_t *bytes,size_t size,const char *root,
    const nflstreet_team_classes *objects,const char *capture_path) {
 if(!bytes||size<32||size>MAX_SOURCE||!private_w_path(root)||!objects||!private_w_path(capture_path)||be(bytes)>1024)return 0;
 source=bytes;source_size=size;unpacked=root;classes=*objects;
 for(unsigned i=0;i<4;++i){if(!classes.fonts[i].bytes||!classes.fonts[i].size)return 0;
  for(unsigned pass=0;pass<2;++pass)if(!nflstreet_font_atlas(&classes.fonts[i],pass,&atlas[i][pass])){cleanup(0);return 0;}}
 if(!scan_images()){cleanup(0);return 0;}
 canvas.width=854;canvas.height=640;canvas.bgra=malloc((size_t)canvas.width*canvas.height*4);
 if(!canvas.bgra||!render()||!nflstreet_texture_bmp(&canvas,capture_path)){cleanup(0);return 0;}
 printf("Field source preview: %u nodes, %u class0/%u class8 constructions, %u wrapped line plans, %u decoded source images; cache/event state supplied by caller; host rasterizer, no GX claim. Capture: %s\n",
  be(source),classes.class_requests[0],classes.class_requests[8],classes.line_plan_count,image_count,capture_path);
 cleanup(0);return 1;
}
static int run_live_preview(uint8_t *bytes,size_t size,const char *root,
    nflstreet_team_classes *objects,nflstreet_field_preview_event_fn event_fn,
    nflstreet_field_preview_tick_fn tick_fn,void *context,
    nflstreet_field_preview_painted_fn after_paint,const unsigned *test_events,
    unsigned test_event_count,unsigned settled_ticks) {
 if(!bytes||size<32||size>MAX_SOURCE||!private_w_path(root)||!objects||
    !event_fn||!tick_fn||be(bytes)>1024)return 0;
 source=bytes;source_size=size;unpacked=root;live_classes=objects;
 live_event=event_fn;live_tick=tick_fn;live_context=context;
 live_after_paint=after_paint;
 live_test_events=test_events;live_test_event_count=test_event_count;live_test_event_index=0;
 live_test_settled_ticks=settled_ticks;live_test_ticks_since_event=0;
 live_test_waiting=live_test_complete=live_test_elapsed_ticks=0;
 if((test_events&&(!test_event_count||test_event_count>32||!settled_ticks))||
    (!test_events&&(test_event_count||settled_ticks)))return 0;
 live_failed=live_updates=live_paints=0;
 for(unsigned i=0;i<4;++i){if(!objects->fonts[i].bytes||!objects->fonts[i].size)return 0;
  for(unsigned pass=0;pass<2;++pass)if(!nflstreet_font_atlas(&objects->fonts[i],pass,&atlas[i][pass])){cleanup(0);return 0;}}
 canvas.width=854;canvas.height=640;canvas.bgra=malloc((size_t)canvas.width*canvas.height*4);
 if(!canvas.bgra||!render()){cleanup(0);return 0;}
 int ok=run_live_window();cleanup(0);live_classes=NULL;live_event=NULL;live_tick=NULL;live_context=NULL;live_after_paint=NULL;
 live_test_events=NULL;live_test_event_count=live_test_event_index=0;
 return ok;
}
int nflstreet_field_screen_preview_live(uint8_t *bytes,size_t size,const char *root,
    nflstreet_team_classes *objects,nflstreet_field_preview_event_fn event_fn,
    nflstreet_field_preview_tick_fn tick_fn,void *context) {
 return run_live_preview(bytes,size,root,objects,event_fn,tick_fn,context,NULL,NULL,0,0);
}
int nflstreet_field_screen_preview_live_after_paint(uint8_t *bytes,size_t size,const char *root,
    nflstreet_team_classes *objects,nflstreet_field_preview_event_fn event_fn,
    nflstreet_field_preview_tick_fn tick_fn,void *context,
    nflstreet_field_preview_painted_fn after_paint) {
 if(!after_paint)return 0;
 return run_live_preview(bytes,size,root,objects,event_fn,tick_fn,context,after_paint,NULL,0,0);
}
int nflstreet_field_screen_preview_live_self_test(uint8_t *bytes,size_t size,const char *root,
    nflstreet_team_classes *objects,nflstreet_field_preview_event_fn event_fn,
    nflstreet_field_preview_tick_fn tick_fn,void *context,
    nflstreet_field_preview_painted_fn after_paint,const unsigned *events,
    unsigned event_count,unsigned settled_ticks) {
 if(!after_paint||!events||event_count!=4||events[0]!=5||events[1]!=4||
    events[2]!=5||events[3]!=7)return 0;
 return run_live_preview(bytes,size,root,objects,event_fn,tick_fn,context,
                         after_paint,events,event_count,settled_ticks);
}
int main(int argc,char **argv) {
 if(argc!=4){fprintf(stderr,"Usage: field-screen-preview.exe raw-entry8.bin unpacked-root capture.bmp\n");return 2;}
 if(!private_w_path(argv[1])||!private_w_path(argv[2])||!private_w_path(argv[3]))return 2;
 unpacked=argv[2];size_t n=0;source=read_file(argv[1],&n,MAX_SOURCE);source_size=n;if(!source||n<32||be(source)>1024){cleanup(1);return 3;}
 unsigned relocated=0;if(!nflstreet_ui_relocate(source,n,BASE,&relocated)||relocated!=1){cleanup(1);return 4;}
 for(unsigned i=0;i<4;++i){char path[4096];size_t font_size=0;int z=snprintf(path,sizeof path,"%s/uis_font/%05u.bin",unpacked,i);if(z<0||z>=(int)sizeof path){cleanup(1);return 5;}
  uint8_t *font=read_file(path,&font_size,16u*1024u*1024u);if(!font||!nflstreet_font_open(font,font_size,&classes.fonts[i])){free(font);cleanup(1);return 5;}
  classes.fonts[i].bytes=font;classes.fonts[i].size=font_size;
  }
 if(!nflstreet_team_classes_construct(source,n,BASE,&classes)){fprintf(stderr,"Original class0/class8 constructor rejected\n");cleanup(1);return 6;}
 if(!nflstreet_field_screen_preview_render(source,n,unpacked,&classes,argv[3])){fprintf(stderr,"Field source render rejected\n");cleanup(1);return 8;}
 cleanup(1);return 0;
}
