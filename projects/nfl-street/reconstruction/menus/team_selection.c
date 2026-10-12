#include "team_selection.h"
#include <string.h>
const nflstreet_team_record *nflstreet_team_catalog_find(const nflstreet_team_catalog *c,uint32_t id){
 if(!c||c->count>NFLSTREET_TEAM_CAROUSEL_MAX_IDS)return NULL;
 for(unsigned i=0;i<c->count;++i)if(c->records[i].id==id)return &c->records[i];return NULL;
}
int nflstreet_team_catalog_append(nflstreet_team_catalog *c,const nflstreet_team_table *t){
 if(!c||!t||t->used!=1||c->count>=NFLSTREET_TEAM_CAROUSEL_MAX_IDS)return 0;
 nflstreet_team_record record={0};unsigned seen=0;
 const uint32_t tags[6]={0x44494754,0x53495654,0x464f5254,0x45445254,0x564f5254,0x414e4454};
 for(unsigned i=0;i<t->field_count;++i){nflstreet_team_field f;if(!nflstreet_team_table_field(t,i,&f))return 0;
  if(f.tag==0x4c474c54u||f.tag==0x50595454u){uint32_t value;unsigned flag=f.tag==0x4c474c54u?1:2;if(record.resource_fields&flag||!nflstreet_team_table_number(t,0,i,&value))return 0;record.resource_fields|=flag;if(flag==1)record.logo_entry=value;else record.raw_type=value;}
  for(unsigned j=0;j<6;++j)if(f.tag==tags[j]){
   if(seen&(1u<<j))return 0;seen|=1u<<j;
   if(j==5){if(!nflstreet_team_table_text(t,0,i,record.name,sizeof record.name))return 0;record.name_bytes=f.width/8;}
   else{uint32_t value;if(!nflstreet_team_table_number(t,0,i,&value))return 0;
    if(j==0)record.id=value;else if(j==1)record.sivt=value;else record.fields[j-2]=value;
   }
  }
 }
 if(seen!=63||!record.name[0]||record.sivt>1||nflstreet_team_catalog_find(c,record.id))return 0;
 unsigned at=0;while(at<c->count&&c->records[at].id<record.id)++at;
 memmove(c->records+at+1,c->records+at,(c->count-at)*sizeof record);c->records[at]=record;++c->count;return 1;
}
int nflstreet_team_selection_seed(const nflstreet_team_catalog *cat,const uint32_t centers[2],nflstreet_team_carousel *out){
 if(!cat||!centers||!out||!cat->count||cat->count>NFLSTREET_TEAM_CAROUSEL_MAX_IDS)return 0;
 nflstreet_team_carousel result={0};
 for(unsigned s=0;s<2;++s){unsigned center=cat->count;
  for(unsigned i=0;i<cat->count;++i){if(i&&cat->records[i-1].id>=cat->records[i].id)return 0;
   if(cat->records[i].sivt>1)return 0;
   result.catalogs[s].entries[i]=(nflstreet_team_carousel_entry){cat->records[i].id,cat->records[i].sivt==0};
   if(cat->records[i].id==centers[s])center=i;
  }
  if(center==cat->count)return 0;result.catalogs[s].count=cat->count;
  for(unsigned i=0;i<3;++i)result.windows[s][i]=cat->records[(center+cat->count-1+i)%cat->count].id;
  result.catalogs[s].cursor=(center+1)%cat->count;
 }
 *out=result;return 1;
}
