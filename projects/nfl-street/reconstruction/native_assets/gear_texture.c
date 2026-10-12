#include "gear_texture.h"
#include "headwear_state.h"
#include <stdio.h>
#include <string.h>
#define WINDOW_START 0x802714f0u
#define WINDOW_SIZE 0x868u
#define CATALOG_SIZE (16u+WINDOW_SIZE+48u+104u+32u+100u)
#define DEFAULT_NAMES 0x802714f0u
#define SELECTORS 0x80271668u
#define SUFFIXES 0x802716ccu
#define INITIAL_COLORS 0x802717ceu
#define CHANNEL_LISTS 0x80271826u
#define LIST_BY_GRSI 0x80271bc8u
#define COLOR_BY_GRSI 0x80271c58u
#define NO_SELECTOR 27u
static uint32_t word(const uint8_t *p){return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static uint16_t half(const uint8_t *p){return(uint16_t)((unsigned)p[0]<<8|p[1]);}
static void put(uint8_t *p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
static void put16(uint8_t *p,uint32_t v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static const uint8_t *at(const nflstreet_gear_texture_catalog *c,uint32_t address,size_t n){
 if(address<WINDOW_START||address-WINDOW_START>WINDOW_SIZE||n>WINDOW_SIZE-(address-WINDOW_START))return NULL;
 return c->window+(address-WINDOW_START);
}
static const char *default_name(const nflstreet_gear_texture_catalog *c,unsigned slot){return(const char*)at(c,DEFAULT_NAMES+15u*slot,15);}
static const char *suffix(const nflstreet_gear_texture_catalog *c,unsigned selector){return(const char*)at(c,SUFFIXES+15u*selector,15);}
static uint32_t selector(const nflstreet_gear_texture_catalog *c,unsigned slot){return word(at(c,SELECTORS+4u*slot,4));}
int nflstreet_gear_texture_catalog_open(const uint8_t *b,size_t n,nflstreet_gear_texture_catalog *out){
 if(!b||!out||n!=CATALOG_SIZE||memcmp(b,"GTC3",4)||word(b+4)!=3||word(b+8)!=WINDOW_START||word(b+12)!=WINDOW_SIZE)return 0;
 nflstreet_gear_texture_catalog c={b+16,b+16+WINDOW_SIZE,b+16+WINDOW_SIZE+48,(const char*)b+16+WINDOW_SIZE+152,b+16+WINDOW_SIZE+184};
 if(!memchr(c.excluded_dynamic_name,0,32))return 0;
 for(unsigned i=0;i<NFLSTREET_GEAR_TEXTURE_SLOTS;++i){uint32_t v=selector(&c,i);if((v!=NO_SELECTOR&&v>=16)||!memchr(default_name(&c,i),0,15))return 0;}
 for(unsigned i=0;i<16;++i)if(!memchr(suffix(&c,i),0,15))return 0;
 for(uint32_t a=INITIAL_COLORS;;a+=4){const uint8_t *p=at(&c,a,6);if(!p)return 0;if(half(p+4)==0xffffu)break;}
 *out=c;return 1;
}
uint32_t nflstreet_gear_texture_template(const nflstreet_gear_texture_catalog *c,unsigned slot){
 return slot<NFLSTREET_GEAR_TEXTURE_SLOTS?word(c->templates+4*slot):UINT32_MAX;
}
static int unique_field(const nflstreet_team_table *t,uint32_t tag,unsigned *index,unsigned *found){
 *found=0;for(unsigned i=0;i<t->field_count;++i){nflstreet_team_field f;if(!nflstreet_team_table_field(t,i,&f))return 0;
  if(f.tag==tag){if((*found)++)return 0;*index=i;}}
 return 1;
}
static void truncate_at_space(uint8_t *text){for(unsigned i=0;i<32&&text[i];++i)if(text[i]==' '){text[i]=0;return;}}
int nflstreet_gear_record_from_table(const nflstreet_team_table *t,unsigned row,const nflstreet_gear_texture_catalog *c,uint8_t out[NFLSTREET_GEAR_RECORD_BYTES]){
 if(!t||!c||!out||row>=t->used)return 0;uint8_t r[NFLSTREET_GEAR_RECORD_BYTES];memcpy(r,out,sizeof r);
 for(unsigned j=0;j<26;++j){unsigned index=0,found;nflstreet_team_field f;
  if(!unique_field(t,word(c->tags+4*j),&index,&found)||found!=1||!nflstreet_team_table_field(t,index,&f))return 0;
  if(j==18||j==19){char text[32];unsigned to=j==18?12u:45u;
   if(f.width!=248||!nflstreet_team_table_text(t,row,index,text,sizeof text))return 0;
   memcpy(r+to,text,31);r[to+31]=0;continue;}
  uint32_t value;if(!nflstreet_team_table_number(t,row,index,&value))return 0;
  if(j<16)r[0x4e + j]=(uint8_t)value;
  else if(j==16)put(r,value);else if(j==17)put(r+8,value);else if(j==20)put(r+4,value);
  else if(j==21)put(r+0x60,value);else if(j==22)put(r+0x64,value);
  else if(j==24)r[0x6c]=(uint8_t)(value!=0);else if(j==25)r[0x6d]=(uint8_t)(value!=0);
 }
 put(r+0x68,0);truncate_at_space(r+0x2d);truncate_at_space(r+12);memcpy(out,r,sizeof r);return 1;
}
int nflstreet_gear_records_build(const nflstreet_team_table *t,const nflstreet_gear_texture_catalog *c,nflstreet_gear_records *out){
 if(!t||!c||!out||t->used>NFLSTREET_GEAR_RECORDS_MAX)return 0;
 for(unsigned row=0;row<t->used;++row){memset(out->records[row],0,NFLSTREET_GEAR_RECORD_BYTES);
  if(!nflstreet_gear_record_from_table(t,row,c,out->records[row]))return 0;}
 out->count=t->used;return 1;
}
int nflstreet_gear_records_find(const nflstreet_gear_records *g,uint32_t category,uint32_t id,unsigned *row){
 if(!g||!row||g->count>NFLSTREET_GEAR_RECORDS_MAX)return 0;unsigned found=0,which=0;
 for(unsigned i=0;i<g->count;++i)if(word(g->records[i]+4)==category&&word(g->records[i]+8)==id){++found;which=i;}
 if(found!=1)return 0;*row=which;return 1;
}
static int play_number(const nflstreet_team_table *t,unsigned row,uint32_t tag,uint32_t *value){
 unsigned index=0,found;if(!unique_field(t,tag,&index,&found))return 0;
 if(!found){*value=0;return 1;}return nflstreet_team_table_number(t,row,index,value);
}
static int gear_state(const nflstreet_gear_records *g,uint32_t category,uint32_t id,uint32_t *grsi){
 unsigned row;if(!nflstreet_gear_records_find(g,category,id,&row))return 0;*grsi=word(g->records[row]+0x60);return 1;
}
static uint8_t gear1_state(uint32_t grsi){
 switch(grsi){case 0x1d:return 1;case 0x1e:return 2;case 0x1f:return 3;case 0x20:return 4;case 0x21:return 5;default:return 0;}
}
int nflstreet_gear_player_record_from_play(const nflstreet_team_table *t,unsigned row,const nflstreet_gear_records *g,uint8_t out[NFLSTREET_GEAR_PLAYER_RECORD_BYTES]){
 static const uint32_t byte_tags[11]={0x4c554c50u,0x4c574c50u,0x524f4850u,0x50544550u,0x54415250u,0x4f544150u,
  0x544c5250u,0x4f544c50u,0x44465450u,0x44425450u,0x43444850u};
 static const uint32_t query_tags[12]={0x54414850u,0x52574550u,0x41505350u,0x52415550u,0x57424550u,0x54525750u,
  0x444e4850u,0x44485250u,0x434f5350u,0x4f485350u,0x54525350u,0x544e5050u};
 if(!t||!g||!out||row>=t->used)return 0;uint8_t r[NFLSTREET_GEAR_PLAYER_RECORD_BYTES];memcpy(r,out,sizeof r);uint32_t v;
 for(unsigned i=0;i<11;++i){if(!play_number(t,row,byte_tags[i],&v))return 0;r[i]=(uint8_t)v;}
 for(unsigned i=0;i<12;++i){if(!play_number(t,row,query_tags[i],&v))return 0;put(r+0x0c+4*i,v);}
 if(!play_number(t,row,0x494b5350u,&v))return 0;put16(r+0x40,v+1u);
 if(!play_number(t,row,0x54484c50u,&v))return 0;put16(r+0x42,v);
 if(!play_number(t,row,0x52414850u,&v))return 0;put16(r+0x44,v==0x3fu?0xcu:v);
 if(!play_number(t,row,0x4e454a50u,&v))return 0;r[0x4a]=(uint8_t)v;
 if(!play_number(t,row,0x54425950u,&v))return 0;r[0x4b]=(uint8_t)v;
 if(!play_number(t,row,0x58454c50u,&v))return 0;r[0x49]=(uint8_t)v;
 uint32_t grsi,state=0;if(!gear_state(g,9,word(r+0x34),&grsi))return 0;
 if(grsi!=11){if(!gear_state(g,0,word(r+0x0c),&grsi))return 0;state=nflstreet_headwear_map(grsi);}
 put16(r+0x46,state);if(!gear_state(g,1,word(r+0x10),&grsi))return 0;r[0x48]=gear1_state(grsi);
 memcpy(out,r,sizeof r);return 1;
}
static int same_ascii(const char *a,const char *b){for(;;){unsigned x=(unsigned char)*a++,y=(unsigned char)*b++;
 if(x>='A'&&x<='Z')x+=32;if(y>='A'&&y<='Z')y+=32;if(x!=y)return 0;if(!x)return 1;}}
/* 800E2FD4/800D9CA4: truncate the key at its first space, then HSH1 lookup. */
static int32_t lookup(const nflstreet_archive *a,char *key){char *space=strchr(key,' ');if(space)*space=0;unsigned entry;return nflstreet_archive_find(a,key,&entry)?(int32_t)entry:-1;}
int nflstreet_gear_texture_select(const nflstreet_gear_texture_catalog *c,const nflstreet_archive *textures,
 const nflstreet_archive *dynamic,const uint8_t gear[NFLSTREET_GEAR_RECORD_BYTES],const uint8_t player[NFLSTREET_GEAR_PLAYER_RECORD_BYTES],
 unsigned logical,int32_t indices[NFLSTREET_GEAR_TEXTURE_SLOTS],uint32_t palettes[NFLSTREET_GEAR_TEXTURE_SLOTS]){
 if(!c||!textures||!dynamic||!gear||!player||logical>=12||!indices||!palettes||!memchr(gear+45,0,33))return 0;
 const char *name=(const char*)gear+45;char key[64];uint32_t palette=UINT32_MAX;
 if(word(gear+0x64)&&!same_ascii(name,c->excluded_dynamic_name)){
  if(snprintf(key,47,"PLADYNCLUT_%s",name)<0)return 0;palette=(uint32_t)lookup(dynamic,key);}
 if(word(c->categories+4*logical)==6){
  if(gear[0x58]){if(snprintf(key,61,"PLATEX_%s_%02d_%s",name,half(player+0x40),suffix(c,10))<0)return 0;
   unsigned slot=logical==6?9:10;indices[slot]=lookup(textures,key);palettes[slot]=palette;}
  return 1;
 }
 for(unsigned slot=0;slot<NFLSTREET_GEAR_TEXTURE_SLOTS;++slot){uint32_t s=selector(c,slot);if(s==NO_SELECTOR||!gear[0x4e + s])continue;
  const char *part=suffix(c,s);int32_t index=-1;
  if(!player[100]){
   if(!player[0x4b]){if(snprintf(key,61,"PLATEX_%s_HEAVY_AWAY_%s",name,part)<0)return 0;index=lookup(textures,key);}
   if(index==-1){if(snprintf(key,61,"PLATEX_%s_AWAY_%s",name,part)<0)return 0;index=lookup(textures,key);}
  }
  if(index==-1){
   if(!player[0x4b]){if(snprintf(key,61,"PLATEX_%s_HEAVY_%s",name,part)<0)return 0;index=lookup(textures,key);}
   if(index==-1){if(snprintf(key,61,"PLATEX_%s_%s",name,part)<0)return 0;index=lookup(textures,key);}
  }
  indices[slot]=index;palettes[slot]=palette;
 }
 return 1;
}
static int color(nflstreet_gear_texture_plan *p,uint32_t channel,uint32_t value){
 if(p->color_count>=NFLSTREET_GEAR_TEXTURE_COLORS_MAX)return 0;
 p->colors[p->color_count++]=(nflstreet_gear_color_request){(uint16_t)channel,(uint16_t)value};return 1;
}
/* 800DDA9C: halfword channel/value pairs until channel FFFF. */
static int channel_list(const nflstreet_gear_texture_catalog *c,uint32_t list,nflstreet_gear_texture_plan *p){
 if(list>WINDOW_SIZE/32u)return 0;
 for(uint32_t a=CHANNEL_LISTS+32u*list;;a+=4){const uint8_t *e=at(c,a,4);if(!e)return 0;uint32_t v=word(e);
  if(v>>16==0xffffu)return 1;if(!color(p,v>>16,v&0xffffu))return 0;}
}
/* 800DDB10 */
static int gear_colors(const nflstreet_gear_texture_catalog *c,const uint8_t *gear,const uint8_t *player,nflstreet_gear_texture_plan *p){
 uint32_t grsi=word(gear+0x60);
 if(grsi==0x3f)return 1;
 if(grsi==0xd)return(!(player[0]&1)||channel_list(c,0xd,p))&&(!(player[0]&2)||channel_list(c,0xe,p));
 if(grsi==0x10)return(!(player[1]&1)||channel_list(c,0x11,p))&&(!(player[1]&2)||channel_list(c,0x12,p));
 const uint8_t *e=grsi<64?at(c,LIST_BY_GRSI+4u*grsi,4):NULL;if(!e)return 0;
 uint32_t list=word(e);return list==0xffu||channel_list(c,list,p);
}
int nflstreet_gear_texture_kind(uint32_t kind,uint32_t packed,uint32_t a4,uint32_t a5,uint32_t a6,uint32_t a7,
 int32_t *index,uint32_t *palette,uint32_t *color_value){
 uint32_t i,pal,shift;
 switch(kind){
  case 1:i=a4+0x44au;pal=0x7eu;shift=8;break;
  case 2:i=a5+(a7?0x4d1u:0x4aeu);pal=UINT32_MAX;shift=24;break;
  case 3:i=a5+0x4f4u;pal=0x7cu;shift=16;break;
  case 4:i=a5+0x4f4u;pal=0x7cu;shift=24;break;
  case 5:i=a5+0x517u;pal=a6?UINT32_MAX:0x80u;shift=16;break;
  case 6:i=0x579u;pal=UINT32_MAX;shift=16;break;
  case 7:i=0x57au;pal=UINT32_MAX;shift=16;break;
  default:return 0;
 }
 *index=(int32_t)i;*palette=pal;*color_value=packed>>shift&0xffu;return 1;
}
static int kind_texture(uint32_t kind,uint32_t channel,uint32_t packed,uint32_t a4,uint32_t a5,uint32_t a6,uint32_t a7,
 int32_t *index,uint32_t *palette,nflstreet_gear_texture_plan *p){
 uint32_t value;return !nflstreet_gear_texture_kind(kind,packed,a4,a5,a6,a7,index,palette,&value)||color(p,channel,value);
}
void nflstreet_gear_texture_fallback(uint32_t code,int32_t *index,uint32_t *palette){
 static const uint16_t indices[28]={0x215,0x216,0x217,0x218,0x219,0x218,0x21a,0x21a,0x21a,0x21e,0x21f,0x220,0x21b,0x21c,
  0x21d,0x21e,0x221,0x222,0x223,0x224,0x225,0x21f,0x226,0x227,0x228,0x229,0x22a,0x22b};
 *index=code<28?indices[code]:0x21b;*palette=0x7au;
}
int nflstreet_gear_texture_plan_build(const nflstreet_gear_texture_catalog *c,const nflstreet_gear_records *g,
 const nflstreet_archive *textures,const nflstreet_archive *dynamic,const uint8_t player[NFLSTREET_GEAR_PLAYER_RECORD_BYTES],
 nflstreet_gear_texture_plan *out){
 if(!c||!g||!textures||!dynamic||!player||!out)return 0;
 nflstreet_gear_texture_plan p;memset(&p,0,sizeof p);
 for(unsigned i=0;i<NFLSTREET_GEAR_TEXTURE_SLOTS;++i){p.indices[i]=-1;p.palettes[i]=UINT32_MAX;}
 p.indices[19]=0x623;p.indices[20]=0x57b;p.indices[21]=0x624;p.indices[22]=0x57c;
 if(half(at(c,INITIAL_COLORS,2))!=0xffffu)
  for(uint32_t a=INITIAL_COLORS;;a+=4){const uint8_t *e=at(c,a,6);if(!color(&p,half(e),half(e+2)))return 0;if(half(e+4)==0xffffu)break;}
 uint32_t a4=player[0x4a],a5=(uint32_t)half(player+0x3e)-1u,a6=player[0x3c],a7=player[0x64];int flag=0;
 for(unsigned logical=0;logical<12;++logical){unsigned row;
  if(!nflstreet_gear_records_find(g,word(c->categories+4*logical),word(player+0x0c+4*logical),&row))return 0;
  const uint8_t *gear=g->records[row];p.gear_rows[logical]=row;
  if(!nflstreet_gear_texture_select(c,textures,dynamic,gear,player,logical,p.indices,p.palettes)||!gear_colors(c,gear,player,&p))return 0;
  if(logical==10){uint32_t grsi=word(gear+0x60);if(grsi==0xb)flag=1;
   const uint8_t *e=grsi<64?at(c,COLOR_BY_GRSI+4u*grsi,4):NULL;if(!e)return 0;uint32_t packed=word(e);
   if(!kind_texture(gear[0x6d]?player[8]:0,0x13,packed,a4,a5,a6,a7,p.indices+16,p.palettes+16,&p)||
      !kind_texture(gear[0x6c]?player[9]:0,0x14,packed,a4,a5,a6,a7,p.indices+17,p.palettes+17,&p))return 0;
  }else if(logical==0&&!kind_texture(player[0x0a],0x15,0xffu,a4,a5,a6,a7,p.indices+18,p.palettes+18,&p))return 0;
 }
 for(unsigned slot=0;slot<NFLSTREET_GEAR_TEXTURE_SLOTS;++slot){const char *name=default_name(c,slot);char key[48];
  if(p.indices[slot]!=-1||!name[0])continue;
  if(snprintf(key,39,"PLATEX_%s_%s%02d",name,name,half(player+0x40))<0)return 0;p.indices[slot]=lookup(textures,key);
  if(slot==7){p.indices[22]=(int32_t)(player[4]*2u+0x57cu);p.indices[21]=(int32_t)(player[5]*2u+0x624u);}
  else if(slot==4){p.indices[20]=(int32_t)(player[4]*2u+0x57bu);p.indices[19]=(int32_t)(player[5]*2u+0x623u);}
 }
 if(p.indices[0]==-1){p.fallback=1;nflstreet_gear_texture_fallback(half(player+0x44),&p.fallback_index,&p.fallback_palette);}
 if(flag&&(!color(&p,0,0xff)||!color(&p,0x17,0xff)||!color(&p,0x15,0xff)))return 0;
 p.record_65=((uint32_t)half(player+0x46)-0xbu<=1u||flag)?0:1;
 *out=p;return 1;
}
