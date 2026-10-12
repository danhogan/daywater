#include "team_identity.h"
#include "team_catalog.h"
#include <string.h>
int nflstreet_team_identity_name(const char*a,const char*b,uint32_t limit,char out[27]){
 if(!a||!b||!out||limit>26)return 0;const char*ae=memchr(a,0,15),*be=memchr(b,0,13);if(!ae||!be)return 0;
 size_t an=(size_t)(ae-a),bn=(size_t)(be-b);if(an>14||bn>12)return 0;if(bn>limit)bn=limit;
 char result[27]={0};unsigned count=0;
 for(unsigned which=0;which<2;++which){const unsigned char*s=(const unsigned char*)(which?b:a);size_t length=which?bn:an;
  for(size_t i=0;i<length;++i){unsigned c=s[i];if(c>127)return 0;if(c>='a'&&c<='z')c-=32;if(c>='A'&&c<='Z')result[count++]=(char)c;}
 }
 memcpy(out,result,sizeof result);return 1;
}
int nflstreet_team_identity_from_db(const uint8_t*b,size_t n,uint32_t tid,uint32_t pid,nflstreet_team_identity*out){
 if(!out)return 0;nflstreet_team_table t;if(!nflstreet_team_table_open_tag(b,n,0x59414c50u,&t))return 0;
 const uint32_t tags[4]={0x44494754u,0x44494750u,0x414e4c50u,0x414e4650u};unsigned indices[4]={0},seen=0;
 for(unsigned i=0;i<t.field_count;++i){nflstreet_team_field f;if(!nflstreet_team_table_field(&t,i,&f))return 0;
  for(unsigned j=0;j<4;++j)if(f.tag==tags[j]){if(seen&(1u<<j))return 0;seen|=1u<<j;indices[j]=i;}
 }
 if(seen!=15)return 0;unsigned matches=0;nflstreet_team_identity result={0};
 for(unsigned r=0;r<t.used;++r){uint32_t row_tid,row_pid;if(!nflstreet_team_table_number(&t,r,indices[0],&row_tid)||!nflstreet_team_table_number(&t,r,indices[1],&row_pid))return 0;
  if(row_tid!=tid||row_pid!=pid)continue;if(++matches!=1)return 0;result.team_id=tid;result.player_id=pid;
  if(!nflstreet_team_table_text(&t,r,indices[2],result.plna,sizeof result.plna)||!nflstreet_team_table_text(&t,r,indices[3],result.pfna,sizeof result.pfna)||!nflstreet_team_identity_name(result.plna,result.pfna,26,result.model_name))return 0;
 }
 if(matches!=1)return 0;*out=result;return 1;
}
