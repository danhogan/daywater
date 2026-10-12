#include "team_bootstrap_weights.h"
#include <limits.h>
#include <string.h>

#define TSTR_TAG 0x52545354u
#define FIELD_STTY 0x59545453u
#define FIELD_PBPO 0x4f504250u
#define ROBP_TAG 0x524f4250u

static const uint32_t tstr_weight_tags[14]={
 0x5a425150u,0x5a425250u,0x46525750u,0x42525750u,0x4c4c4f50u,
 0x434c4f50u,0x524c4f50u,0x4c4c4450u,0x524c4450u,0x46424c50u,
 0x42424c50u,0x4c424450u,0x43424450u,0x52424450u
};
static const uint32_t curve_weight_tags[10]={
 /* Output order from the original 80264058 query, mapped back to literal
    ROBP columns by matching all 14 freshly initialized runtime rows. */
 0x57474150u,0x57455350u,0x57574150u,0x57414350u,0x57544250u,
 0x57435450u,0x57535050u,0x574c4250u,0x57564350u,0x57544450u
};

static int field_index(const nflstreet_team_table*t,uint32_t tag,uint32_t type,uint32_t width,unsigned*out){
 unsigned found=0;for(unsigned i=0;i<t->field_count;++i){nflstreet_team_field f;
  if(!nflstreet_team_table_field(t,i,&f))return 0;
  if(f.tag==tag){if(f.type!=type||(width&&f.width!=width)||++found!=1)return 0;*out=i;}}
 return found==1;
}

static int raw_float(const nflstreet_team_table*t,unsigned row,unsigned index,float*out){
 nflstreet_team_field f;if(!t||!out||row>=t->used||!nflstreet_team_table_field(t,index,&f)||
    f.type!=4||f.width!=32||f.bit_offset&7||f.bit_offset>(uint64_t)t->record_bytes*8||
    32u>(uint64_t)t->record_bytes*8-f.bit_offset)return 0;
 size_t off=t->records_at+(size_t)row*t->record_bytes+f.bit_offset/8;if(off+4>t->size)return 0;
 const uint8_t*p=t->bytes+off;uint32_t bits=(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];memcpy(out,&bits,4);return 1;
}

static int read_double(const uint8_t*ram,size_t n,uint32_t base,uint32_t address,double*out){
 if(!ram||!out||address<base||(uint64_t)address-base+8>n)return 0;size_t off=address-base;
 uint64_t bits=(uint64_t)ram[off]<<56|(uint64_t)ram[off+1]<<48|(uint64_t)ram[off+2]<<40|
  (uint64_t)ram[off+3]<<32|(uint64_t)ram[off+4]<<24|(uint64_t)ram[off+5]<<16|
  (uint64_t)ram[off+6]<<8|ram[off+7];memcpy(out,&bits,8);return 1;
}
static double uint32_to_double_bias(uint32_t value){uint64_t bits=0x4330000000000000ull|(uint64_t)value;double result;memcpy(&result,&bits,8);return result;}

static int get_number_tag(const nflstreet_team_table*t,unsigned row,uint32_t tag,uint32_t*out){
 unsigned found=0,index=0;for(unsigned i=0;i<t->field_count;++i){nflstreet_team_field f;
  if(!nflstreet_team_table_field(t,i,&f))return 0;
  if(f.tag==tag){if(f.type!=3||f.width>32||++found!=1)return 0;index=i;}}
 return found==1&&nflstreet_team_table_number(t,row,index,out);
}

static int get_row_role(const uint8_t*db,size_t size,uint32_t team,uint32_t player,uint32_t*role){
 nflstreet_team_table play;if(!nflstreet_team_table_open_tag(db,size,0x59414c50u,&play))return 0;unsigned matches=0;
 for(unsigned row=0;row<play.used;++row){uint32_t t,id;
  if(!get_number_tag(&play,row,0x44494754u,&t)||!get_number_tag(&play,row,0x44494750u,&id))return 0;
  if(t!=team||id!=player)continue;if(++matches!=1||!get_number_tag(&play,row,FIELD_PBPO,role))return 0;
 }
 return matches==1;
}

static int get_tstr_weights(const uint8_t*db,size_t size,uint32_t mode,int32_t out[14]){
 nflstreet_team_table tstr;if(!nflstreet_team_table_open_tag(db,size,TSTR_TAG,&tstr))return 0;
 unsigned matches=0;
 for(unsigned row=0;row<tstr.used;++row){uint32_t m;
  if(!get_number_tag(&tstr,row,FIELD_STTY,&m))return 0;if(m!=mode)continue;
  if(++matches!=1)return 0;
  for(unsigned i=0;i<14;++i){uint32_t v;if(!get_number_tag(&tstr,row,tstr_weight_tags[i],&v))return 0;memcpy(&out[i],&v,sizeof v);}
 }
 return matches==1;
}

