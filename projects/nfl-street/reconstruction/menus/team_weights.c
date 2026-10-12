#include "team_weights.h"
#include <limits.h>
#include <math.h>
#include <string.h>

#define PLAY_TAG 0x59414c50u
#define FIELD_DIGT 0x44494754u
#define FIELD_DIGP 0x44494750u
#define GEAR_TAG 0x52414547u
#define GRIP_TAG 0x50495247u

static const uint32_t gear_id_tag=0x44494547u; /* GEID */
static const uint32_t gear_category_tag=0x49545247u; /* GRTI */
static const uint32_t gear_key_tag=0x594b5047u; /* GPKY */
static const uint32_t grip_value_tags[10]={
 0x4c474147u,0x52574147u,0x4b544247u,0x43544347u,0x44505347u,
 0x4b435447u,0x53535047u,0x4b4c4247u,0x564f4347u,0x43544447u
};

static const uint32_t base_tags[10]={
 0x49474150u,0x52574150u,0x4b544250u,0x48544350u,0x44505350u,
 0x4b415450u,0x53535050u,0x4b4c4250u,0x564f4350u,0x54464450u
};
static const uint32_t query_tags[12]={
 0x54414850u,0x52574550u,0x41505350u,0x52415550u,0x57424550u,0x54525750u,
 0x444e4850u,0x44485250u,0x434f5350u,0x4f485350u,0x54525350u,0x544e5050u
};

static int field_index(const nflstreet_team_table*t,uint32_t tag,unsigned*out){
 unsigned found=0;for(unsigned i=0;i<t->field_count;++i){nflstreet_team_field f;if(!nflstreet_team_table_field(t,i,&f))return 0;
  if(f.tag==tag){if(f.type!=3||++found!=1)return 0;*out=i;}}
 return found==1;
}

static int raw_field_index(const nflstreet_team_table*t,uint32_t tag,uint32_t type,
 uint32_t width,unsigned*out){
 unsigned found=0;for(unsigned i=0;i<t->field_count;++i){nflstreet_team_field f;
  if(!nflstreet_team_table_field(t,i,&f))return 0;
  if(f.tag==tag){if(f.type!=type||(width&&f.width!=width)||++found!=1)return 0;*out=i;}}
 return found==1;
}

/* Read a bounded numeric DBv8 field. Original 801b94c8 routes type 2 through
   the signed bit extractor (80213bd8), while type 3 uses the unsigned one. */
static int raw_field_number(const nflstreet_team_table*t,unsigned row,unsigned index,uint32_t*out){
 if(!t||!out||row>=t->used)return 0;nflstreet_team_field f;
 if(!nflstreet_team_table_field(t,index,&f)||(f.type!=2&&f.type!=3)||!f.width||f.width>16||
    f.bit_offset>(uint64_t)t->record_bytes*8||f.width>(uint64_t)t->record_bytes*8-f.bit_offset)return 0;
 uint64_t off=(uint64_t)row*t->record_bytes;if(off+t->record_bytes>t->size-t->records_at)return 0;
 const uint8_t*p=t->bytes+t->records_at+(size_t)off;uint32_t value=0;
 for(uint32_t bit=0;bit<f.width;++bit){uint32_t at=f.bit_offset+bit;value=(value<<1)|((p[at>>3]>>(7-(at&7)))&1u);}
 if(f.type==2&&(value&(1u<<(f.width-1u))))value|=~((1u<<f.width)-1u);
 *out=value;return 1;
}

