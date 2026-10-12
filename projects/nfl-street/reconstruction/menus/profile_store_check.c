/* Offline Windows filesystem checks; no original module or saved RAM. */
#include <windows.h>
#include "profile_store.h"
#include <stdio.h>
#include <string.h>
int main(int argc,char**argv){if(argc!=2)return 2;uint32_t a[22],b[22],before[22];uint8_t bytes[196];nflstreet_option_defaults(a);for(unsigned i=0;i<22;++i)a[i]^=i*0x1234567u;if(!nflstreet_option_encode(a,bytes)||!nflstreet_option_decode(bytes,sizeof bytes,b)||memcmp(a,b,sizeof a))return 3;memcpy(before,b,sizeof b);for(unsigned i=0;i<196;++i){bytes[i]^=1;if(nflstreet_option_decode(bytes,sizeof bytes,b)||memcmp(before,b,sizeof b))return 4;bytes[i]^=1;}for(unsigned n=0;n<196;++n)if(nflstreet_option_decode(bytes,n,b)||memcmp(before,b,sizeof b))return 5;
 /* Caller supplies a new private disposable path. Do not overwrite old saves. */
 if(GetFileAttributesA(argv[1])!=INVALID_FILE_ATTRIBUTES)return 6;
 if(nflstreet_option_load(argv[1],b)!=2||memcmp(before,b,sizeof b)||!nflstreet_option_save(argv[1],a)||nflstreet_option_load(argv[1],b)!=1||memcmp(a,b,sizeof a))return 7;
 a[3]=7;a[9]=0;if(!nflstreet_option_save(argv[1],a)||nflstreet_option_load(argv[1],b)!=1||memcmp(a,b,sizeof a))return 8;
 HANDLE locked=CreateFileA(argv[1],GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);if(locked==INVALID_HANDLE_VALUE)return 9;uint32_t altered[22];memcpy(altered,a,sizeof a);altered[3]=2;if(nflstreet_option_save(argv[1],altered))return 10;CloseHandle(locked);if(nflstreet_option_load(argv[1],b)!=1||memcmp(a,b,sizeof a))return 11;
 HANDLE f=CreateFileA(argv[1],GENERIC_WRITE,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);DWORD wrote;uint8_t bad=0;if(f==INVALID_HANDLE_VALUE||!WriteFile(f,&bad,1,&wrote,NULL)||wrote!=1)return 12;CloseHandle(f);memcpy(before,b,sizeof b);if(nflstreet_option_load(argv[1],b)||memcmp(before,b,sizeof b))return 13;DeleteFileA(argv[1]);puts("Native settings store:22-word roundtrip,196 corruptions,196 truncations,missing file,replace/reload,locked-destination preservation,corrupt-file rollback;no emulator/module dependencies");return 0;}
