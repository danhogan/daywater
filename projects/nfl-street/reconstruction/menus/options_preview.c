/* Independent Options inspection. Source construction, event scripts, focus,
   settings and timers execute locally. Host affine rasterization is diagnostic;
   no claim of original GX projection, clipping, filtering or card dialogs. */
#include <windows.h>
#include "options_screen.h"
#include "profile_store.h"
#include "construct.h"
#include "text_construct.h"
#include "image_construct.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define BASE 0x01000000u
static uint8_t*screen;static size_t screen_size;
static uint8_t*source_screen;static const char*profile_path;
static nflstreet_ui_options_screen state;
static nflstreet_texture canvas,atlas[3][2],images[4][16];
static uint8_t*font_bytes[3];
static unsigned be(const uint8_t*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static unsigned sh(const uint8_t*p){return (unsigned)p[0]<<8|p[1];}
static float fl(const uint8_t*p){unsigned w=be(p);float f;memcpy(&f,&w,4);return f;}
static uint8_t*ptr(unsigned a,size_t z){if(a<BASE||a-BASE>screen_size||z>screen_size-(a-BASE))return NULL;return screen+a-BASE;}
static uint8_t*read(const char*path,size_t*n){FILE*f=fopen(path,"rb");if(!f)return NULL;fseek(f,0,SEEK_END);long z=ftell(f);rewind(f);if(z<32||z>8*1024*1024){fclose(f);return NULL;}uint8_t*b=malloc(z);if(!b||fread(b,1,z,f)!=(size_t)z){free(b);b=NULL;}fclose(f);*n=z;return b;}
static int construct(void*ctx,uint8_t*b,size_t n,uint32_t base,const nflstreet_ui_service_request*q,uint32_t*result){(void)result;nflstreet_ui_options_screen*s=ctx;if(q->kind||q->property!=UINT32_MAX||q->target<base||q->target-base>n)return 0;if(q->class_id==8){if(40>n-(q->target-base)||b[q->target-base+2]>2)return 0;return nflstreet_ui_text_construct(b,n,base,q->target,&s->fonts[b[q->target-base+2]]);}if(q->class_id==0){unsigned request;uint32_t identity;return nflstreet_ui_image_construct(b,n,base,q->target,&request,&identity);}return 0;}
typedef struct {float a,b,c,d,x,y,color[8];} transform;
static transform combine(transform p,const uint8_t*r){float angle=fl(r+40)*0.017453292519943295f,sx=fl(r+44),sy=fl(r+48),cs=cosf(angle),sn=sinf(angle),x=fl(r+8),y=fl(r+12);transform t=p;t.a=p.a*cs*sx+p.c*sn*sx;t.b=p.b*cs*sx+p.d*sn*sx;t.c=-p.a*sn*sy+p.c*cs*sy;t.d=-p.b*sn*sy+p.d*cs*sy;t.x=p.x+p.a*x+p.c*y;t.y=p.y+p.b*x+p.d*y;for(unsigned i=0;i<4;++i){t.color[i]=p.color[i]*fl(r+60+4*i);t.color[4+i]=p.color[4+i]+fl(r+76+4*i);}return t;}
static void blend(int x,int y,unsigned src,const float*c){if(x<0||y<0||(unsigned)x>=canvas.width||(unsigned)y>=canvas.height)return;float af=(src>>24)/255.0f*c[3];if(!isfinite(af)||af<=0)return;if(af>1)af=1;uint32_t*dst=canvas.bgra+(size_t)y*canvas.width+x,v=0xff000000;for(unsigned k=0;k<3;++k){float s=((src>>(k*8))&255)*c[2-k];if(s<0)s=0;if(s>255)s=255;v|=(unsigned)(s*af+((*dst>>(k*8))&255)*(1-af))<<(k*8);}*dst=v;}
static void sprite(const nflstreet_texture*tex,float x,float y,unsigned u,unsigned v,unsigned width,unsigned height,const float*color,transform t){
 if(!tex->bgra||!width||!height)return;float det=t.a*t.d-t.b*t.c;if(!isfinite(det)||fabsf(det)<1e-8f)return;
 float xx[4],yy[4];for(unsigned i=0;i<4;++i){float lx=x+(i&1?width:0),ly=y+(i&2?height:0);xx[i]=t.x+t.a*lx+t.c*ly;yy[i]=t.y+t.b*lx+t.d*ly;}
 float minx=xx[0],maxx=xx[0],miny=yy[0],maxy=yy[0];for(unsigned i=1;i<4;++i){minx=fminf(minx,xx[i]);maxx=fmaxf(maxx,xx[i]);miny=fminf(miny,yy[i]);maxy=fmaxf(maxy,yy[i]);}int x0=(int)fmaxf(0,floorf(minx)),x1=(int)fminf(canvas.width,ceilf(maxx)),y0=(int)fmaxf(0,floorf(miny)),y1=(int)fminf(canvas.height,ceilf(maxy));for(int py=y0;py<y1;++py)for(int px=x0;px<x1;++px){float dx=px+0.5f-t.x,dy=py+0.5f-t.y,lx=(t.d*dx-t.c*dy)/det-x,ly=(-t.b*dx+t.a*dy)/det-y;if(lx>=0&&ly>=0&&lx<width&&ly<height&&u+(unsigned)lx<tex->width&&v+(unsigned)ly<tex->height)blend(px,py,tex->bgra[(v+(unsigned)ly)*tex->width+u+(unsigned)lx],color);}
}
typedef struct {float x,y,u,v,color[4];} vertex;
static void triangle(const nflstreet_texture*tex,const vertex*a,const vertex*b,const vertex*c){float den=(b->y-c->y)*(a->x-c->x)+(c->x-b->x)*(a->y-c->y);if(!isfinite(den)||fabsf(den)<1e-8f)return;int x0=(int)fmaxf(0,floorf(fminf(a->x,fminf(b->x,c->x)))),x1=(int)fminf(canvas.width,ceilf(fmaxf(a->x,fmaxf(b->x,c->x)))),y0=(int)fmaxf(0,floorf(fminf(a->y,fminf(b->y,c->y)))),y1=(int)fminf(canvas.height,ceilf(fmaxf(a->y,fmaxf(b->y,c->y))));for(int y=y0;y<y1;++y)for(int x=x0;x<x1;++x){float p=((b->y-c->y)*(x+.5f-c->x)+(c->x-b->x)*(y+.5f-c->y))/den,q=((c->y-a->y)*(x+.5f-c->x)+(a->x-c->x)*(y+.5f-c->y))/den,r=1-p-q;if(p<0||q<0||r<0)continue;float color[4];for(unsigned j=0;j<4;++j)color[j]=p*a->color[j]+q*b->color[j]+r*c->color[j];unsigned source=0xffffffff;if(tex){float u=p*a->u+q*b->u+r*c->u,v=p*a->v+q*b->v+r*c->v;unsigned tx=(unsigned)fmaxf(0,fminf(tex->width-1,u*tex->width)),ty=(unsigned)fmaxf(0,fminf(tex->height-1,v*tex->height));source=tex->bgra[(size_t)ty*tex->width+tx];}blend(x,y,source,color);}}
static int draw_object(unsigned cls,unsigned target,transform t){uint8_t*o=ptr(target,cls==8?40:76);if(!o)return 0;if(cls==8){unsigned entry=o[2];if(entry>2||!state.fonts[entry].bytes)return 0;uint8_t*d=ptr(be(o+32),12),*text=d?ptr(be(d+8),1):NULL;if(!text||!memchr(text,0,screen_size-(size_t)(text-screen)))return 0;if(!nflstreet_ui_text_prepare(o,&state.fonts[entry],(char*)text))return 0;nflstreet_ui_text_plan plan;nflstreet_glyph_box glyphs[354];unsigned count;if(!nflstreet_ui_text_draw(o,(char*)text,t.color,&plan)||!nflstreet_font_layout(&state.fonts[entry],(char*)text,plan.flags,0,glyphs,354,&count))return 0;if(plan.visible)for(unsigned pass=0;pass<2;++pass)for(unsigned i=0;i<count;++i){nflstreet_glyph_box*g=glyphs+i;sprite(&atlas[entry][pass],plan.position[0]+g->x+(pass?0:plan.shadow_offset[0]),plan.position[1]+g->y+(pass?0:plan.shadow_offset[1]),g->u,g->v,g->width,g->height,plan.color[pass?0:1],t);}return 1;}
 if(cls!=0)return 1;unsigned flags=sh(o);if(flags&0x8000)return 0;const nflstreet_texture*tex=NULL;if(flags&2){unsigned archive=sh(o+6),entry=sh(o+4);if((archive!=1&&archive!=3)||entry>=16||!images[archive][entry].bgra)return 0;tex=&images[archive][entry];}vertex vertices[4];for(unsigned i=0;i<4;++i){float x=(int16_t)sh(o+52+i*6)/16.0f,y=(int16_t)sh(o+54+i*6)/16.0f;vertex*v=vertices+i;v->x=t.x+t.a*x+t.c*y;v->y=t.y+t.b*x+t.d*y;v->u=(int16_t)sh(o+36+i*4)/256.0f;v->v=(int16_t)sh(o+38+i*4)/256.0f;for(unsigned j=0;j<4;++j)v->color[j]=(o[20+i*4+j]+t.color[4+j])*0.003921568859368563f*t.color[j];}triangle(tex,vertices,vertices+1,vertices+2);triangle(tex,vertices,vertices+2,vertices+3);return 1;}
static unsigned budget;
static int draw_node(unsigned index,transform parent){if(!budget--||index>=be(screen))return 0;uint8_t*nodes=ptr(be(screen+4),(size_t)be(screen)*20);if(!nodes)return 0;uint8_t*row=nodes+index*20,*root=ptr(be(row),100);if(!root)return 0;if(!be(root))return 1;transform t=combine(parent,root);for(unsigned j=0;j<8;++j)if(!isfinite(t.color[j]))return 0;unsigned count=be(row+4);uint8_t*groups=ptr(be(row+8),(size_t)count*4);if(!groups)return 0;for(unsigned i=0;i<count;++i){uint8_t*g=ptr(be(groups+4*i),12),*gr=g?ptr(be(g),4):NULL;if(!gr)return 0;if(!be(gr))continue;unsigned links=be(g+4);uint8_t*l=ptr(be(g+8),(size_t)links*8);if(!l)return 0;for(unsigned j=0;j<links;++j){unsigned cls=sh(l+8*j);if(cls==65535){if(!draw_node(be(l+8*j+4),t))return 0;}else if(sh(l+8*j+2)&&(cls==0||cls==8)){uint8_t*cell=ptr(be(l+8*j+4),4);if(!cell||be(cell)>UINT32_MAX-BASE||!draw_object(cls,BASE+be(cell),t))return 0;}}}return 1;}
static int draw(void){for(unsigned i=0;i<canvas.width*canvas.height;++i)canvas.bgra[i]=0xff202020;transform t={0};t.a=t.d=1;for(unsigned i=0;i<4;++i)t.color[i]=1;budget=8192;return draw_node(0,t);}
static void clear_effects(void){state.options.count=state.effect_count=0;state.shared.pending_count=state.shared.broadcast_count=state.shared.audio_count=state.shared.resource_count=0;}
static int advance(void){clear_effects();return nflstreet_ui_options_screen_advance(screen,screen_size,BASE,&state,16)&&draw();}
static int control(unsigned event){clear_effects();return nflstreet_ui_options_screen_control(screen,screen_size,BASE,&state,event)&&draw();}
/* Host persistence commands. Original card dialog routing remains separate. */
static int restore_profile(void){
 if(!profile_path)return 0;uint32_t loaded[22];if(nflstreet_option_load(profile_path,loaded)!=1)return 0;
 uint8_t*saved=malloc(screen_size);nflstreet_ui_options_screen*prior=malloc(sizeof state);if(!saved||!prior){free(saved);free(prior);return 0;}memcpy(saved,screen,screen_size);*prior=state;
 memset(&state,0,sizeof state);memcpy(state.fonts,prior->fonts,sizeof state.fonts);memcpy(state.profile,loaded,sizeof loaded);state.shared.callback_byte_6f78=1;state.shared.rates.clock=16;memcpy(screen,source_screen,screen_size);unsigned relocated;
 int ok=nflstreet_ui_relocate(screen,screen_size,BASE,&relocated)&&nflstreet_ui_construct(screen,screen_size,BASE,0,UINT32_MAX,0x101,construct,&state,sizeof state)&&nflstreet_ui_options_screen_start(screen,screen_size,BASE,&state);
 for(unsigned i=0;ok&&i<160;++i)ok=advance();
 if(!ok){memcpy(screen,saved,screen_size);state=*prior;(void)draw();}free(saved);free(prior);return ok;
}
static LRESULT CALLBACK window(HWND h,UINT msg,WPARAM w,LPARAM l){if(msg==WM_PAINT){PAINTSTRUCT ps;HDC dc=BeginPaint(h,&ps);BITMAPINFO bi={0};bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=canvas.width;bi.bmiHeader.biHeight=-(LONG)canvas.height;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;SetDIBitsToDevice(dc,0,0,canvas.width,canvas.height,0,0,0,canvas.height,canvas.bgra,&bi,DIB_RGB_COLORS);EndPaint(h,&ps);return 0;}if(msg==WM_TIMER){if(!advance()){KillTimer(h,1);SetWindowTextA(h,"NFL Street Options - operation unavailable");}if(state.return_count)DestroyWindow(h);else InvalidateRect(h,NULL,FALSE);return 0;}if(msg==WM_KEYDOWN){if(w==VK_F5||w==VK_F9){int ok=w==VK_F5?profile_path&&nflstreet_option_save(profile_path,state.profile):restore_profile();SetWindowTextA(h,ok?(w==VK_F5?"NFL Street Options - settings saved / F9 reload":"NFL Street Options - settings loaded / F5 save"):"NFL Street Options - settings operation failed; current settings retained");InvalidateRect(h,NULL,FALSE);return 0;}if(w==VK_ESCAPE){DestroyWindow(h);return 0;}unsigned event=w==VK_UP?2:w==VK_DOWN?3:w==VK_LEFT?4:w==VK_RIGHT?5:w==VK_BACK?6:w==VK_RETURN?7:0;if(event){if(!control(event))SetWindowTextA(h,"NFL Street Options - operation unavailable");InvalidateRect(h,NULL,FALSE);}return 0;}if(msg==WM_DESTROY){PostQuitMessage(0);return 0;}return DefWindowProcA(h,msg,w,l);}
int main(int argc,char**argv){if(argc>=5&&!strcmp(argv[argc-2],"--profile")){profile_path=argv[argc-1];argc-=2;}if(argc!=3&&argc!=5){fprintf(stderr,"Usage: options-preview.exe raw-options.bin unpacked-directory [--captures/--continuation-captures/--store-captures prefix] [--profile native-save-file]\n");return 2;}if(argc==5&&strcmp(argv[3],"--captures")&&strcmp(argv[3],"--continuation-captures")&&strcmp(argv[3],"--store-captures")&&strcmp(argv[3],"--startup-capture"))return 2;screen=read(argv[1],&screen_size);source_screen=screen?malloc(screen_size):NULL;if(!source_screen)return 4;memcpy(source_screen,screen,screen_size);unsigned relocated;if(!screen||!nflstreet_ui_relocate(screen,screen_size,BASE,&relocated))return 3;for(unsigned e=0;e<=2;e+=2){char path[4096];size_t n;snprintf(path,sizeof path,"%s/uis_font/%05u.bin",argv[2],e);font_bytes[e]=read(path,&n);if(!font_bytes[e]||!nflstreet_font_open(font_bytes[e],n,&state.fonts[e])||!nflstreet_font_atlas(&state.fonts[e],0,&atlas[e][0])||!nflstreet_font_atlas(&state.fonts[e],1,&atlas[e][1]))return 4;}
 for(unsigned a=1;a<=3;a+=2)for(unsigned e=0;e<(a==1?16:2);++e){char path[4096];size_t n;snprintf(path,sizeof path,"%s/%s/%05u.bin",argv[2],a==1?"uis_comn":"uis_butt",e);uint8_t*b=read(path,&n);if(!b||!nflstreet_texture_open(b,n,&images[a][e]))return 4;free(b);}
 /* Explicit query26=true context bypasses the unimplemented card confirmation coroutine. */
 state.shared.callback_byte_6f78=1;state.shared.rates.clock=16;
 if(profile_path){nflstreet_option_defaults(state.profile);int loaded=nflstreet_option_load(profile_path,state.profile);if(!loaded){fprintf(stderr,"Settings file invalid or unreadable; preserved without overwrite: %s\n",profile_path);return 4;}}
 if(!nflstreet_ui_construct(screen,screen_size,BASE,0,UINT32_MAX,0x101,construct,&state,sizeof state)||!nflstreet_ui_options_screen_start(screen,screen_size,BASE,&state))return 5;
 canvas.width=854;canvas.height=640;canvas.bgra=malloc((size_t)canvas.width*canvas.height*4);if(!canvas.bgra)return 4;for(unsigned i=0;i<160;++i)if(!advance())return 6;
 if(argc==5&&!strcmp(argv[3],"--startup-capture")){if(!nflstreet_texture_bmp(&canvas,argv[4]))return 4;puts("Native Options fresh process startup from explicit settings file/local assets;source reconstruction and160 updates");return 0;}
 if(argc==5&&!strcmp(argv[3],"--store-captures")){
  if(!profile_path||GetFileAttributesA(profile_path)!=INVALID_FILE_ATTRIBUTES)return 10;
  uint32_t initial[22];memcpy(initial,state.profile,sizeof initial);
  /* Exercise source direction controls until a persistent word changes.
     Source headings/actions are traversed without assigning invented row roles. */
  unsigned altered=0,reverse=0;
  for(unsigned row=0;row<19&&!altered;++row){
   for(unsigned event=4;event<=5&&!altered;++event){if(!control(event))return 7;for(unsigned i=0;i<20;++i)if(!advance())return 6;if(memcmp(initial,state.profile,sizeof initial)){altered=1;reverse=event==4?5:4;}}
   if(!altered){if(!control(3))return 7;for(unsigned i=0;i<20;++i)if(!advance())return 6;}
  }
  uint32_t changed[22];memcpy(changed,state.profile,sizeof changed);if(!altered){fprintf(stderr,"Source direction sequence changed no persistent words\n");return 10;}if(!nflstreet_option_save(profile_path,changed))return 10;
  char path[4096];snprintf(path,sizeof path,"%s-saved.bmp",argv[4]);if(!nflstreet_texture_bmp(&canvas,path))return 4;
  if(!control(reverse)||!memcmp(changed,state.profile,sizeof changed)){fprintf(stderr,"Store check reverse control failed\n");return 10;}if(!restore_profile()||memcmp(changed,state.profile,sizeof changed)){fprintf(stderr,"Store check reload failed\n");return 10;}
  snprintf(path,sizeof path,"%s-reloaded.bmp",argv[4]);if(!nflstreet_texture_bmp(&canvas,path))return 4;
  printf("Native source Options setting change -> atomic settings save -> further change -> disk reload/source reconstruction;22 words match;2 captures;save retained for fresh-process check\n");return 0;
 }
 if(argc==5&&!strcmp(argv[3],"--continuation-captures")){
  uint8_t*saved=malloc(screen_size);nflstreet_ui_options_screen*original=malloc(sizeof state);if(!saved||!original)return 4;memcpy(saved,screen,screen_size);*original=state;
  for(unsigned result=0;result<3;++result){memcpy(screen,saved,screen_size);state=*original;state.shared.callback_byte_6f78=0;state.coroutines.capacity=4;if(!control(6)||state.coroutines.count!=1||state.coroutines.frames[0].request.target!=0x60004)return 7;
   char path[4096];snprintf(path,sizeof path,"%s-%u-suspended.bmp",argv[4],result);if(!nflstreet_texture_bmp(&canvas,path))return 4;
   clear_effects();unsigned completed=0;if(!nflstreet_ui_options_screen_complete(screen,screen_size,BASE,&state,0x60004,result,&completed)||!completed||state.coroutines.count)return 7;
   for(unsigned tick=0;tick<160;++tick)if(!advance())return 6;if(state.return_count!=(result==2?1u:0u)||(result==2&&state.return_identity!=2))return 8;
   snprintf(path,sizeof path,"%s-%u-resumed.bmp",argv[4],result);if(!nflstreet_texture_bmp(&canvas,path))return 4;
  }puts("Native actual Options continuation:3 explicit recipient results,480 timed updates,6 captures;original recipient UI/card I/O not supplied");return 0;
 }
 if(argc==5){unsigned captures=0;for(unsigned step=0;step<55;++step){unsigned event=step?step<19?3:step<37?2:step&1?4:5:0;if(event&&!control(event))return 7;for(unsigned i=0;i<20;++i)if(!advance())return 6;char path[4096];snprintf(path,sizeof path,"%s-%02u.bmp",argv[4],step);if(!nflstreet_texture_bmp(&canvas,path))return 4;++captures;}if(!control(6))return 7;for(unsigned i=0;i<160;++i)if(!advance())return 6;if(state.return_count!=1||state.return_identity!=2)return 8;printf("Native Options: child construction/start,54 controls,%u rendered captures,source Back return2; supplied profile/query context,diagnostic rasterizer; card flows/audio playback separate\n",captures);return 0;}
 HINSTANCE inst=GetModuleHandleA(NULL);WNDCLASSA wc={0};wc.lpfnWndProc=window;wc.hInstance=inst;wc.lpszClassName="NFLStreetOptionsInspection";wc.hCursor=LoadCursorA(NULL,IDC_ARROW);if(!RegisterClassA(&wc))return 9;RECT r={0,0,854,640};AdjustWindowRect(&r,WS_OVERLAPPEDWINDOW,FALSE);HWND h=CreateWindowA(wc.lpszClassName,profile_path?"NFL Street Options - arrows / Enter / Backspace / F5 save / F9 reload":"NFL Street Options - arrows / Enter / Backspace",WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,r.right-r.left,r.bottom-r.top,NULL,NULL,inst,NULL);if(!h)return 9;ShowWindow(h,SW_SHOW);SetTimer(h,1,16,NULL);MSG msg;while(GetMessageA(&msg,NULL,0,0)>0){TranslateMessage(&msg);DispatchMessageA(&msg);}return 0;}
