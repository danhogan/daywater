/* Test-only inclusion permits structural hostile-input testing behind the
   pinned public API, without exporting a revision-bypass API to consumers. */
#include "disc_slot_materials.c"
#include <stdio.h>
#define REQUIRE(x) do{if(!(x)){fprintf(stderr,"comparison failed at line %d\n",__LINE__);return 4;}}while(0)
static uint8_t *read_file(const char *path,size_t *n){FILE *f=fopen(path,"rb");if(!f)return NULL;
 if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}long len=ftell(f);if(len<0){fclose(f);return NULL;}rewind(f);
 uint8_t *b=malloc((size_t)len);if(!b){fclose(f);return NULL;}*n=fread(b,1,(size_t)len,f);fclose(f);return b;}
static uint32_t random_word(uint32_t *s){*s=*s*1664525u+1013904223u;return *s;}
static int consumers(const nflstreet_slot_material_catalog *a,const nflstreet_slot_material_catalog *b,uint32_t seed){
 nflstreet_slot_material slots[2][25]={0};uint8_t objects[2][25][40],textures[2][25][3*44],entries[2][25][16];
 REQUIRE(a->setting_count==b->setting_count&&a->descriptor_count==b->descriptor_count);
 for(unsigned k=0;k<2;++k)REQUIRE(a->shader_stages[k]==b->shader_stages[k]);
 for(unsigned s=0;s<25;++s){unsigned count=random_word(&seed)%4;uint32_t token=0x100000u+(random_word(&seed)&0xffffu)*256u;
  uint8_t obj[40],tex[3*44],ent[16];for(unsigned i=0;i<40;++i)obj[i]=(uint8_t)random_word(&seed);
  for(unsigned i=0;i<sizeof tex;++i)tex[i]=(uint8_t)random_word(&seed);
  memset(ent,0,sizeof ent);ent[9]=(uint8_t)(random_word(&seed)%3);ent[11]=(uint8_t)(random_word(&seed)%3);ent[13]=(uint8_t)(random_word(&seed)%2);
  sm_write(obj+16,count);for(unsigned i=0;i<3;++i){sm_write(tex+44*i+20,random_word(&seed)%11);tex[44*i+32]=0;tex[44*i+33]=(uint8_t)(1+random_word(&seed)%8);}
  REQUIRE(!strcmp(nflstreet_slot_material_name(a,s),nflstreet_slot_material_name(b,s)));
  for(unsigned k=0;k<2;++k){memcpy(objects[k][s],obj,40);memcpy(textures[k][s],tex,sizeof tex);memcpy(entries[k][s],ent,16);
   nflstreet_slot_material *m=&slots[k][s];m->material=nflstreet_slot_material_name(k?b:a,s);m->entries=count?token:0;
   m->texture_set.object.bytes=objects[k][s];m->texture_set.textures.bytes=textures[k][s];m->texture_set.textures.size=sizeof tex;
   m->texture_set.textures.token=token;m->texture_set.palettes.token=token+0x100;m->texture_set.entries.bytes=entries[k][s];}}
 REQUIRE(nflstreet_slot_materials_configure(a,slots[0])==1);REQUIRE(nflstreet_slot_materials_configure(b,slots[1])==1);
 REQUIRE(!memcmp(objects[0],objects[1],sizeof objects[0]));REQUIRE(!memcmp(textures[0],textures[1],sizeof textures[0]));
 nflstreet_actor_material actors[2][64];unsigned counts[2]={0};
 REQUIRE(nflstreet_actor_materials_hook(a,slots[0],actors[0],&counts[0])==1);
 REQUIRE(nflstreet_actor_materials_hook(b,slots[1],actors[1],&counts[1])==1);REQUIRE(counts[0]==counts[1]);
 for(unsigned i=0;i<counts[0];++i){nflstreet_actor_material *x=&actors[0][i],*y=&actors[1][i];
  REQUIRE(!strcmp(x->material,y->material)&&!strcmp(x->texture,y->texture));
  REQUIRE(x->shader==y->shader&&x->stage_count==y->stage_count&&!memcmp(x->stages,y->stages,sizeof x->stages));}
 return 0;
}
static uint8_t *source_at(uint8_t *dol,size_t size,uint32_t address,size_t n){sm_builder b={0};b.dol=dol;b.size=size;
 if(!nflstreet_dol_parse_header(dol,size,&b.header))return NULL;return(uint8_t*)sm_span(&b,address,n);}
