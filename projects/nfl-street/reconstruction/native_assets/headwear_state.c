#include "headwear_state.h"
static int field(const nflstreet_team_table *t,uint32_t tag,unsigned *out){
 unsigned found=0;for(unsigned i=0;i<t->field_count;++i){nflstreet_team_field f;
  if(!nflstreet_team_table_field(t,i,&f))return 0;
  if(f.tag==tag){if(f.type!=3||found++)return 0;*out=i;}}
 return found==1;
}
uint32_t nflstreet_headwear_map(uint32_t v){
 switch(v){case 24:return 1;case 25:return 4;case 26:return 5;case 27:return 6;
 case 28:return 7;case 34:return 11;case 35:return 12;default:return 0;}
}
int nflstreet_headwear_catalog_open(const uint8_t *bytes,size_t size,nflstreet_headwear_catalog *out){
 if(!out)return 0;nflstreet_headwear_catalog c;
 if(!nflstreet_team_table_open_tag(bytes,size,0x52414547u,&c.table)||
    !field(&c.table,0x44494547u,&c.id_field)||!field(&c.table,0x49545247u,&c.type_field)||
    !field(&c.table,0x49535247u,&c.state_field))return 0;*out=c;return 1;
}
static int gear(const nflstreet_headwear_catalog *c,uint32_t type,uint32_t id,uint32_t *out){
 unsigned found=0;uint32_t result=0;
 for(unsigned i=0;i<c->table.used;++i){uint32_t tid,iid;
  if(!nflstreet_team_table_number(&c->table,i,c->type_field,&tid)||
     !nflstreet_team_table_number(&c->table,i,c->id_field,&iid))return 0;
  if(tid==type&&iid==id){if(found++||!nflstreet_team_table_number(&c->table,i,c->state_field,&result))return 0;}}
 if(found!=1)return 0;*out=result;return 1;
}
int nflstreet_headwear_resolve(const nflstreet_headwear_catalog *catalog,const uint8_t *db,size_t size,
 uint32_t team,uint32_t player,nflstreet_headwear_selection *out){
 if(!catalog||!out)return 0;nflstreet_team_table t;unsigned f[4];
 if(!nflstreet_team_table_open_tag(db,size,0x59414c50u,&t)||
    !field(&t,0x44494754u,f)||!field(&t,0x44494750u,f+1)||
    !field(&t,0x54525350u,f+2)||!field(&t,0x54414850u,f+3))return 0;
 nflstreet_headwear_selection r={0};unsigned found=0;
 for(unsigned i=0;i<t.used;++i){uint32_t tid,pid;
  if(!nflstreet_team_table_number(&t,i,f[0],&tid)||!nflstreet_team_table_number(&t,i,f[1],&pid))return 0;
  if(tid==team&&pid==player){if(found++)return 0;r.play_row=i;
   if(!nflstreet_team_table_number(&t,i,f[2],&r.gear9_id)||
      !nflstreet_team_table_number(&t,i,f[3],&r.gear0_id))return 0;}}
 if(found!=1||!gear(catalog,9,r.gear9_id,&r.gear9_value))return 0;
 if(r.gear9_value!=11){if(!gear(catalog,0,r.gear0_id,&r.gear0_value))return 0;
  r.queried_gear0=1;r.state=nflstreet_headwear_map(r.gear0_value);}
 *out=r;return 1;
}
