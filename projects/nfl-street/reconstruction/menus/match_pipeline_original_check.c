/* Reuse the checked session ABI recorder. The original settings recipient is
   executed below, rather than treated as a successful resolution boundary. */
#define main bounded_session_checker_main
#include "match_session_original_check.c"
#undef main
#include "match_pipeline.h"
#include "match_player_query.h"
typedef struct {
    env base;
    const uint8_t *database; size_t database_size;
    unsigned settings_calls;
    uint32_t insert_words[24]; double insert_float;
    uint32_t selected[25], normalized[25];
} pipeline_env;
static void settings_values(uint32_t*v){
    for(unsigned i=0;i<25;++i)v[i]=0x100u+i;
    v[0]=0x4479ff5cu;v[1]=111;v[2]=2047;v[3]=15;v[4]=127;v[5]=127;v[6]=127;v[7]=UINT32_MAX;
    v[8]=0x10;v[9]=63;v[10]=7;v[11]=4;v[12]=3;v[13]=255;
    v[14]=0x20;v[15]=63;v[16]=8;v[17]=4;v[18]=3;v[19]=255;
    v[20]=UINT32_MAX;v[21]=1023;v[22]=99;v[23]=15;v[24]=255;
}
static uint32_t settings_pc(nflstreet_match_settings_query q){
    static const uint32_t pcs[]={0x80266f44u,0x80267118u,0x8026713cu,0x802671a0u};return pcs[q];
}
static int32_t settings_native(void*opaque,nflstreet_match_settings_query q,uint32_t arg,uint32_t*v,uint32_t count,double f){
    pipeline_env*e=opaque;uint32_t args[5]={0};if(q==NFLSTREET_SETTING_SIDE_PAIR)args[0]=arg;
    entry*r=record(&e->base,settings_pc(q),args);++e->settings_calls;
    if(q==NFLSTREET_SETTING_SESSION_INSERT){if(count!=25)exit(8);memcpy(e->normalized,v,100);memcpy(e->insert_words,v+1,96);e->insert_float=f;}
    if(r->status)return r->status;
    if(q==NFLSTREET_SETTING_INITIAL){if(count!=25)exit(8);settings_values(v);memcpy(e->selected,v,100);}
    else if(q==NFLSTREET_SETTING_DEPENDENT_DEFAULT){if(count!=1||arg!=0)exit(8);v[0]=55;}
    else if(q==NFLSTREET_SETTING_SIDE_PAIR){if(count!=2)exit(8);v[0]=arg+1;v[1]=arg+2;r->output[0]=v[0];r->output[1]=v[1];}
    else {if(count!=25)exit(8);memcpy(e->normalized,v,100);memcpy(e->insert_words,v+1,96);e->insert_float=f;}
    return 0;
}
static int settings_original(CPUState*c,uint8_t*ram,pipeline_env*e){
    if(c->pc!=0x801b4510u)return 0;uint32_t pc=c->gpr[4],args[5]={0};
    if(pc!=0x80266f44u&&pc!=0x80267118u&&pc!=0x8026713cu&&pc!=0x802671a0u)return 0;
    if(pc==0x8026713cu)args[0]=c->gpr[7];entry*r=record(&e->base,pc,args);++e->settings_calls;
    if(pc==0x802671a0u){for(unsigned i=0;i<6;++i)e->insert_words[i]=c->gpr[5+i];
        for(unsigned i=0;i<18;++i)e->insert_words[6+i]=be(ptr(ram,c->gpr[1]+8+4*i,4));e->insert_float=c->fpr[1];}
    if(!r->status){
        if(pc==0x80266f44u){uint32_t dest[25];settings_values(e->selected);
            for(unsigned i=0;i<6;++i)dest[i]=c->gpr[5+i];
            for(unsigned i=0;i<19;++i)dest[6+i]=be(ptr(ram,c->gpr[1]+8+4*i,4));
            for(unsigned i=0;i<25;++i){uint8_t*p=ptr(ram,dest[i],4);if(!p)return -1;put(p,e->selected[i]);}}
        else if(pc==0x80267118u){uint8_t*p=ptr(ram,c->gpr[1]+172,4);if(!p)return -1;put(p,55);}
        else if(pc==0x8026713cu){uint8_t*a=ptr(ram,c->gpr[5],4),*b=ptr(ram,c->gpr[6],4);if(!a||!b)return -1;
            r->output[0]=args[0]+1;r->output[1]=args[0]+2;put(a,r->output[0]);put(b,r->output[1]);}
        else {for(unsigned i=0;i<6;++i)e->insert_words[i]=c->gpr[5+i];
            for(unsigned i=0;i<18;++i)e->insert_words[6+i]=be(ptr(ram,c->gpr[1]+8+4*i,4));e->insert_float=c->fpr[1];}
    }
    c->gpr[3]=(uint32_t)r->status;c->pc=c->lr;return 1;
}
static int32_t actual_player(pipeline_env*e,uint32_t word,uint32_t*out){
    if(!nflstreet_match_player_query(e->database,e->database_size,1,word,out)){fprintf(stderr,"actual PLAY lookup rejected\n");exit(8);}return 0;
}
static int player_original(CPUState*c,uint8_t*ram,pipeline_env*e){
    if(c->pc!=0x801b4510u)return 0;uint32_t pc=c->gpr[4];
    if(pc==0x80268b98u){uint32_t args[5]={c->gpr[6],0,0,0,0};entry*r=record(&e->base,pc,args);
        r->output[0]=1;put(ptr(ram,c->gpr[5],4),1);c->gpr[3]=(uint32_t)r->status;c->pc=c->lr;return 1;}
    if(pc==0x80268bd0u){uint32_t args[5]={c->gpr[6],c->gpr[7],0,0,0};entry*r=record(&e->base,pc,args);
        if(c->gpr[7]!=1)return -1;actual_player(e,c->gpr[6],&r->output[0]);put(ptr(ram,c->gpr[5],4),r->output[0]);c->gpr[3]=(uint32_t)r->status;c->pc=c->lr;return 1;}
    return 0;
}
static int32_t start_native(void*opaque,const nflstreet_match_start_request*q,uint32_t*out){
    pipeline_env*e=opaque;
    if(q->kind==NFLSTREET_MATCH_QUERY_SIDE||q->kind==NFLSTREET_MATCH_QUERY_PLAYER){uint32_t args[5]={0};
        uint32_t pc=q->kind==NFLSTREET_MATCH_QUERY_SIDE?0x80268b98u:0x80268bd0u;
        if(q->kind==NFLSTREET_MATCH_QUERY_SIDE){args[0]=q->side;*out=1;}
        else {args[0]=q->value;args[1]=q->context;if(q->context!=1)exit(8);actual_player(e,q->value,out);}
        entry*r=record(&e->base,pc,args);r->output[0]=*out;return r->status;}
    joined_env fake={&e->base,NULL,NULL};return native_outer(&fake,q,out);
}
static int selection(const char*path,uint32_t out[2][14]){
    size_t n=0;char*t=(char*)load(path,&n);if(!t||n>65536){free(t);return 0;}char*z=realloc(t,n+1);if(!z){free(t);return 0;}t=z;t[n]=0;
    const char*p=strstr(t,"\"formation_players\"");if(!p){free(t);return 0;}p=strchr(p,'[');if(!p){free(t);return 0;}
    for(unsigned i=0;i<28;++i){while(*p&&(*p<'0'||*p>'9'))++p;if(!*p){free(t);return 0;}
        char*end;unsigned long x=strtoul(p,&end,10);if(end==p||x>UINT32_MAX){free(t);return 0;}out[i/14][i%14]=(uint32_t)x;p=end;}
    free(t);return 1;
}
static int pipeline_case(const ModernGekkoModuleDesc*m,const uint8_t*baseline,const uint32_t forms[2][14],const uint8_t*db,size_t db_size,const plan*p,const char*label){
    uint8_t*ram=malloc(RAM_SIZE);if(!ram)return 0;memcpy(ram,baseline,RAM_SIZE);
    for(unsigned side=0;side<2;++side)for(unsigned i=0;i<14;++i)put(ptr(ram,side?ARRAY_B:ARRAY_A,56)+4*i,forms[side][i]);
    *ptr(ram,R13-0x62a8u,1)=p->initialized;put(ptr(ram,R13-0x62b4u,4),p->pre?PRE_CALLBACK:0);put(ptr(ram,R13-0x62b0u,4),p->post?POST_CALLBACK:0);
    trace ot={0},nt={0};pipeline_env oe={0},ne={0};oe.base.p=ne.base.p=p;oe.base.t=&ot;ne.base.t=&nt;
    oe.database=ne.database=db;oe.database_size=ne.database_size=db_size;
    CPUState c={0};c.ram=ram;c.ram_size=RAM_SIZE;c.msr=0x2000;c.gpr[1]=STACK;c.gpr[2]=0x803b9a40u;c.gpr[13]=R13;
    c.gpr[3]=ARRAY_A;c.gpr[4]=ARRAY_B;c.pc=0x801ea480u;c.cycle_budget=200000000;
    uint32_t accepted_address=0;unsigned resolve_calls=0;
    for(unsigned step=0;c.pc&&step<200000;++step){
        if(c.pc==0x801e577cu)accepted_address=c.gpr[5];
        if(c.pc==0x801e51d4u)++resolve_calls;
        int handled=settings_original(&c,ram,&oe);if(!handled)handled=player_original(&c,ram,&oe);
        if(!handled&&c.pc!=0x801e51d4u)handled=original_service(&c,ram,&oe.base);
        if(!handled)handled=outer_original(&c,ram,&oe.base);
        if(handled<0){free(ram);return 0;}if(handled)continue;
        c.downcount=-1000000000;m->dispatch(&c,c.pc);if(c.exception){fprintf(stderr,"pipeline exception pc=%08x\n",c.pc);free(ram);return 0;}
    }
    if(c.pc||!accepted_address||resolve_calls!=1){free(ram);return 0;}
    nflstreet_match_session_owner owner={p->initialized,p->pre,p->post};nflstreet_match_pipeline_result result={0};
    nflstreet_match_pipeline_services services={start_native,&ne,settings_native,&ne,native_service,&ne.base};
    if(!nflstreet_match_pipeline_run(forms,&owner,&services,&result)){free(ram);return 0;}
    int ok=compare(&ot,&nt)&&c.gpr[3]==(uint32_t)result.start.status&&*ptr(ram,accepted_address,1)==result.session.continuation&&
        *ptr(ram,R13-0x62a8u,1)==owner.initialized&&oe.settings_calls==ne.settings_calls&&
        !memcmp(oe.selected,ne.selected,100)&&!memcmp(oe.insert_words,ne.insert_words,96)&&oe.insert_float==ne.insert_float&&
        !memcmp(oe.base.arrays,ne.base.arrays,sizeof oe.base.arrays)&&result.settings_called==1&&result.session_called==1;
    if(result.settings.insert_descriptor_ready){if(memcmp(ne.normalized,result.settings.normalized_values,100))ok=0;}
    for(unsigned side=0;side<2;++side)for(unsigned i=0;i<14;++i)if(be(ptr(ram,side?ARRAY_B:ARRAY_A,56)+4*i)!=result.start.formations[side][i])ok=0;
    if(!ok)fprintf(stderr,"pipeline %s mismatch status%08x/%08x trace%u/%u settings%u/%u\n",label,c.gpr[3],(uint32_t)result.start.status,ot.count,nt.count,oe.settings_calls,ne.settings_calls);
    else printf("%s: pipeline exact status=%08x continuation=%u initialized=%u trace=%u settings=%u insert-float=%.8g\n",label,c.gpr[3],result.session.continuation,owner.initialized,ot.count,oe.settings_calls,oe.insert_float);
    free(ram);return ok;
}
int main(int argc,char**argv){
    if(argc!=5){fprintf(stderr,"Usage: match-pipeline-original-check.exe DLL RAM selection.json actual-team1DB\n");return 2;}
    HMODULE h=LoadLibraryA(argv[1]);if(!h)return 3;ModernGekkoGetModuleFn get=(ModernGekkoGetModuleFn)GetProcAddress(h,MODERNGEKKO_GET_MODULE_SYMBOL);
    if(!get){FreeLibrary(h);return 3;}const ModernGekkoModuleDesc*m=get();if(!m||m->cpu_state_size!=sizeof(CPUState)){FreeLibrary(h);return 3;}
    size_t size=0,db_size=0;uint8_t*baseline=load(argv[2],&size),*db=load(argv[4],&db_size);uint32_t forms[2][14];
    if(!baseline||size!=RAM_SIZE||!db||!selection(argv[3],forms)){free(baseline);free(db);FreeLibrary(h);return 4;}
    plan p={0};p.initialized=1;p.pre=p.post=p.cancel=1;int ok=pipeline_case(m,baseline,forms,db,db_size,&p,"actual-selection-success");
    const uint32_t errors[]={0x80266f44u,0x80267118u,0x8026713cu,0x802671a0u,0x801e5628u,0x801e0e7cu,0x80268b00u};
    for(unsigned i=0;ok&&i<sizeof errors/sizeof errors[0];++i){char label[48];snprintf(label,sizeof label,"pipeline-error-%08x",errors[i]);p.fail_pc=errors[i];p.fail_status=0x71+(int32_t)i;ok=pipeline_case(m,baseline,forms,db,db_size,&p,label);}
    p.fail_pc=0;p.fail_status=0;p.cancel=0;if(ok)ok=pipeline_case(m,baseline,forms,db,db_size,&p,"pipeline-cancel-update");
    free(baseline);free(db);FreeLibrary(h);if(ok)puts("Joined original801EA480-801E577C-801E51D4 differential passed using actual Win32 selection and PLAY rows; all downstream stages externally supplied.");return ok?0:5;
}
