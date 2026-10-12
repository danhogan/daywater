#include "team_format.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint32_t be(const uint8_t*p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static uint8_t*ptr(uint8_t*b,size_t n,uint32_t base,uint32_t a,size_t z){return a>=base&&(size_t)(a-base)<=n&&z<=n-(a-base)?b+a-base:NULL;}
static const char*text(uint8_t*b,size_t n,uint32_t base,uint32_t d){uint8_t*p=ptr(b,n,base,d,12);if(!p)return NULL;uint8_t*s=ptr(b,n,base,be(p+8),1);return s&&memchr(s,0,n-(size_t)(s-b))?(const char*)s:NULL;}
int nflstreet_team_format(uint8_t*b,size_t n,uint32_t base,const nflstreet_ui_service_request*q){
 if(!b||!q||q->kind!=10||q->argc>16)return 0;if(!q->target||!q->property)return 1;
 uint8_t*d=ptr(b,n,base,q->target,12);const char*f=text(b,n,base,q->property);if(!d||!f)return 0;
 uint32_t cap=be(d+4);if(!cap)return 1;if(cap>4096)return 0;uint8_t*dest=ptr(b,n,base,be(d+8),(size_t)cap+1);if(!dest)return 0;
 char*next=malloc((size_t)cap+1);if(!next)return 0;unsigned written=0,arg=0;int ok=1;
 for(size_t i=0;f[i]&&ok;++i){if(f[i]!='%'){if(written<cap)next[written++]=f[i];continue;}
  ++i;if(f[i]=='%'){if(written<cap)next[written++]='%';continue;}if(arg>=q->argc){ok=0;break;}
  char digits[32];const char*s=NULL;
  if(f[i]=='d'||f[i]=='i'){snprintf(digits,sizeof digits,"%d",(int32_t)q->args[arg]);s=digits;}
  else if(f[i]=='u'){snprintf(digits,sizeof digits,"%u",q->args[arg]);s=digits;}
  else if(f[i]=='s')s=q->args[arg]?text(b,n,base,q->args[arg]):"(null)";
  if(!s){ok=0;break;}++arg;for(size_t j=0;s[j];++j)if(written<cap)next[written++]=s[j];
 }
 if(ok){next[written]=0;memmove(dest,next,written+1);}free(next);return ok;
}