int nflstreet_team_gear_lookup_from_template(const uint8_t*db,size_t size,
 uint32_t gear_id,uint32_t category,nflstreet_team_gear_lookup*out){
 if(!db||!out)return 0;nflstreet_team_table gear,grip;
 if(!nflstreet_team_table_open_tag(db,size,GEAR_TAG,&gear)||
    !nflstreet_team_table_open_tag(db,size,GRIP_TAG,&grip))return 0;
 unsigned idf,catf,gkeyf,rkeyf,valuef[10];
 if(!raw_field_index(&gear,gear_id_tag,3,6,&idf)||!raw_field_index(&gear,gear_category_tag,3,4,&catf)||
    !raw_field_index(&gear,gear_key_tag,3,9,&gkeyf)||!raw_field_index(&grip,gear_key_tag,3,9,&rkeyf))return 0;
 for(unsigned i=0;i<10;++i)if(!raw_field_index(&grip,grip_value_tags[i],2,9,&valuef[i]))return 0;
 nflstreet_team_gear_lookup result={0};unsigned matches=0;uint32_t key=0;unsigned gearrow=0;
 for(unsigned row=0;row<gear.used;++row){uint32_t id,cat,k;
  if(!raw_field_number(&gear,row,idf,&id)||!raw_field_number(&gear,row,catf,&cat)||!raw_field_number(&gear,row,gkeyf,&k))return 0;
  if(id==gear_id&&cat==category){if(++matches!=1)return 0;key=k;gearrow=row;}}
 if(matches==0){*out=result;return 1;}
 unsigned grip_matches=0,griprow=0;uint32_t values[10];
 for(unsigned row=0;row<grip.used;++row){uint32_t k;if(!raw_field_number(&grip,row,rkeyf,&k))return 0;
  if(k!=key)continue;if(++grip_matches!=1)return 0;griprow=row;
  for(unsigned i=0;i<10;++i)if(!raw_field_number(&grip,row,valuef[i],&values[i]))return 0;}
 if(grip_matches!=1){*out=result;return 1;}
 result.found=1;result.gear_row=gearrow;result.grip_row=griprow;result.gear_key=key;
 for(unsigned i=0;i<10;++i)result.values[i]=(int16_t)(uint16_t)values[i];
 *out=result;return 1;
}

int nflstreet_team_weights_from_db(const uint8_t*db,size_t size,uint32_t team_id,
 uint32_t player_id,nflstreet_team_weight_inputs*out){
 if(!db||!out)return 0;nflstreet_team_table play;if(!nflstreet_team_table_open_tag(db,size,PLAY_TAG,&play))return 0;
 uint32_t tags[24];tags[0]=FIELD_DIGT;tags[1]=FIELD_DIGP;memcpy(tags+2,base_tags,sizeof base_tags);memcpy(tags+12,query_tags,sizeof query_tags);
 unsigned fields[24];for(unsigned i=0;i<24;++i)if(!field_index(&play,tags[i],&fields[i]))return 0;
 unsigned match=0;uint32_t values[24];
 for(unsigned row=0;row<play.used;++row){uint32_t team=0,id=0;
  if(!nflstreet_team_table_number(&play,row,fields[0],&team)||!nflstreet_team_table_number(&play,row,fields[1],&id))return 0;
  if(team!=team_id||id!=player_id)continue;
  if(++match!=1)return 0;
  for(unsigned i=0;i<24;++i)if(!nflstreet_team_table_number(&play,row,fields[i],&values[i]))return 0;
 }
 if(match!=1)return 0;nflstreet_team_weight_inputs result={0};result.team_id=values[0];result.player_id=values[1];
 memcpy(result.base_values,values+2,sizeof result.base_values);memcpy(result.query_ids,values+12,sizeof result.query_ids);
 *out=result;return 1;
}

int nflstreet_team_weights_build(const nflstreet_team_weight_inputs*in,
 const nflstreet_team_gear_response responses[12],uint32_t status_17,uint32_t ratings[10]){
 if(!in||!responses||!ratings)return 0;int16_t sums[10]={0};
 for(unsigned slot=0;slot<12;++slot){if(slot==7)continue;unsigned response_slot=slot;if(slot==6&&in->query_ids[6]==0)response_slot=7;if(responses[response_slot].status==status_17)continue;
  for(unsigned j=0;j<10;++j){uint16_t bits;memcpy(&bits,&sums[j],sizeof bits);bits=(uint16_t)(bits+(uint16_t)responses[response_slot].values[j]);memcpy(&sums[j],&bits,sizeof bits);}
 }
 uint32_t result[10];for(unsigned j=0;j<10;++j){uint32_t bits=in->base_values[j]+(uint32_t)(int32_t)sums[j];int32_t value;memcpy(&value,&bits,4);if(value<1)value=1;else if(value>99)value=99;result[j]=(uint32_t)value;}
 memcpy(ratings,result,sizeof result);return 1;
}