int nflstreet_team_bootstrap_curve_from_template(const uint8_t*template_db,size_t template_size,
 const uint8_t*ram,size_t ram_size,uint32_t ram_base,nflstreet_team_rating_curve*out){
 if(!template_db||!ram||!out)return 0;nflstreet_team_table t;if(!nflstreet_team_table_open_tag(template_db,template_size,ROBP_TAG,&t))return 0;
 unsigned rolef,highf,lowf,weightsf[10];
 if(!field_index(&t,0x534f5050u,3,5,&rolef)||!field_index(&t,0x48445250u,3,7,&highf)||
    !field_index(&t,0x4c445250u,3,7,&lowf))return 0;
 for(unsigned i=0;i<10;++i)if(!field_index(&t,curve_weight_tags[i],4,32,&weightsf[i]))return 0;
 /* The score constants live in the original DOL image; the role records are
    runtime BSS and are reconstructed below from template/00002.bin ROBP. */
 nflstreet_team_rating_curve c;if(!nflstreet_team_rating_curve_from_ram(ram,ram_size,ram_base,&c))return 0;
 double uint_bias;if(!read_double(ram,ram_size,ram_base,0x80264150u,&uint_bias))return 0;
 float role_limit_seed;if(0x80264158u<ram_base||(uint64_t)0x80264158u-ram_base+4>ram_size)return 0;
 uint32_t seedbits=(uint32_t)ram[0x80264158u-ram_base]<<24|(uint32_t)ram[0x80264159u-ram_base]<<16|
   (uint32_t)ram[0x8026415au-ram_base]<<8|ram[0x8026415bu-ram_base];memcpy(&role_limit_seed,&seedbits,4);
 float anchors[14],multipliers[14],limits[14],coefficients[14][10];uint8_t seen[14]={0};
 for(unsigned row=0;row<t.used;++row){uint32_t role,hi,lo;if(!nflstreet_team_table_number(&t,row,rolef,&role)||role>=14||seen[role]++||
    !nflstreet_team_table_number(&t,row,highf,&hi)||!nflstreet_team_table_number(&t,row,lowf,&lo))return 0;
  anchors[role]=(float)(uint32_to_double_bias(hi)-uint_bias);multipliers[role]=(float)(uint32_to_double_bias(lo)-uint_bias);limits[role]=role_limit_seed;
  for(unsigned i=0;i<10;++i){if(!raw_float(&t,row,weightsf[i],&coefficients[role][i]))return 0;limits[role]+=coefficients[role][i];}
 }
 if(t.used!=14)return 0;for(unsigned role=0;role<14;++role)if(!seen[role])return 0;
 memcpy(c.role_anchor,anchors,sizeof anchors);memcpy(c.role_multiplier,multipliers,sizeof multipliers);
 memcpy(c.role_limit,limits,sizeof limits);memcpy(c.coefficients,coefficients,sizeof coefficients);*out=c;return 1;
}

int nflstreet_team_bootstrap_responses_from_template(const uint8_t*template_db,size_t template_size,
 const nflstreet_team_weight_inputs*inputs,const uint32_t categories[12],uint32_t status_no_row,
 nflstreet_team_gear_response out[12]){
 if(!template_db||!inputs||!categories||!out)return 0;nflstreet_team_gear_response result[12];
 for(unsigned i=0;i<12;++i)result[i].status=status_no_row;
 for(unsigned slot=0;slot<12;++slot){if(slot==7)continue;unsigned response_slot=slot;if(slot==6&&inputs->query_ids[6]==0)response_slot=7;
  nflstreet_team_gear_lookup lookup;if(!nflstreet_team_gear_lookup_from_template(template_db,template_size,
    inputs->query_ids[response_slot],categories[slot],&lookup))return 0;
  if(lookup.found){result[response_slot].status=0;memcpy(result[response_slot].values,lookup.values,sizeof lookup.values);}
 }
 memcpy(out,result,sizeof result);return 1;
}

int nflstreet_team_bootstrap_weights_build(const uint8_t*db,size_t db_size,
 const uint8_t*template_db,size_t template_size,const nflstreet_team_rating_curve*curve,
 uint32_t mode,uint32_t team_id,uint32_t player_id,
 const uint32_t categories[12],const nflstreet_team_gear_response supplied[12],
 uint32_t status_no_row,nflstreet_team_bootstrap_weight_row*out){
 if(!db||!template_db||!curve||!categories||!supplied||!out)return 0;
 nflstreet_team_weight_inputs inputs;if(!nflstreet_team_weights_from_db(db,db_size,team_id,player_id,&inputs))return 0;
 nflstreet_team_gear_response responses[12];memcpy(responses,supplied,sizeof responses);
 for(unsigned slot=0;slot<12;++slot){if(slot==7)continue;unsigned response_slot=slot,category_slot=slot;
  if(slot==6&&inputs.query_ids[6]==0)response_slot=7;
  nflstreet_team_gear_lookup lookup;if(!nflstreet_team_gear_lookup_from_template(template_db,template_size,
      inputs.query_ids[response_slot],categories[category_slot],&lookup))return 0;
  if(lookup.found){if(responses[response_slot].status==status_no_row||memcmp(responses[response_slot].values,lookup.values,sizeof lookup.values))return 0;}
  else if(responses[response_slot].status!=status_no_row){for(unsigned i=0;i<10;++i)if(responses[response_slot].values[i]!=0)return 0;}
 }
 uint32_t ratings[10];if(!nflstreet_team_weights_build(&inputs,responses,status_no_row,ratings))return 0;
 int32_t modifiers[14];if(!get_tstr_weights(template_db,template_size,mode,modifiers))return 0;
 uint32_t role;if(!get_row_role(db,db_size,team_id,player_id,&role))return 0;
 nflstreet_team_bootstrap_weight_row result={0};result.player_id=player_id;result.role=role;
 memcpy(result.adjusted_ratings,ratings,sizeof ratings);
 uint32_t reordered[10];nflstreet_team_rating_reorder(ratings,reordered);
 for(unsigned i=0;i<14;++i){uint32_t score;if(!nflstreet_team_rating_score(curve,i,reordered,0,&score))return 0;
  result.scores[i]=score;uint32_t modifier_bits=(uint32_t)modifiers[i]+100u;uint32_t product=score*modifier_bits;result.weighted[i]=product/200u;}
 *out=result;return 1;
}
