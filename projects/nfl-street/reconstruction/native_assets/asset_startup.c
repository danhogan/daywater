#include "startup.h"
#include "texture.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
static nflstreet_texture display;
static int render_check,painted,render_failed;
static LRESULT CALLBACK window_proc(HWND w,UINT m,WPARAM a,LPARAM b){
    if(m==WM_DESTROY){PostQuitMessage(0);return 0;}
    if(m==WM_PAINT){
        PAINTSTRUCT ps;HDC dc=BeginPaint(w,&ps);RECT rect;GetClientRect(w,&rect);
        /* Composite alpha over a checkerboard before GDI scaling. */
        size_t count=(size_t)display.width*display.height;uint32_t *pixels=malloc(count*4);
        if(pixels){for(unsigned y=0;y<display.height;++y)for(unsigned x=0;x<display.width;++x){
            size_t i=(size_t)y*display.width+x;uint32_t p=display.bgra[i];unsigned alpha=p>>24;
            unsigned bg=((x/8+y/8)&1)?192:128,r=(p>>16)&255,g=(p>>8)&255,blue=p&255;
            pixels[i]=((r*alpha+bg*(255-alpha))/255)<<16|((g*alpha+bg*(255-alpha))/255)<<8|(blue*alpha+bg*(255-alpha))/255;
        }
        BITMAPINFO bi={0};bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=(LONG)display.width;
        bi.bmiHeader.biHeight=-(LONG)display.height;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;bi.bmiHeader.biCompression=BI_RGB;
        SetStretchBltMode(dc,COLORONCOLOR);
        int width=rect.right,height=(int)((size_t)width*display.height/display.width);
        if(height>rect.bottom){height=rect.bottom;width=(int)((size_t)height*display.width/display.height);}
        int lines=StretchDIBits(dc,(rect.right-width)/2,(rect.bottom-height)/2,width,height,0,0,(int)display.width,(int)display.height,pixels,&bi,DIB_RGB_COLORS,SRCCOPY);
        if(lines<=0)render_failed=1;else ++painted;
        free(pixels);}else render_failed=1;
        EndPaint(w,&ps);return 0;
    }
    if(m==WM_TIMER&&render_check){DestroyWindow(w);return 0;}
    return DefWindowProcA(w,m,a,b);
}
static int show(nflstreet_texture *t){
    display=*t;HINSTANCE instance=GetModuleHandleA(NULL);WNDCLASSA c={0};c.lpfnWndProc=window_proc;c.hInstance=instance;
    c.lpszClassName="NFLStreetNativeAssets";c.hCursor=LoadCursor(NULL,IDC_ARROW);c.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);
    if(!RegisterClassA(&c))return 0;
    HWND w=CreateWindowA(c.lpszClassName,"NFL Street native asset startup - texture preview",WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,CW_USEDEFAULT,800,600,NULL,NULL,instance,NULL);
    if(!w)return 0;
    ShowWindow(w,SW_SHOW);UpdateWindow(w);
    if(render_check&&!SetTimer(w,1,250,NULL)){DestroyWindow(w);return 0;}
    MSG msg;int result;
    while((result=GetMessageA(&msg,NULL,0,0))>0){TranslateMessage(&msg);DispatchMessageA(&msg);}
    printf("Windows GDI paint passes %d, failed %d\n",painted,render_failed);
    return result==0&&painted>0&&!render_failed;
}
#endif
static uint32_t be32(const uint8_t *p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static int read_archive(const char *path,uint8_t **bytes,nflstreet_archive *a){
    FILE *f=fopen(path,"rb");if(!f){fprintf(stderr,"Cannot open %s\n",path);return 0;}
    int ok=0;if(fseek(f,0,SEEK_END))goto end;long size=ftell(f);
    if(size<16||size>128*1024*1024||fseek(f,0,SEEK_SET))goto end;
    *bytes=malloc((size_t)size);if(!*bytes)goto end;
    if(fread(*bytes,1,(size_t)size,f)!=(size_t)size||!nflstreet_archive_open(*bytes,(size_t)size,a)){free(*bytes);*bytes=NULL;goto end;}
    ok=1;
end: fclose(f);if(!ok)fprintf(stderr,"Invalid or oversized TERF: %s\n",path);return ok;
}
int main(int argc,char **argv){
    if(argc<4||argc>5){fprintf(stderr,"Usage: asset-startup animdata.dat texture-archive.dat output.bmp [--headless|--render-check]\n");return 2;}
    int headless=argc==5&&!strcmp(argv[4],"--headless");
    if(argc==5&&!headless&&strcmp(argv[4],"--render-check"))return 2;
#ifdef _WIN32
    render_check=argc==5&&!strcmp(argv[4],"--render-check");
#endif
    uint8_t *ab=NULL,*tb=NULL;nflstreet_archive anim,tex;nflstreet_native_banks banks={0};nflstreet_texture first={0};int status=1;
    if(!read_archive(argv[1],&ab,&anim)||!read_archive(argv[2],&tb,&tex)||!nflstreet_native_banks_open(&anim,&banks))goto done;
    printf("Native startup: %u animation entries; %u direct banks; %zu owned bytes; no captured registry.\n",anim.count,banks.count,banks.store.arena_size);
    unsigned duration_count=0,unsupported_programs=0;
    for(unsigned i=0;i<banks.count;++i){const uint8_t *bank;if(!nflstreet_asset_bank(&banks.store,i,&bank)||!bank)goto done;
        const uint8_t *programs=nflstreet_asset_read(&banks.store,be32(bank+12),4);if(!programs)goto done;
        unsigned n=be32(programs);programs=nflstreet_asset_read(&banks.store,be32(bank+12),4+(size_t)n*4);if(!programs)goto done;
        uint32_t bank_id=be32(banks.registry+i*28+12);
        for(unsigned j=0;j<n;++j){uint32_t id=be32(programs+4+j*4);
            if(id<bank_id || id-bank_id>=banks.bank_size[i])goto done;
            size_t remaining=banks.bank_size[i]-(id-bank_id);
            const uint8_t *p=nflstreet_asset_read(&banks.store,id,1);float duration;
            if(!p||!nflstreet_animation_program_duration(p,remaining,&duration)){++unsupported_programs;continue;}
            if(i==0&&(j==75||j==84))printf("Common bank clip %u duration %.9g\n",j,(double)duration);
            ++duration_count;
        }
    }
    printf("Native animation program scans: %u, unsupported %u\n",duration_count,unsupported_programs);
    unsigned images=0,compressed=0,other=0;
    for(unsigned i=0;i<tex.count;++i){nflstreet_archive_entry e;uint8_t *b=NULL;size_t n;nflstreet_texture t={0};
        if(!nflstreet_archive_get(&tex,i,&e))goto done;
        if(!nflstreet_archive_decode(&e,&b,&n)){
            if(e.codec==0||e.codec==5){fprintf(stderr,"Invalid asset/checksum at entry %u\n",i);goto done;}
            ++compressed;continue;
        }
        if(nflstreet_texture_open(b,n,&t)){printf("Texture entry %u: %u x %u\n",i,t.width,t.height);++images;
            if(!first.bgra)first=t;else nflstreet_texture_close(&t);
        }else ++other;
        free(b);
    }
    printf("Textures decoded %u, compressed unsupported %u, other unsupported %u\n",images,compressed,other);
    if(!images||!nflstreet_texture_bmp(&first,argv[3]))goto done;
#ifdef _WIN32
    if(!headless&&!show(&first))goto done;
#else
    if(!headless){fprintf(stderr,"Interactive rendering requires Windows. Use --headless.\n");goto done;}
#endif
    status=0;
done: if(status)fprintf(stderr,"Native asset startup failed; no match was started.\n");
    nflstreet_texture_close(&first);nflstreet_native_banks_close(&banks);free(ab);free(tb);return status;
}