static int read32(const uint8_t*ram,size_t n,uint32_t base,uint32_t address,uint32_t*out){
 if(address<base)return 0;uint64_t off=(uint64_t)address-base;if(off+4>n)return 0;*out=(uint32_t)ram[off]<<24|(uint32_t)ram[off+1]<<16|(uint32_t)ram[off+2]<<8|ram[off+3];return 1;
}
static int read_float(const uint8_t*ram,size_t n,uint32_t base,uint32_t address,float*out){uint32_t word;if(!read32(ram,n,base,address,&word))return 0;memcpy(out,&word,4);return 1;}

int nflstreet_team_rating_curve_from_ram(const uint8_t*ram,size_t n,uint32_t base,
 nflstreet_team_rating_curve*out){
 if(!ram||!out)return 0;nflstreet_team_rating_curve c={0};uint32_t high,low;
 if(!read_float(ram,n,base,0x80264350u,&c.floor_value))return 0;
 c.weight_threshold=c.floor_value;
 if(!read32(ram,n,base,0x80264358u,&high)||!read32(ram,n,base,0x8026435cu,&low))return 0;
 uint64_t bias_bits=(uint64_t)high<<32|low;memcpy(&c.integer_bias,&bias_bits,8);
 if(!read_float(ram,n,base,0x80264360u,&c.scale)||
    !read_float(ram,n,base,0x80264364u,&c.offset)||!read_float(ram,n,base,0x80264368u,&c.clamp_low)||
    !read_float(ram,n,base,0x8026436cu,&c.clamp_high))return 0;
 if(!read32(ram,n,base,0x80264370u,&high)||!read32(ram,n,base,0x80264374u,&low))return 0;
 uint64_t conversion_bits=(uint64_t)high<<32|low;memcpy(&c.conversion_base,&conversion_bits,8);
 for(unsigned role=0;role<14;++role){uint32_t at=0x8034f604u+role*0x34u;
  if(!read_float(ram,n,base,at,&c.role_anchor[role])||!read_float(ram,n,base,at+4,&c.role_multiplier[role])||
     !read_float(ram,n,base,at+0x30,&c.role_limit[role]))return 0;
  for(unsigned j=0;j<10;++j)if(!read_float(ram,n,base,at+8+j*4,&c.coefficients[role][j]))return 0;
 }
 *out=c;return 1;
}

int nflstreet_team_rating_score(const nflstreet_team_rating_curve*c,uint32_t role,
 const uint32_t values[10],uint32_t clamp_mode,uint32_t*out){
 if(!c||!values||!out||role>=14)return 0;float sum=c->floor_value;
 for(unsigned i=0;i<10;++i){float weight=c->coefficients[role][i];if(c->weight_threshold<weight){uint32_t encoded=values[i]^0x80000000u;uint64_t bits=0x4330000000000000ull|encoded;double exact;memcpy(&exact,&bits,8);float converted=(float)(exact-c->integer_bias);sum=fmaf(converted,weight,sum);}}
 float anchor=c->role_anchor[role],mult=c->role_multiplier[role],limit=c->role_limit[role];
 float product=(float)((double)mult*(double)limit);float numerator=(float)((double)sum-(double)product);
 float delta=(float)((double)anchor-(double)mult);float denom=(float)((double)delta*(double)limit);denom=(float)(denom*c->scale);
 float score=(float)(numerator/denom);score=(float)(score+c->offset);
 if(score<(float)c->floor_value)score=(float)c->floor_value;
 if(clamp_mode==1){if(score<c->clamp_low)score=c->clamp_low;else if(c->clamp_high<score)score=c->clamp_high;}
 if(!isfinite(score))return 0;
 if((double)score>=c->conversion_base){double delta_score=(double)score-c->conversion_base;if(delta_score>(double)INT32_MAX||delta_score<(double)INT32_MIN)return 0;*out=(uint32_t)(int32_t)delta_score^0x80000000u;}
 else if(score<0.0f||(double)score>(double)UINT32_MAX)return 0;else *out=(uint32_t)score;
 return 1;
}

void nflstreet_team_rating_reorder(const uint32_t v[10],uint32_t out[10]){
 static const unsigned index[10]={0,4,1,3,2,5,6,7,8,9};if(!v||!out)return;uint32_t reordered[10];for(unsigned i=0;i<10;++i)reordered[i]=v[index[i]];memcpy(out,reordered,sizeof reordered);
}