int main(int argc,char **argv){
 REQUIRE(argc==6);size_t dn=0,rn=0,mn=0;uint8_t *dol=read_file(argv[2],&dn),*reference=read_file(argv[3],&rn),*ram=read_file(argv[4],&mn);
 REQUIRE(dol&&reference&&ram&&mn==24u*1024u*1024u);unsigned cases=(unsigned)strtoul(argv[5],NULL,10);
 nflstreet_slot_material_catalog ref;REQUIRE(nflstreet_slot_material_catalog_open(reference,rn,&ref));
 nflstreet_disc_slot_materials disc={0};REQUIRE(nflstreet_disc_slot_materials_from_disc(argv[1],&disc));
 REQUIRE(disc.size==rn&&!memcmp(disc.bytes,reference,rn));REQUIRE(!consumers(&disc.catalog,&ref,1));
 nflstreet_disc_slot_materials saved=disc;REQUIRE(!nflstreet_disc_slot_materials_from_dol(dol,dn,&disc));REQUIRE(!memcmp(&saved,&disc,sizeof saved));
 nflstreet_disc_slot_materials_destroy(&disc);nflstreet_disc_slot_materials_destroy(&disc);
 uint8_t *copy=malloc(dn),*expected=malloc(rn);REQUIRE(copy&&expected);unsigned malformed=0;
 for(unsigned c=0;c<=cases;++c){memcpy(copy,dol,dn);memcpy(expected,reference,rn);nflstreet_disc_slot_materials owned={0};
  REQUIRE(nflstreet_disc_slot_materials_from_dol(copy,dn,&owned));
  memset(copy,0xa5,dn);REQUIRE(owned.size==rn&&!memcmp(owned.bytes,reference,rn));
  uint32_t seed=sm_read(ram+((c*997u)%(mn-4)))^c;
  REQUIRE(!consumers(&owned.catalog,&ref,seed));nflstreet_disc_slot_materials_destroy(&owned);
  /* Varied structurally valid extraction fixtures; never revision-certified.
     Independent expected bytes start with Python export, with explicit format
     offsets changed only for the corresponding DOL source field. */
  memcpy(copy,dol,dn);uint8_t *p=source_at(copy,dn,SM_SETTINGS,28);REQUIRE(p&&ref.setting_count);
  unsigned item=c%ref.setting_count;p=source_at(copy,dn,SM_SETTINGS+28u*item,28);REQUIRE(p);
  uint32_t min=c%6;sm_write(p+8,min);sm_write(expected+12u+400u+40u*item+20u,min);
  /* Descriptor material names may use another slot name without changing the
     descriptor texture binding. All descriptor names are compared bytewise. */
  uint8_t *list=source_at(copy,dn,SM_DESCRIPTORS,8);REQUIRE(list);uint32_t table=sm_read(list+4);
  unsigned d=c%ref.descriptor_count,s=c%25;uint8_t *desc=source_at(copy,dn,table+48u*d,48),*binding=source_at(copy,dn,SM_BINDING+32u*s,4);
  REQUIRE(desc&&binding);memcpy(desc,binding,4);size_t desc_at=(size_t)(ref.descriptors-reference);
  memcpy(expected+desc_at+48u*d,reference+8u+16u*s,16);
  REQUIRE(sm_build(copy,dn,&owned));REQUIRE(owned.size==rn&&!memcmp(owned.bytes,expected,rn));
  nflstreet_slot_material_catalog changed;REQUIRE(nflstreet_slot_material_catalog_open(expected,rn,&changed));
  REQUIRE(!consumers(&owned.catalog,&changed,seed));nflstreet_disc_slot_materials_destroy(&owned);
 }
 /* Reserved header padding is structurally valid and does not change the
    independently exported catalog, but MUST fail the public revision pin. */
 memcpy(copy,dol,dn);copy[255]^=1;nflstreet_disc_slot_materials wrong_revision={0},wrong_before=wrong_revision;
 REQUIRE(sm_build(copy,dn,&wrong_revision));REQUIRE(wrong_revision.size==rn&&!memcmp(wrong_revision.bytes,reference,rn));
 nflstreet_disc_slot_materials_destroy(&wrong_revision);
 REQUIRE(!nflstreet_disc_slot_materials_from_dol(copy,dn,&wrong_revision));
 REQUIRE(!memcmp(&wrong_revision,&wrong_before,sizeof wrong_revision));
 /* Malformed extraction paths exercised directly behind the public revision
    boundary. Each failed construction must preserve all destination bytes. */
 for(unsigned kind=0;kind<14;++kind){memcpy(copy,dol,dn);uint8_t *p=NULL;
  switch(kind){
  case 0:p=source_at(copy,dn,SM_BINDING,4);REQUIRE(p);sm_write(p,0xfffffffeu);break;
  case 1:p=source_at(copy,dn,SM_BINDING,4);REQUIRE(p);sm_write(p,0);break;
  case 2:p=source_at(copy,dn,SM_BINDING,4);REQUIRE(p);p=source_at(copy,dn,sm_read(p),16);REQUIRE(p);memset(p,'X',16);break;
  case 3:p=source_at(copy,dn,SM_DESCRIPTORS,8);REQUIRE(p);sm_write(p,65);break;
  case 4:p=source_at(copy,dn,SM_DESCRIPTORS,8);REQUIRE(p);sm_write(p+4,0xfffffff0u);break;
  case 5:p=source_at(copy,dn,SM_DESCRIPTORS,8);REQUIRE(p);p=source_at(copy,dn,sm_read(p+4),48);REQUIRE(p);p[9]=2;break;
  case 6:p=source_at(copy,dn,0x80293acc+0x18,4);REQUIRE(p);sm_write(p,0);break;
  case 7:p=source_at(copy,dn,0x80293b40+0x18,4);REQUIRE(p);sm_write(p,9);break;
  case 8:p=source_at(copy,dn,0x80293acc,16);REQUIRE(p);p[0]='x';break;
  case 9:p=source_at(copy,dn,SM_SETTINGS,28);REQUIRE(p);sm_write(p,0xfffffff8u);break;
  case 10:{p=source_at(copy,dn,SM_BINDING,4);REQUIRE(p);uint32_t name=sm_read(p);
   for(unsigned i=0;i<=64;++i){p=source_at(copy,dn,SM_SETTINGS+28u*i,28);REQUIRE(p);sm_write(p,name);}break;}
  case 11:sm_write(copy+0x90,0xffffffffu);break;
  case 12:p=source_at(copy,dn,SM_DESCRIPTORS,8);REQUIRE(p);p=source_at(copy,dn,sm_read(p+4),48);REQUIRE(p);sm_write(p+4,0xfffffffeu);break;
  default:p=source_at(copy,dn,SM_DESCRIPTORS,8);REQUIRE(p);p=source_at(copy,dn,sm_read(p+4),48);REQUIRE(p);sm_write(p+12,0xfffffffeu);break;
  }
  nflstreet_disc_slot_materials out={0},before=out;REQUIRE(!sm_build(copy,dn,&out));REQUIRE(!memcmp(&out,&before,sizeof out));
  REQUIRE(!nflstreet_disc_slot_materials_from_dol(copy,dn,&out));REQUIRE(!memcmp(&out,&before,sizeof out));++malformed;
 }
 nflstreet_disc_slot_materials empty={0},before=empty;
 REQUIRE(!nflstreet_disc_slot_materials_from_dol(NULL,dn,&empty));REQUIRE(!nflstreet_disc_slot_materials_from_dol(dol,255,&empty));
 REQUIRE(!nflstreet_disc_slot_materials_from_disc("W:\\missing-daywater-materials.iso",&empty));REQUIRE(!memcmp(&before,&empty,sizeof empty));
 free(copy);free(expected);free(dol);free(reference);free(ram);
 printf("Disc materials: %u cases; %u malformed fixtures rejected; full owned SMC1 and parser/configure/hook match; source lifetime checked\n",cases+1,malformed);return 0;
}
