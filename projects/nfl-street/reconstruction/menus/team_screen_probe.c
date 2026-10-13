#include "team_screen.h"
#include "screen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint8_t*load(const char*p,size_t*n){FILE*f=fopen(p,"rb");if(!f)return NULL;if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}long z=ftell(f);if(z<1||z>8*1024*1024){fclose(f);return NULL;}rewind(f);uint8_t*b=malloc((size_t)z);if(!b||fread(b,1,(size_t)z,f)!=(size_t)z){free(b);fclose(f);return NULL;}fclose(f);*n=(size_t)z;return b;}
int main(int argc,char**argv){
 if(argc<3||argc>5)return 2;size_t n;uint8_t*b=load(argv[1],&n);nflstreet_team_screen*s=calloc(1,sizeof *s);if(!b||!s)return 3;unsigned relocated;
 nflstreet_team_bootstrap_static constants={0};nflstreet_team_bootstrap host={0};
 if(argc>=4){size_t z,tz;uint8_t*d=load(argv[3],&z);char p[4096];snprintf(p,sizeof p,"%s/template/00002.bin",argv[2]);uint8_t*t=load(p,&tz);
  if(!d||!t||!nflstreet_team_bootstrap_static_from_dol(d,z,&constants))return 5;free(d);
  host.constants=&constants;host.template_db=t;host.template_size=tz;host.weighting_mode=0;
  if(!nflstreet_team_bootstrap_curve_from_template(t,tz,constants.mapped_dol,constants.mapped_size,0x80000000,&host.curve))return 5;s->bootstrap=&host;
 }
 if(!nflstreet_ui_relocate(b,n,0x81100000,&relocated))return 4;
 for(unsigned i=0;i<4;++i){char p[4096];size_t z;snprintf(p,sizeof p,"%s/uis_font/%05u.bin",argv[2],i);uint8_t*f=load(p,&z);if(!f||!nflstreet_font_open(f,z,&s->objects.fonts[i]))return 5;if(i==3)s->classes.shared.font=s->objects.fonts[i];else s->classes.fonts[i]=s->objects.fonts[i];}
 for(unsigned i=1;i<=32;++i){char p[4096];size_t z;snprintf(p,sizeof p,"%s/db_teams/%05u.bin",argv[2],i);uint8_t*r=load(p,&z);nflstreet_team_table t;if(!r||!nflstreet_team_table_open(r,z,&t)||!nflstreet_team_catalog_append(&s->game.catalog,&t))return 5;unsigned k=s->supplied_roster_count++;uint32_t team,count,assetcount;
 if(!nflstreet_team_preview_team_id_from_db(r,z,&team))return 5;
 /* Loaded STAT membership is authoritative. The fallback block supplies
    catalog metadata, but cannot replace a team already present in STAT. Keep
    one owned copy per roster so existing teardown ownership stays unchanged. */
 if(s->bootstrap){nflstreet_team_table stat;unsigned id=UINT32_MAX,loaded=0;
  if(!nflstreet_team_table_open_tag(host.template_db,host.template_size,0x59414c50u,&stat))return 5;
  for(unsigned f=0;f<stat.field_count;++f){nflstreet_team_field field;if(!nflstreet_team_table_field(&stat,f,&field))return 5;if(field.tag==0x44494754u)id=f;}
  if(id==UINT32_MAX)return 5;
  for(unsigned row=0;row<stat.used;++row){uint32_t value;if(!nflstreet_team_table_number(&stat,row,id,&value))return 5;loaded|=value==team;}
  if(loaded){uint8_t *source=malloc(host.template_size);if(!source)return 5;memcpy(source,host.template_db,host.template_size);free(r);r=source;z=host.template_size;}
 }
 if(!nflstreet_team_preview_roster_from_db(r,z,team,s->supplied_rosters[k].players,&count)||!nflstreet_team_preview_assets_from_db(r,z,team,s->supplied_rosters[k].assets,&assetcount)||count!=assetcount){fprintf(stderr,"Roster source failed team%u database bytes%zu\n",team,z);return 5;}
 s->supplied_rosters[k].id=team;s->supplied_rosters[k].count=count;if(s->bootstrap){s->roster_databases[k]=r;s->roster_database_sizes[k]=z;}else free(r);}
 if(s->bootstrap){unsigned cases=0;
  for(unsigned mode=0;mode<5;++mode){host.weighting_mode=mode;
   for(unsigned k=0;k<s->supplied_roster_count;++k)for(unsigned side=0;side<2;++side){
    nflstreet_team_activation a={0};nflstreet_team_player_window_side w;nflstreet_team_label_row labels[20];uint32_t selected[7];
    if(!nflstreet_team_bootstrap_prepare(&host,s->roster_databases[k],s->roster_database_sizes[k],s->supplied_rosters[k].id,side,&a.sides[side],&w,labels)||!nflstreet_team_activation_apply(&a,side,selected,7)||a.sides[side].selected_count!=7||a.sides[side].rebuild)return 9;
    for(unsigned i=0;i<7;++i){unsigned hits=0;for(unsigned j=0;j<s->supplied_rosters[k].count;++j)if(selected[i]==s->supplied_rosters[k].players[j])++hits;if(hits!=1)return 9;for(unsigned j=0;j<i;++j)if(selected[j]==selected[i])return 9;}
    ++cases;
   }
  }
  host.weighting_mode=0;printf("Fresh authentic-data lineup/formation checks pass: %u team/side/mode cases.\n",cases);
 }
 for(unsigned side=0;side<2;++side)s->game.startup.prior_profile[side]=s->game.startup.selected_id[side]=UINT32_MAX;
 s->supplied_port_mapping_mask=1;s->port_mapping[0]=0; /* Explicit inspection controller mapping. */
 s->supplied_common_mode=6;s->game.query_mode=1;s->supplied_static_archives=0xffffffffu;s->classes.shared.rates.clock=16;s->classes.shared.link_enabled=1;s->classes.shared.linked_screen=5;
#ifdef DAYWATER_TEAM_STARTUP_HOST
 if(!DAYWATER_TEAM_STARTUP_HOST(s))return 12;
#endif
 if(!nflstreet_team_screen_construct(b,n,0x81100000,s)){fputs("Team source constructor unavailable\n",stderr);return 6;}
 if(!nflstreet_team_screen_start(b,n,0x81100000,s)){fputs("Team source activation unavailable\n",stderr);return 7;}
 unsigned frames=argc==5?(unsigned)strtoul(argv[4],NULL,10):160;if(!frames||frames>10000)return 2;
 for(unsigned i=0;i<frames;++i)if(!nflstreet_team_screen_advance(b,n,0x81100000,s,16)){fprintf(stderr,"Team source update unavailable frame%u\n",i);return 8;}
#ifdef DAYWATER_TEAM_STARTUP_CONTROLS
 if(!DAYWATER_TEAM_STARTUP_CONTROLS(b,n,s))return 11;
#endif
 printf("Independent native team source creation/activation and%u updates pass; selected teams%u/%u, lineup sizes%u/%u, spawn requests%u. Original full startup comparison and visible player previews remain required.\n",frames,s->game.startup.selected_id[0],s->game.startup.selected_id[1],s->activation.sides[0].selected_count,s->activation.sides[1].selected_count,s->player_spawn_count);
#ifdef DAYWATER_TEAM_STARTUP_CAPTURE
 if(!DAYWATER_TEAM_STARTUP_CAPTURE(b,n,s,argv[2]))return 10;
#endif
 for(unsigned k=0;k<s->supplied_roster_count;++k)free((void*)s->roster_databases[k]);
 for(unsigned i=0;i<4;++i)free((void*)s->objects.fonts[i].bytes);
 free((void*)host.template_db);nflstreet_team_bootstrap_static_free(&constants);free(s);free(b);return 0;
}
