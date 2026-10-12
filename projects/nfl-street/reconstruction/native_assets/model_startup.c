#include "archive.h"
#include "mesh_render.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef NFLSTREET_SCENE_PREVIEW
#include "scene.h"
static int attach_props(nflstreet_scene *scene,const nflstreet_archive *env,const char *path){
    const char *slash=strrchr(path,'/'),*back=strrchr(path,'\\');
    if(back&&(!slash||back>slash))slash=back;
    size_t prefix=slash?(size_t)(slash-path)+1:0;
    uint8_t *owned[2]={0};nflstreet_archive archives[2];int ok=0;
    const char *names[2]={"objdefs.dat","objmodel.dat"};
    for(unsigned i=0;i<2;++i){char filename[4096];
        if(prefix+strlen(names[i])+1>sizeof filename)goto done;
        memcpy(filename,path,prefix);strcpy(filename+prefix,names[i]);
        FILE *f=fopen(filename,"rb");if(!f)goto done;
        if(fseek(f,0,SEEK_END)){fclose(f);goto done;}long size=ftell(f);
        if(size<16||size>128*1024*1024||fseek(f,0,SEEK_SET)){fclose(f);goto done;}
        owned[i]=malloc((size_t)size);if(!owned[i]){fclose(f);goto done;}
        int read=fread(owned[i],1,(size_t)size,f)==(size_t)size;fclose(f);
        if(!read||!nflstreet_archive_open(owned[i],(size_t)size,archives+i))goto done;
    }
    ok=nflstreet_scene_attach_props(scene,env,archives,archives+1);
done:free(owned[0]);free(owned[1]);return ok;
}
#endif
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
static nflstreet_texture frame;
static const nflstreet_mesh *current_mesh;
static float yaw=0.6f;
static float pitch=0.35f,zoom=1;
static int checking,paints,error;
static LRESULT CALLBACK proc(HWND w,UINT m,WPARAM a,LPARAM b){
    if(m==WM_DESTROY){PostQuitMessage(0);return 0;}
    if(m==WM_TIMER&&checking){DestroyWindow(w);return 0;}
    if(m==WM_KEYDOWN&&(a==VK_LEFT||a==VK_RIGHT||a==VK_UP||a==VK_DOWN||a==VK_OEM_PLUS||a==VK_OEM_MINUS)){
        nflstreet_texture next={0};
        if(a==VK_LEFT||a==VK_RIGHT)yaw+=a==VK_LEFT?-0.15f:0.15f;
        if(a==VK_UP&&pitch<1.4f)pitch+=0.1f;
        if(a==VK_DOWN&&pitch>-1.4f)pitch-=0.1f;
        if(a==VK_OEM_PLUS&&zoom<4)zoom+=0.1f;
        if(a==VK_OEM_MINUS&&zoom>0.3f)zoom-=0.1f;
        if(nflstreet_mesh_render_view(current_mesh,frame.width,frame.height,yaw,pitch,zoom,&next)){nflstreet_texture_close(&frame);frame=next;InvalidateRect(w,NULL,FALSE);}else error=1;
        return 0;
    }
    if(m==WM_PAINT){PAINTSTRUCT ps;HDC dc=BeginPaint(w,&ps);RECT r;GetClientRect(w,&r);BITMAPINFO bi={0};
        bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=(LONG)frame.width;bi.bmiHeader.biHeight=-(LONG)frame.height;
        bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;bi.bmiHeader.biCompression=BI_RGB;
        int n=StretchDIBits(dc,0,0,r.right,r.bottom,0,0,(int)frame.width,(int)frame.height,frame.bgra,&bi,DIB_RGB_COLORS,SRCCOPY);
        if(n>0)++paints;else error=1;
        EndPaint(w,&ps);return 0;
    }
    return DefWindowProcA(w,m,a,b);
}
static int show(nflstreet_texture *image,const nflstreet_mesh *mesh,int check){
    frame=*image;current_mesh=mesh;pitch=mesh->inspection_opaque?0.75f:0.35f;checking=check;HINSTANCE instance=GetModuleHandleA(NULL);WNDCLASSA c={0};
    c.lpfnWndProc=proc;c.hInstance=instance;c.lpszClassName="NFLStreetNativeMesh";c.hCursor=LoadCursor(NULL,IDC_ARROW);
    if(!RegisterClassA(&c))return 0;
    char title[256];snprintf(title,sizeof title,"Native inspection: %s (arrows orbit, +/- zoom)",mesh->name);
    HWND w=CreateWindowA(c.lpszClassName,title,WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,820,680,NULL,NULL,instance,NULL);
    if(!w)return 0;
    ShowWindow(w,SW_SHOW);UpdateWindow(w);
    if(check&&!SetTimer(w,1,250,NULL)){DestroyWindow(w);return 0;}
    MSG msg;int n;while((n=GetMessageA(&msg,NULL,0,0))>0){TranslateMessage(&msg);DispatchMessageA(&msg);}
    *image=frame;printf("Windows mesh paint passes %d, failed %d\n",paints,error);return n==0&&paints>0&&!error;
}
#endif
int main(int argc,char **argv){
#ifdef NFLSTREET_SCENE_PREVIEW
    if(argc<4||argc>7){fprintf(stderr,"Usage: scene-startup envrnmt.dat venue-prefix output.bmp [--headless|--render-check] [--props] [private-mesh-dump]\n");return 2;}
#else
    if(argc<4||argc>6){fprintf(stderr,"Usage: model-startup archive.dat entry-index output.bmp [--headless|--render-check] [private-mesh-dump]\n");return 2;}
#endif
    const char *dump_path=NULL;
#ifdef NFLSTREET_SCENE_PREVIEW
    int headless=0,check=0,props=0;
    for(int i=4;i<argc;++i){
        if(!strcmp(argv[i],"--headless")){if(headless||check)return 2;headless=1;}
        else if(!strcmp(argv[i],"--render-check")){if(headless||check)return 2;check=1;}
        else if(!strcmp(argv[i],"--props")){if(props)return 2;props=1;}
        else {if(dump_path||argv[i][0]=='-')return 2;dump_path=argv[i];}
    }
#else
    int headless=argc>=5&&!strcmp(argv[4],"--headless"),check=argc>=5&&!strcmp(argv[4],"--render-check");
    if(argc>=5&&!headless&&!check)return 2;
    if(argc==6)dump_path=argv[5];
#endif
#ifndef NFLSTREET_SCENE_PREVIEW
    char *end;errno=0;unsigned long index=strtoul(argv[2],&end,10);if(errno||!*argv[2]||*end||index>65535)return 2;
#endif
    FILE *f=fopen(argv[1],"rb");if(!f)return 1;
    if(fseek(f,0,SEEK_END)){fclose(f);return 1;}long size=ftell(f);
    if(size<16||size>128*1024*1024||fseek(f,0,SEEK_SET)){fclose(f);return 1;}
    uint8_t *archive=malloc((size_t)size);if(!archive){fclose(f);return 1;}
    int ok=fread(archive,1,(size_t)size,f)==(size_t)size;fclose(f);
    nflstreet_archive a;uint8_t *bytes=NULL;nflstreet_mesh mesh={0};nflstreet_texture image={0};int status=1;
    if(!ok||!nflstreet_archive_open(archive,(size_t)size,&a)){free(archive);return 1;}
#ifdef NFLSTREET_SCENE_PREVIEW
    nflstreet_scene scene={0};
    if(!nflstreet_scene_open(&a,argv[2],&scene))goto done;
    if(props&&!attach_props(&scene,&a,argv[1]))goto done;
    mesh=scene.mesh;memset(&scene.mesh,0,sizeof scene.mesh);
    for(unsigned i=0;i<scene.layers;++i)printf("Scene layer %u: original entry %u, %zu triangles\n",i,scene.entries[i],scene.triangles[i]);
    printf("Scene instances %u: rendered %u, nonvisual %u, unsupported %u\n",scene.instances,scene.rendered_instances,scene.nonvisual_instances,scene.unsupported_instances);
    printf("Inspection scene: original asset coordinates, approximate instance rotations, asset poses; original draw state remains open\n");
#else
    nflstreet_archive_entry entry;size_t n;
    if(!nflstreet_archive_get(&a,(unsigned)index,&entry)||!nflstreet_archive_decode(&entry,&bytes,&n)||!nflstreet_mesh_open(bytes,n,&mesh))goto done;
#endif
    printf("Original model %s: %u draws, %zu triangles\n",mesh.name,mesh.draws,mesh.count);
    printf("Bounds %.9g %.9g %.9g -> %.9g %.9g %.9g\n",(double)mesh.minimum[0],(double)mesh.minimum[1],(double)mesh.minimum[2],(double)mesh.maximum[0],(double)mesh.maximum[1],(double)mesh.maximum[2]);
    unsigned textured=0;for(unsigned i=0;i<mesh.material_count;++i)if(mesh.materials[i].texture_index>=0)++textured;
    printf("Materials %u, embedded OnePass resolutions %u; inspection lighting\n",mesh.material_count,textured);
    if(mesh.ignored_color_indices)printf("Inspection-only color indices outside declared arrays: %u; original color behavior unresolved\n",mesh.ignored_color_indices);
    if(dump_path&&!nflstreet_mesh_write(&mesh,dump_path))goto done;
    if(!nflstreet_mesh_render(&mesh,768,640,0.6f,&image)||!nflstreet_texture_bmp(&image,argv[3]))goto done;
#ifdef _WIN32
    if(!headless&&!show(&image,&mesh,check))goto done;
#else
    if(!headless){fprintf(stderr,"Interactive rendering requires Windows\n");goto done;}
#endif
    status=0;
done:if(status)fprintf(stderr,"Unsupported/invalid static mesh or renderer failure; no match started\n");
    nflstreet_texture_close(&image);nflstreet_mesh_close(&mesh);
#ifdef NFLSTREET_SCENE_PREVIEW
    nflstreet_scene_close(&scene);
#endif
    free(bytes);free(archive);return status;
}
