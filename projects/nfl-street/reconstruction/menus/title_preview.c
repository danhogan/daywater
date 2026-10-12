/* Independent Windows inspection: original title text resource -> local drawing.
   Host canvas/compositing are diagnostic. Actual title activation/fade and
   optional background portrait bytecode execute; full navigation unfinished. */
#include <windows.h>
#include "screen.h"
#include "text.h"
#include "script.h"
#include "title.h"
#include "broadcast.h"
#include "background.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
static nflstreet_texture canvas;
static uint8_t*active_screen;
static size_t active_size;
static nflstreet_ui_title title_state;
static nflstreet_font active_font;
static uint8_t active_object[40];
static const char*active_text;
static uint8_t*background_screen;
static size_t background_size;
static uint32_t background_flags;
static nflstreet_ui_rates background_rates;
static nflstreet_texture logo;
static unsigned background_root;
static int full_background;
static nflstreet_ui_background animated_background;
static nflstreet_texture portraits[22];
static nflstreet_texture backdrop;
static int interactive_menu;
static int logo_x,logo_y;
static unsigned be(const uint8_t*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static float source_float(const uint8_t*p){unsigned w=be(p);float f;memcpy(&f,&w,4);return f;}
static int redraw_title(int animated);
static int background_service(void*ctx,uint8_t*b,size_t n,uint32_t base,const nflstreet_ui_service_request*q,uint32_t*result){(void)result;return nflstreet_ui_rate_request(ctx,b,n,base,0,q);}
static LRESULT CALLBACK window(HWND h,UINT msg,WPARAM w,LPARAM l){
 if(msg==WM_PAINT){PAINTSTRUCT ps;HDC dc=BeginPaint(h,&ps);BITMAPINFO bi={0};bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=(LONG)canvas.width;bi.bmiHeader.biHeight=-(LONG)canvas.height;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;bi.bmiHeader.biCompression=BI_RGB;SetDIBitsToDevice(dc,0,0,canvas.width,canvas.height,0,0,0,canvas.height,canvas.bgra,&bi,DIB_RGB_COLORS);EndPaint(h,&ps);return 0;}
 if(msg==WM_TIMER){if(!nflstreet_ui_title_advance(active_screen,active_size,0x01000000,&title_state,16)||(full_background&&!nflstreet_ui_background_advance(background_screen,background_size,0x03000000,&animated_background,16))||!redraw_title(1)){KillTimer(h,1);SetWindowTextA(h,"NFL Street title inspection - activation failed");return 0;}InvalidateRect(h,NULL,FALSE);if(!full_background&&!title_state.rates.count)KillTimer(h,1);return 0;}
 if(msg==WM_KEYDOWN&&w==VK_ESCAPE){DestroyWindow(h);return 0;}if(msg==WM_DESTROY){PostQuitMessage(0);return 0;}return DefWindowProcA(h,msg,w,l);
}
static uint8_t*read(const char*path,size_t*size){FILE*f=fopen(path,"rb");if(!f)return NULL;if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}long n=ftell(f);rewind(f);if(n<32||n>8*1024*1024){fclose(f);return NULL;}uint8_t*b=malloc((size_t)n);if(!b||fread(b,1,(size_t)n,f)!=(size_t)n){free(b);b=NULL;}fclose(f);*size=(size_t)n;return b;}
static unsigned channel(float f){return f<=0?0:f>=1?255:(unsigned)(f*255);}
static void blit(const nflstreet_texture*atlas,const nflstreet_glyph_box*g,int ox,int oy,const float color[4]){
 unsigned rgb[3]={channel(color[2]),channel(color[1]),channel(color[0])},ca=channel(color[3]);
 for(unsigned y=0;y<g->height;++y)for(unsigned x=0;x<g->width;++x){int px=ox+g->x+(int)x,py=oy+g->y+(int)y;if(px<0||py<0||(unsigned)px>=canvas.width||(unsigned)py>=canvas.height)continue;
  uint32_t src=atlas->bgra[(g->v+y)*atlas->width+g->u+x];unsigned a=(src>>24)*ca/255;uint32_t*dst=canvas.bgra+(size_t)py*canvas.width+px,result=0xff000000;
  for(unsigned k=0;k<3;++k){unsigned val=rgb[k]*((src>>(k*8))&255)/255,old=(*dst>>(k*8))&255;result|=((val*a+old*(255-a))/255)<<(k*8);}*dst=result;
 }
}
static void draw_logo(void){
 unsigned ca=channel(source_float(background_screen+background_root+0x48));
 for(unsigned y=0;y<logo.height;++y)for(unsigned x=0;x<logo.width;++x){int px=logo_x+(int)x,py=logo_y+(int)y;if(px<0||py<0||(unsigned)px>=canvas.width||(unsigned)py>=canvas.height)continue;
  uint32_t src=logo.bgra[(size_t)y*logo.width+x],*dst=canvas.bgra+(size_t)py*canvas.width+px,result=0xff000000;unsigned a=(src>>24)*ca/255;
  for(unsigned k=0;k<3;++k)result|=(((((src>>(k*8))&255)*a+((*dst>>(k*8))&255)*(255-a))/255)<<(k*8));*dst=result;
 }
}
static void source_quad(const nflstreet_texture*t,int ox,int oy,float width,float height,float v0,float v1,unsigned ca){
 for(unsigned y=0;y<(unsigned)height;++y)for(unsigned x=0;x<(unsigned)width;++x){int px=ox+(int)x,py=oy+(int)y;if(px<0||py<0||(unsigned)px>=canvas.width||(unsigned)py>=canvas.height)continue;
  unsigned u=(unsigned)((float)x/width*t->width),v=(unsigned)((v0+(float)y/height*(v1-v0))*t->height);if(u>=t->width)u=t->width-1;if(v>=t->height)v=t->height-1;
  uint32_t src=t->bgra[(size_t)v*t->width+u],*dst=canvas.bgra+(size_t)py*canvas.width+px,result=0xff000000;unsigned a=(src>>24)*ca/255;
  for(unsigned k=0;k<3;++k)result|=(((((src>>(k*8))&255)*a+((*dst>>(k*8))&255)*(255-a))/255)<<(k*8));*dst=result;
 }
}
static int redraw_title(int animated){
 float context[8]={1,1,1,1,0,0,0,0};
 if(animated){unsigned table=be(active_screen+4)-0x01000000,root=be(active_screen+table)-0x01000000;if(root>active_size||0x5c>active_size-root)return 0;
  /* Original 801D8268 supplies root+3C and +4C to 801D7E74.
     Diagnostic top-level parent: identity multiplier and zero addition. */
  for(unsigned i=0;i<4;++i){context[i]=source_float(active_screen+root+0x3c+i*4);context[4+i]=source_float(active_screen+root+0x4c+i*4);}if(context[3]<0)context[3]=0;if(context[3]>1)context[3]=1;
 }
 nflstreet_ui_text_plan plan;nflstreet_glyph_box boxes[354];unsigned count;
 if(!nflstreet_ui_text_draw(active_object,active_text,context,&plan)||!nflstreet_font_layout(&active_font,active_text,plan.flags,0,boxes,354,&count))return 0;
 for(unsigned i=0;i<canvas.width*canvas.height;++i)canvas.bgra[i]=0xff202020;
 if(full_background){
  /* Source class0 archive7 entry0; diagnostic full image, identity root1. */
  source_quad(&backdrop,0,0,427,480,0,1,255);
  unsigned table=be(background_screen+4)-0x03000000,root=be(background_screen+table+60)-0x03000000;
  unsigned obj=be(background_screen+root+0x8c);if(obj+16>background_size)return 0;unsigned entry=(unsigned)background_screen[obj+4]<<8|background_screen[obj+5];if(entry>=22)return 0;
  const nflstreet_texture*t=portraits+entry;float alpha=source_float(background_screen+root+0x48),sx=source_float(background_screen+root+0x2c),sy=source_float(background_screen+root+0x30);
  int ox=(int)source_float(background_screen+root+8),oy=(int)source_float(background_screen+root+12);unsigned ca=channel(alpha);
  if(!isfinite(sx)||!isfinite(sy)||sx<=0||sy<=0||sx>4||sy>4)return 0;
  /* Original801D7AD8: position frac4, UV frac8. Diagnostic nearest sampling. */
  unsigned qw=(unsigned)background_screen[obj+0x3a]<<8|background_screen[obj+0x3b],qh=(unsigned)background_screen[obj+0x42]<<8|background_screen[obj+0x43];
  unsigned v0=(unsigned)background_screen[obj+0x26]<<8|background_screen[obj+0x27],v1=(unsigned)background_screen[obj+0x2e]<<8|background_screen[obj+0x2f];
  if(!qw||!qh||v1<v0||v1>256)return 0;source_quad(t,ox,oy,qw/16.0f*sx,qh/16.0f*sy,v0/256.0f,v1/256.0f,ca);
 }
 if(background_screen)draw_logo();
 if(plan.visible)for(unsigned pass=0;pass<2;++pass){nflstreet_texture atlas={0};if(!nflstreet_font_atlas(&active_font,pass,&atlas))return 0;for(unsigned i=0;i<count;++i)blit(&atlas,boxes+i,(int)plan.position[0],(int)plan.position[1],plan.color[pass==0?1:0]);nflstreet_texture_close(&atlas);}return 1;
}
int main(int argc,char**argv){
 char*menu_args[8];if(argc==7&&!strcmp(argv[3],"--menu-window")){interactive_menu=1;menu_args[0]=argv[0];menu_args[1]=argv[1];menu_args[2]=argv[2];menu_args[3]="--menu-captures";menu_args[4]="";menu_args[5]=argv[4];menu_args[6]=argv[5];menu_args[7]=argv[6];argv=menu_args;argc=8;}

 if(argc!=3&&argc!=4&&argc!=5&&argc!=7&&argc!=8){fprintf(stderr,"Usage: title-preview.exe nested-title.bin uis_font-directory [capture.bmp|--script-check|--activation-captures prefix|--flow-captures prefix nested-background.bin source-logo.bin|--menu-captures prefix nested-background.bin source-logo.bin uis_bgpl-directory|--menu-window nested-background.bin source-logo.bin uis_bgpl-directory]\n");return 2;}
 size_t size;uint8_t*screen=read(argv[1],&size);unsigned relocated;if(!screen||!nflstreet_ui_relocate(screen,size,0x01000000,&relocated))return 3;
 if(argc==4&&!strcmp(argv[3],"--script-check")){
  uint8_t*before=malloc(size);if(!before)return 8;memcpy(before,screen,size);
  unsigned starts[3]={0x4d1,0x4e1,0x4f1};
  for(unsigned j=0;j<3;++j){nflstreet_ui_script s={0};s.sp=7;s.pc=0x01000000+starts[j];s.words[1]=UINT32_MAX;s.words[2]=s.words[5]=0x01000098;
   if(!nflstreet_ui_script_run(screen,size,0x01000000,0x02000000,&s,1000)||s.sp!=6)return 12;
   uint32_t value=(uint32_t)screen[0x108]<<24|(uint32_t)screen[0x109]<<16|(uint32_t)screen[0x10a]<<8|screen[0x10b];
   if(j==0?(value!=0||s.flags!=UINT32_MAX):j==1?(value!=1||s.flags!=0):(value!=1||s.words[4]!=1))return 12;
  }memcpy(screen,before,size);free(before);free(screen);puts("Native Windows title helper inspection passed: three source routines; owned root-cell/stack/flags only, no game callback or input mapping.");return 0;
 }
 uint8_t object[40];const char*text;if(!nflstreet_ui_text_find(screen,size,0x01000000,0,0,object,&text)||object[3]!=2||object[2]>3)return 4;
 char path[4096];snprintf(path,sizeof path,"%s/%05u.bin",argv[2],object[2]);size_t fontsize;uint8_t*bytes=read(path,&fontsize);nflstreet_font font;if(!bytes||!nflstreet_font_open(bytes,fontsize,&font))return 5;
 float context[8]={1,1,1,1,0,0,0,0};nflstreet_ui_text_plan plan;if(!nflstreet_ui_text_prepare(object,&font,text)||!nflstreet_ui_text_draw(object,text,context,&plan))return 6;
 nflstreet_glyph_box boxes[354];unsigned count;if(!nflstreet_font_layout(&font,text,plan.flags,0,boxes,354,&count))return 7;
 canvas.width=1024;canvas.height=768;canvas.bgra=malloc((size_t)canvas.width*canvas.height*4);if(!canvas.bgra)return 8;
 for(unsigned i=0;i<canvas.width*canvas.height;++i)canvas.bgra[i]=0xff202020;
 if(plan.visible)for(unsigned pass=0;pass<2;++pass){nflstreet_texture atlas={0};if(!nflstreet_font_atlas(&font,pass,&atlas))return 9;for(unsigned i=0;i<count;++i)blit(&atlas,boxes+i,(int)plan.position[0],(int)plan.position[1],plan.color[pass==0?1:0]);nflstreet_texture_close(&atlas);}
 printf("Source title text: %s; font archive=%d entry=%d; local position=(%.0f,%.0f,%.0f); %u glyphs. Host 1024x768 canvas/alpha composition; original projection and navigation unfinished.\n",text,plan.archive,plan.entry,plan.position[0],plan.position[1],plan.position[2],count);
 active_screen=screen;active_size=size;active_font=font;active_text=text;memcpy(active_object,object,40);
 if(argc==5||argc==7||argc==8){if(strcmp(argv[3],argc==5?"--activation-captures":argc==7?"--flow-captures":"--menu-captures"))return 2;title_state.rates.clock=16;if(!nflstreet_ui_title_activate(screen,size,0x01000000,&title_state))return 12;
  if(argc==7||argc==8){background_screen=read(argv[5],&background_size);if(!background_screen||!nflstreet_ui_relocate(background_screen,background_size,0x03000000,&relocated)||be(background_screen)!=4)return 12;
   unsigned table=be(background_screen+4)-0x03000000;if(table>background_size||80>background_size-table)return 12;unsigned root=be(background_screen+table)-0x03000000;background_root=be(background_screen+table+40)-0x03000000;
   if(root>background_size||0x1d4>background_size-root||background_root>background_size||0x5c>background_size-background_root)return 12;
   memset(background_screen+root+0x158,0,4);memset(background_screen+root+0x1d0,0,4);logo_x=(int)source_float(background_screen+background_root+8);logo_y=(int)source_float(background_screen+background_root+12);
   size_t logo_size;uint8_t*logo_bytes=read(argv[6],&logo_size);if(!logo_bytes||!nflstreet_texture_open(logo_bytes,logo_size,&logo)||logo.width!=512||logo.height!=128)return 12;free(logo_bytes);
   if(argc==8){full_background=1;
    char backdrop_path[4096];if(strlen(argv[6])+1>sizeof backdrop_path)return 12;strcpy(backdrop_path,argv[6]);char*slash=strrchr(backdrop_path,'/'),*backslash=strrchr(backdrop_path,'\\');if(backslash&&(!slash||backslash>slash))slash=backslash;if(!slash||sizeof backdrop_path-(size_t)(slash+1-backdrop_path)<10)return 12;strcpy(slash+1,"00000.bin");size_t backdrop_size;uint8_t*backdrop_bytes=read(backdrop_path,&backdrop_size);if(!backdrop_bytes||!nflstreet_texture_open(backdrop_bytes,backdrop_size,&backdrop))return 12;free(backdrop_bytes);
animated_background.rates.clock=16;animated_background.random_sample=0;
    for(unsigned i=0;i<22;++i){char asset_path[4096];snprintf(asset_path,sizeof asset_path,"%s/%05u.bin",argv[7],i);size_t bytes_size;uint8_t*asset=read(asset_path,&bytes_size);if(!asset||!nflstreet_texture_open(asset,bytes_size,portraits+i))return 12;free(asset);animated_background.ready[i]=1;}
    if(!nflstreet_ui_background_activate(background_screen,background_size,0x03000000,&animated_background))return 12;
   }
   background_rates.clock=16;unsigned consumed=0;for(unsigned i=0;i<title_state.broadcasts;++i){nflstreet_ui_service_request*q=title_state.requests+i;if(q->property!=9&&!(full_background&&q->property==8))continue;unsigned executed;if(!nflstreet_ui_broadcast(background_screen,background_size,0x03000000,&background_flags,(uint16_t)q->property,q->argc,q->args,full_background?nflstreet_ui_background_service:background_service,full_background?(void*)&animated_background:(void*)&background_rates,full_background?sizeof animated_background:sizeof background_rates,&executed)||executed!=1)return 12;++consumed;}if(consumed!=(full_background?2u:1u))return 12;
   printf("Actual property9 recipient executed: source uis_bgmp archive7 entry2 logo, local position=(%d,%d); additional archive8 scripts execute only in --menu-captures. Source transforms/GX composition not fully verified.\n",logo_x,logo_y);
  }
  if(interactive_menu){if(!redraw_title(1))return 12;goto show_window;}
  for(unsigned tick=0;tick<=(full_background?2600u:16u);++tick){if(tick&&!nflstreet_ui_title_advance(screen,size,0x01000000,&title_state,16))return 12;if(tick&&background_screen&&!(full_background?nflstreet_ui_background_advance(background_screen,background_size,0x03000000,&animated_background,16):nflstreet_ui_rate_update(&background_rates,background_screen,background_size,0x03000000,0,16,0,NULL,NULL)))return 12;
   if(full_background&&tick!=0&&tick!=6&&tick!=20&&tick!=1250&&tick!=1260&&tick!=1280&&tick!=2500&&tick!=2520&&tick!=2600)continue;if(!redraw_title(1))return 12;char output[4096];snprintf(output,sizeof output,"%s-%02u.bmp",argv[4],tick);if(!nflstreet_texture_bmp(&canvas,output))return 10;}
  printf("Native source title activation/fade completed: flag=%u entry flags=%08x remaining title rates=%u requests retained=%u. Navigation remains unfinished.\n",title_state.title_flag,title_state.entry_flags,title_state.rates.count,title_state.broadcasts);for(unsigned i=0;i<22;++i)nflstreet_texture_close(portraits+i);nflstreet_texture_close(&backdrop);free(background_screen);nflstreet_texture_close(&logo);free(screen);free(bytes);nflstreet_texture_close(&canvas);return 0;
 }
 if(argc==4){int ok=nflstreet_texture_bmp(&canvas,argv[3]);free(screen);free(bytes);nflstreet_texture_close(&canvas);return ok?0:10;}
 title_state.rates.clock=16;if(!nflstreet_ui_title_activate(screen,size,0x01000000,&title_state)||!redraw_title(1))return 12;
 show_window:;
 HINSTANCE inst=GetModuleHandleA(NULL);WNDCLASSA wc={0};wc.lpfnWndProc=window;wc.hInstance=inst;wc.lpszClassName="NFLStreetOriginalTitleInspection";wc.hCursor=LoadCursorA(NULL,IDC_ARROW);if(!RegisterClassA(&wc))return 11;
 RECT rect={0,0,1024,768};AdjustWindowRect(&rect,WS_OVERLAPPEDWINDOW,FALSE);HWND h=CreateWindowA(wc.lpszClassName,"NFL Street original title and background - native inspection; navigation unfinished",WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,rect.right-rect.left,rect.bottom-rect.top,NULL,NULL,inst,NULL);if(!h)return 11;ShowWindow(h,SW_SHOW);SetTimer(h,1,16,NULL);MSG msg;while(GetMessageA(&msg,NULL,0,0)>0){TranslateMessage(&msg);DispatchMessageA(&msg);}free(screen);free(bytes);nflstreet_texture_close(&canvas);return 0;
}
