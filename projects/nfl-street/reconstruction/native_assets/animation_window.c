/* Plays frames produced by animation_preview's native pose/mesh renderer.
   This inspection player preloads rendered frames; it is not a live match. */
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint8_t **frames;static unsigned count,index,width,height,paints,ticks;static int check;
static unsigned word(const uint8_t *p){return p[0]|(unsigned)p[1]<<8|(unsigned)p[2]<<16|(unsigned)p[3]<<24;}
static LRESULT CALLBACK proc(HWND window,UINT message,WPARAM a,LPARAM b){
    if(message==WM_DESTROY){PostQuitMessage(0);return 0;}
    if(message==WM_TIMER){index=(index+1)%count;++ticks;InvalidateRect(window,NULL,FALSE);
        if(check&&ticks>=count+2)DestroyWindow(window);return 0;}
    if(message==WM_PAINT){PAINTSTRUCT ps;HDC dc=BeginPaint(window,&ps);RECT rect;GetClientRect(window,&rect);
        BITMAPINFO bi={0};bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=(LONG)width;bi.bmiHeader.biHeight=(LONG)height;
        bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;bi.bmiHeader.biCompression=BI_RGB;
        if(StretchDIBits(dc,0,0,rect.right,rect.bottom,0,0,(int)width,(int)height,frames[index],&bi,DIB_RGB_COLORS,SRCCOPY)>0)++paints;
        EndPaint(window,&ps);return 0;}
    return DefWindowProcA(window,message,a,b);
}
int main(int argc,char **argv){if(argc<4||argc>5)return 2;count=(unsigned)strtoul(argv[2],NULL,10);unsigned delay=(unsigned)strtoul(argv[3],NULL,10);
    check=argc==5&&!strcmp(argv[4],"--render-check");if(!count||count>600||delay<16||delay>1000||(argc==5&&!check))return 2;
    frames=calloc(count,sizeof *frames);if(!frames)return 1;
    for(unsigned i=0;i<count;++i){char path[4096];if(snprintf(path,sizeof path,"%s/frame-%03u.bmp",argv[1],i)>=(int)sizeof path)return 2;
        FILE *f=fopen(path,"rb");if(!f)return 3;uint8_t header[54];if(fread(header,1,54,f)!=54||memcmp(header,"BM",2)||word(header+10)!=54||word(header+30)!=0||header[28]!=32)return 3;
        unsigned w=word(header+18),h=word(header+22);if(!w||w>4096||!h||h>4096||(i&&(w!=width||h!=height)))return 3;width=w;height=h;
        size_t bytes=(size_t)w*h*4;frames[i]=malloc(bytes);if(!frames[i]||fread(frames[i],1,bytes,f)!=bytes)return 3;fclose(f);
    }
    HINSTANCE instance=GetModuleHandleA(NULL);WNDCLASSA cls={0};cls.lpfnWndProc=proc;cls.hInstance=instance;cls.lpszClassName="NFLStreetNativeAnimation";cls.hCursor=LoadCursor(NULL,IDC_ARROW);
    if(!RegisterClassA(&cls))return 4;HWND window=CreateWindowA(cls.lpszClassName,"NFL Street native clip inspection - pre-rendered native poses",WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,800,600,NULL,NULL,instance,NULL);
    if(!window)return 4;ShowWindow(window,SW_SHOW);UpdateWindow(window);if(!SetTimer(window,1,delay,NULL))return 4;
    MSG msg;int result;while((result=GetMessageA(&msg,NULL,0,0))>0){TranslateMessage(&msg);DispatchMessageA(&msg);}
    printf("Native Windows animation playback: %u paints, %u timer ticks, %u frames\n",paints,ticks,count);
    for(unsigned i=0;i<count;++i)free(frames[i]);free(frames);
    return result==0&&(check?(paints>1&&ticks>=count):paints>0)?0:1;
}
