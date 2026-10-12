/* Joined native source test: actual team completion, field recipient input,
   then caller-owned quick-match selection record. This never starts a match. */
#include "team_input_probe.c"
#include "dispatcher.h"
#include "field_screen_runtime.h"
#include "field_availability.h"
#include "field_stat_text.h"
#include "field_screen_preview.h"
#include "quick_match_selection.h"
#include <errno.h>
#include <windows.h>

static int loaded_team_start(uint8_t *,size_t,uint32_t,nflstreet_team_screen *);
static int complete_team(uint8_t *,size_t,nflstreet_team_screen *);
static int join_capture(uint8_t *,size_t,nflstreet_team_screen *,const char *);
static const char *team_sequence="7,7,7,7";
static const char *field_sequence="5,4,5,7";
static const char *field_source_path;
static const char *field_template_path;
static const char *main_dol_path;
static int32_t stat42_input;
static uint32_t field_archive_mask=0x802u;
static uint32_t field_rate_clock=16u;
static const char *field_capture_prefix;
static unsigned field_capture_sequence;

typedef struct {
    nflstreet_field_screen_runtime *runtime;
    const char *root;
    unsigned resources_acked,audio_logged,broadcasts_logged,links_logged,focus_logged;
    uint32_t last_link;
    unsigned saw_link5;
    unsigned self_test,finish_completed;
    uint32_t finish_slot,finish_key;
    unsigned successful_paint_callbacks;
} joined_live_context;
static int joined_live_event(void *opaque,unsigned event) {
    joined_live_context *live=opaque;
    return live&&live->runtime&&nflstreet_field_screen_runtime_event(live->runtime,event);
}
static int joined_live_tick(void *opaque,uint32_t elapsed) {
    joined_live_context *live=opaque;
    return live&&live->runtime&&nflstreet_field_screen_runtime_tick(live->runtime,elapsed);
}

#define DAYWATER_TEAM_STARTUP_HOST daywater_team_startup_host
#define DAYWATER_TEAM_STARTUP_CONTROLS complete_team
#define DAYWATER_TEAM_STARTUP_CAPTURE join_capture
#define nflstreet_team_screen_start(b,n,base,s) loaded_team_start(b,n,base,s)
#define main team_probe_main
#include "team_screen_probe.c"
#undef main
#undef nflstreet_team_screen_start

static uint8_t *join_load(const char *path,size_t *size,size_t limit) {
    FILE *f=fopen(path,"rb");if(!f)return NULL;
    if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}long n=ftell(f);
    if(n<1||(size_t)n>limit||fseek(f,0,SEEK_SET)){fclose(f);return NULL;}
    uint8_t *p=malloc((size_t)n);
    if(!p||fread(p,1,(size_t)n,f)!=(size_t)n){free(p);fclose(f);return NULL;}
    if(fclose(f)){free(p);return NULL;}*size=(size_t)n;return p;
}
static int join_i32(const char *s,int32_t *out) {
    char *end=NULL;errno=0;long long n=strtoll(s,&end,0);
    if(errno||!end||*end||n<INT32_MIN||n>INT32_MAX)return 0;*out=(int32_t)n;return 1;
}
/* The field root object's source-owned cursor is at +0x90.  The event
   callback's supplied_selected_index is only a host query input, so it is not
   used as evidence that the source script navigated. */
static int field_source_cursor(const uint8_t *b,size_t n,uint32_t *cursor) {
    if(!b||!cursor||n<8)return 0;
    uint32_t count=read_be(b),nodes=read_be(b+4);
    if(count==0||count>4096||nodes<0x81100000u||nodes-0x81100000u>n||
       (size_t)count*20>n-(nodes-0x81100000u))return 0;
    uint32_t root=read_be(b+(nodes-0x81100000u));
    if(root<0x81100000u||root-0x81100000u>n||
       n-(root-0x81100000u)<0x94)return 0;
    *cursor=read_be(b+(root-0x81100000u)+0x90);
    return *cursor<8;
}
static int capture_field_state(uint8_t *source,size_t source_size,const char *root,
        nflstreet_field_screen_state *state,const char *label) {
    if(!field_capture_prefix)return 1;
    if(strncmp(field_capture_prefix,"W:\\Daywater\\",12))return 0;
    char path[4096];int count=snprintf(path,sizeof path,"%s-%02u-%s.bmp",
        field_capture_prefix,field_capture_sequence++,label);
    if(count<0||(size_t)count>=sizeof path)return 0;
    if(state->resource_effect_count>128)return 0;
    nflstreet_field_resource_effect pending[128];
    unsigned effect_count=state->resource_effect_count;
    memcpy(pending,state->resource_effects,effect_count*sizeof pending[0]);
    if(!nflstreet_field_screen_preview_render(source,source_size,root,
                                               &state->classes,path))return 0;
    if(!nflstreet_field_screen_preview_consume_resource_effects(root,pending,effect_count) ||
       !nflstreet_field_screen_runtime_ack_resource_effects(state,pending,effect_count))return 0;
    return 1;
}
static void write_live_status(const nflstreet_field_screen_state *state,
        const joined_live_context *live,int self_test,int opened) {
    if(!field_capture_prefix)return;
    char path[4096];int n=snprintf(path,sizeof path,"%s-live-status.txt",field_capture_prefix);
    if(n<0||(size_t)n>=sizeof path)return;
    FILE *f=fopen(path,"wb");if(!f)return;
    uint32_t cursor=UINT32_MAX;
    (void)field_source_cursor(live->runtime->source,live->runtime->source_size,&cursor);
    fprintf(f,"self_test=%d\nopened=%d\nruntime_events=%u\nlast_event=%u\nsource_cursor=%u\ntransition_requests=%u\ntransition_completions=%u\nfinish_completed=%u\ncommit_count=%u\ncommit_slot=%u\ncommit_key=%u\nlink_to5_seen=%u\nsuccessful_paint_callbacks=%u\nresource_queue=%u\nbutton_resource_queue=%u\naudio_queue=%u\nbroadcast_queue=%u\nlink_queue=%u\nfocus_queue=%u\n",self_test,opened,state->runtime_event_count,state->last_runtime_event,cursor,state->transition_request_count,state->transition_completion_count,live->finish_completed,state->field_commit_request_count,state->field_commit_request_count?state->field_commit_requests[0].ui_slot:UINT32_MAX,state->field_commit_request_count?state->field_commit_requests[0].raw_key:UINT32_MAX,live->saw_link5,live->successful_paint_callbacks,state->resource_effect_count,state->main_menu.resource_count,state->main_menu.audio_count,state->main_menu.broadcast_count,state->main_menu.link_count,state->focus_effect_count);
    fclose(f);
}
static int joined_live_after_paint(void *opaque) {
    joined_live_context *live=opaque;
    if(!live||!live->runtime||!live->runtime->state)return 0;
    ++live->successful_paint_callbacks;
    nflstreet_field_screen_state *state=live->runtime->state;
    if(state->resource_effect_count){
        unsigned count=state->resource_effect_count;
        nflstreet_field_resource_effect pending[128];
        if(count>128)return 0;
        memcpy(pending,state->resource_effects,count*sizeof pending[0]);
        if(!nflstreet_field_screen_preview_consume_resource_effects(live->root,pending,count)||
           !nflstreet_field_screen_runtime_ack_resource_effects(state,pending,count))return 0;
        live->resources_acked+=count;
    }
    if(state->main_menu.resource_count){
        unsigned count=state->main_menu.resource_count;nflstreet_ui_button_effect pending[128];
        if(count>128)return 0;memcpy(pending,state->main_menu.resources,count*sizeof pending[0]);
        for(unsigned i=0;i<count;++i)printf("Field source button-resource effect %s archive=%u entry=%u inspected.\n",
            pending[i].kind?"acquire":"release",pending[i].archive,pending[i].entry);
        if(!nflstreet_field_screen_runtime_ack_button_resources(state,pending,count))return 0;
    }
    if(state->main_menu.audio_count){
        unsigned count=state->main_menu.audio_count;uint32_t pending[128];
        if(count>128)return 0;memcpy(pending,state->main_menu.audio,count*sizeof pending[0]);
        for(unsigned i=0;i<count;++i)printf("Field source audio effect word=0x%08x logged, not played.\n",pending[i]);
        if(!nflstreet_field_screen_runtime_ack_audio(state,pending,count))return 0;
        live->audio_logged+=count;
    }
    if(state->main_menu.broadcast_count){
        unsigned count=state->main_menu.broadcast_count;
        nflstreet_ui_service_request pending[128];
        if(count>128)return 0;memcpy(pending,state->main_menu.broadcasts,count*sizeof pending[0]);
        for(unsigned i=0;i<count;++i){const nflstreet_ui_service_request *q=pending+i;
            printf("Field source broadcast inspected: kind=%u target=%u property=0x%08x argc=%u",
                   q->kind,q->target,q->property,q->argc);
            for(unsigned arg=0;arg<q->argc;++arg)printf(" arg%u=0x%08x",arg,q->args[arg]);
            putchar('\n');
        }
        if(!nflstreet_field_screen_runtime_ack_broadcasts(state,pending,count))return 0;
        live->broadcasts_logged+=count;
    }
    if(state->main_menu.link_count){
        unsigned count=state->main_menu.link_count;uint32_t pending[16];
        if(count>16)return 0;memcpy(pending,state->main_menu.links,count*sizeof pending[0]);
        if(live->links_logged>count)return 0;
        for(unsigned i=live->links_logged;i<count;++i){live->last_link=pending[i];live->saw_link5|=pending[i]==5;
            printf("Field source route effect target=%u logged, not dispatched.\n",pending[i]);}
        live->links_logged=count;
    }
    if(state->focus_effect_count){
        unsigned count=state->focus_effect_count;
        nflstreet_ui_focus_effects pending[128];
        if(count>128)return 0;memcpy(pending,state->focus_effects,count*sizeof pending[0]);
        printf("Field source focus effects inspected: %u.\n",count);
        if(!nflstreet_field_screen_runtime_ack_focus_effects(state,pending,count))return 0;
        live->focus_logged+=count;
    }
    if(live->self_test&&!live->finish_completed&&state->last_runtime_event==7&&
       state->transition_completion_count==1&&state->main_menu.link_count==1&&
       state->main_menu.links[0]==live->runtime->linked_screen){
        uint32_t slot=UINT32_MAX,key=UINT32_MAX;
        if(!field_source_cursor(live->runtime->source,live->runtime->source_size,&slot)||
           !nflstreet_field_screen_runtime_key_for_slot(state,slot,&key))return 0;
        state->supplied_selected_index=slot;
        if(!nflstreet_field_screen_runtime_finish(live->runtime)||
           state->field_commit_request_count!=1||
           state->field_commit_requests[0].ui_slot!=slot||
           state->field_commit_requests[0].raw_key!=key||
           state->field_commit_requests[0].supplied_owner_value!=1)return 0;
        unsigned count=state->main_menu.link_count;uint32_t pending[16];
        if(count>16)return 0;memcpy(pending,state->main_menu.links,count*sizeof pending[0]);
        if(!nflstreet_field_screen_runtime_ack_links(state,pending,count))return 0;
        live->finish_slot=slot;live->finish_key=key;live->finish_completed=1;
        printf("Win32 self-test source FFFD commit completed before window exit: slot=%u key=%u owner=1.\n",slot,key);
        if(!joined_live_after_paint(live))return 0;
        return 2;
    }
    return 1;
}
static int verify_coordinator_field8(const nflstreet_team_screen *teams) {
    const char *path="W:\\Daywater\\research-temp\\nflstreet-original-menus-20261009\\nested\\uis_all\\00000\\00005.bin";
    size_t size=0;uint8_t *source=join_load(path,&size,8u*1024u*1024u);
    if(!source)return 0;unsigned relocated=0,flags=0;nflstreet_ui_dispatcher setup={0};
    int ok=nflstreet_ui_relocate(source,size,0x81100000u,&relocated)&&relocated==1&&
      nflstreet_ui_quick_setup_event(source,size,0x81100000u,&flags,&setup,0xfffe,0,0)&&
      nflstreet_ui_quick_setup_event(source,size,0x81100000u,&flags,&setup,1,0x76,UINT32_MAX)&&
      nflstreet_ui_quick_setup_event(source,size,0x81100000u,&flags,&setup,0xfffc,0,0);
    unsigned first=setup.count,forwarded=0;
    for(unsigned i=0;ok&&i<teams->classes.shared.broadcast_count;++i){
      const nflstreet_ui_service_request *q=&teams->classes.shared.broadcasts[i];
      if(q->property==1&&q->argc==2){
        ok=nflstreet_ui_quick_setup_event(source,size,0x81100000u,&flags,&setup,1,q->args[0],q->args[1]);
        ++forwarded;
      }
    }
    if(ok)ok=nflstreet_ui_quick_setup_event(source,size,0x81100000u,&flags,&setup,0xfffc,0,0);
    unsigned loaded=0,activated=0;
    for(unsigned i=first;ok&&i<setup.count;++i){
      const nflstreet_ui_service_request *q=&setup.requests[i];
      if(q->target==8){loaded+=q->property==0;activated+=q->property==3;}
    }
    free(source);return ok&&forwarded==1&&loaded==1&&activated==1;
}
static int selection_failure_checks(const nflstreet_team_screen *teams,
                                    const nflstreet_field_screen_state *field) {
    nflstreet_quick_match_selection out,prior;
    memset(&prior,0xa5,sizeof prior);
    unsigned locked=8;
    for(unsigned slot=0;slot<8;++slot){
        const uint8_t *row=field->field_bootstrap.records+
            slot*NFLSTREET_FIELD_CACHE_RECORD_SIZE;
        if(!row[0xca]||row[0xc9]){locked=slot;break;}
    }
    if(locked==8)return 0;
    memcpy(&out,&prior,sizeof out);
    if(nflstreet_quick_match_selection_build(teams->game.startup.selected_id,
       &teams->activation,&field->field_bootstrap,locked,&out)||memcmp(&out,&prior,sizeof out))return 0;
    nflstreet_team_activation incomplete=teams->activation;
    --incomplete.sides[0].selected_count;
    memcpy(&out,&prior,sizeof out);
    if(nflstreet_quick_match_selection_build(teams->game.startup.selected_id,
       &incomplete,&field->field_bootstrap,0,&out)||memcmp(&out,&prior,sizeof out))return 0;
    nflstreet_team_activation duplicate=teams->activation;
    duplicate.sides[0].active_formation[1]=duplicate.sides[0].active_formation[0];
    memcpy(&out,&prior,sizeof out);
    if(nflstreet_quick_match_selection_build(teams->game.startup.selected_id,
       &duplicate,&field->field_bootstrap,0,&out)||memcmp(&out,&prior,sizeof out))return 0;
    printf("Quick-match input guards pass: unavailable UI slot%u, incomplete lineup and duplicate formation all reject atomically; match/session creation not invoked.\n",locked);
    return 1;
}
static int write_selection_snapshot(const nflstreet_quick_match_selection *selection) {
    if(!field_capture_prefix)return 1;
    if(!selection||strncmp(field_capture_prefix,"W:\\Daywater\\",12))return 0;
    char path[4096],temporary[4128];
    int n=snprintf(path,sizeof path,"%s-selection.json",field_capture_prefix);
    if(n<0||(size_t)n>=sizeof path)return 0;
    int t=snprintf(temporary,sizeof temporary,"%s.tmp",path);
    if(t<0||(size_t)t>=sizeof temporary)return 0;
    FILE *f=fopen(temporary,"wb");if(!f)return 0;
    int ok=fprintf(f,"{\n  \"schema_version\": 1,\n  \"team_ids\": [%u, %u],\n  \"selected_players\": [\n",
        selection->team_ids[0],selection->team_ids[1])>0;
    for(unsigned side=0;ok&&side<2;++side){
        ok=fprintf(f,"    [%u, %u, %u, %u, %u, %u, %u]%s\n",
           selection->selected_players[side][0],selection->selected_players[side][1],
           selection->selected_players[side][2],selection->selected_players[side][3],
           selection->selected_players[side][4],selection->selected_players[side][5],
           selection->selected_players[side][6],side?"":",")>0;
    }
    if(ok)ok=fprintf(f,"  ],\n  \"formation_players\": [\n")>0;
    for(unsigned side=0;ok&&side<2;++side){
        ok=fprintf(f,"    [")>0;
        for(unsigned i=0;ok&&i<14;++i)
            ok=fprintf(f,"%s%u",i?", ":"",selection->formation_players[side][i])>0;
        if(ok)ok=fprintf(f,"]%s\n",side?"":",")>0;
    }
    if(ok)ok=fprintf(f,"  ],\n  \"field_slot\": %u,\n  \"field_key\": %u\n}\n",
        selection->field_slot,selection->field_key)>0;
    if(fclose(f))ok=0;
    if(!ok||!MoveFileExA(temporary,path,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)){
        DeleteFileA(temporary);return 0;
    }
    printf("Private native selection snapshot: %s\n",path);return 1;
}
static int loaded_team_start(uint8_t *b,size_t n,uint32_t base,nflstreet_team_screen *s) {
    const uint32_t args[2]={1,0x69};
    return nflstreet_team_screen_start_context(b,n,base,s,7,5,0x817c0000u,
                                               UINT32_MAX,2,args);
}
static int complete_team(uint8_t *b,size_t n,nflstreet_team_screen *s) {
    const char *p=team_sequence;unsigned count=0;
    while(*p){if(*p<'2'||*p>'7'||++count>32)return 0;unsigned event=(unsigned)(*p++-'0');
        if(*p&&*p++!=',')return 0;
        if(!nflstreet_team_screen_input(b,n,0x81100000u,s,0,(uint16_t)event,1.0f))return 0;
        for(unsigned tick=0;tick<60;++tick)
            if(!nflstreet_team_screen_advance(b,n,0x81100000u,s,16))return 0;
    }
    if(!count||s->activation.sides[0].selected_count!=7||
       s->activation.sides[1].selected_count!=7||s->classes.shared.link_count!=1||
       s->classes.shared.links[0]!=5||s->transition_count!=1)return 0;
    for(unsigned side=0;side<2;++side){
        const nflstreet_team_activation_side *a=&s->activation.sides[side];
        if(a->formation_count!=7)return 0;
        for(unsigned slot=0;slot<14;++slot){unsigned matches=0;
            for(unsigned player=0;player<7;++player)matches+=a->selected_ids[player]==a->active_formation[slot];
            if(matches!=1)return 0;
        }
    }
    return 1;
}
static void free_field_fonts(nflstreet_field_screen_state *state) {
    for(unsigned i=0;i<4;++i)free((void*)state->fonts[i].bytes);
}
static int run_field_recipient(nflstreet_team_screen *teams,const char *root) {
    size_t source_size=0,template_size=0,dol_size=0;
    uint8_t *source=join_load(field_source_path,&source_size,8u*1024u*1024u);
    uint8_t *template_bytes=join_load(field_template_path,&template_size,128u*1024u*1024u);
    uint8_t *dol=join_load(main_dol_path,&dol_size,64u*1024u*1024u);
    nflstreet_field_screen_state *state=calloc(1,sizeof *state);
    if(!source||!template_bytes||!dol||!state)goto fail;
    /* The source's property0x0A path is an explicitly supplied logical port0
       mapping. Keep the other mappings absent, as in the controlled field run. */
    state->supplied_port_mapping_mask=1023;state->port_mapping[0]=0;
    for(unsigned port=1;port<10;++port)state->port_mapping[port]=UINT32_MAX;
    /* Explicit finish owner input for the source's FFFD → 80000002 path. */
    state->supplied_finish_owner_value=1;
    char font_directory[4096];int font_path=snprintf(font_directory,sizeof font_directory,"%s\\uis_font",root);
    if(font_path<0||(size_t)font_path>=sizeof font_directory)goto fail;
    if(!nflstreet_field_screen_runtime_prepare(source,source_size,template_bytes,
       template_size,dol,dol_size,font_directory,state,stat42_input,0,1,0,
       field_archive_mask,field_rate_clock)){fprintf(stderr,"field runtime prepare rejected\n");goto fail;}
    nflstreet_field_screen_runtime runtime={source,source_size,state,8,5,0x817c0000u};
    const uint32_t load_args[2]={1,0x69};
    if(!nflstreet_field_screen_runtime_start(source,source_size,state,8,5,
       0x817c0000u,UINT32_MAX,2,load_args)){fprintf(stderr,"field FFFE/FFFC startup rejected\n");goto fail;}
    if(!state->startup_fffe_completed||!state->startup_fffc_completed){
        fprintf(stderr,"field start counters do not confirm FFFE/FFFC\n");goto fail;
    }
    if(!selection_failure_checks(teams,state)){fprintf(stderr,"quick-match selection rejection guard failed\n");goto fail;}
    for(unsigned tick=0;tick<180;++tick)
        if(!nflstreet_field_screen_runtime_tick(&runtime,16)){fprintf(stderr,"field startup settle tick%u rejected\n",tick);goto fail;}
    if(!capture_field_state(source,source_size,root,state,"startup")){
        fprintf(stderr,"optional field startup render/capture failed\n");goto fail;
    }
    uint32_t source_cursor=UINT32_MAX;
    if(!field_source_cursor(source,source_size,&source_cursor)||source_cursor!=0){
        fprintf(stderr,"field root+0x90 cursor did not start at source-observed slot0\n");goto fail;
    }

    const char *live_env=getenv("DAYWATER_FIELD_JOIN_LIVE");
    const char *self_test_env=getenv("DAYWATER_FIELD_JOIN_LIVE_SELF_TEST");
    int live_self_test=self_test_env&&!strcmp(self_test_env,"1");
    int live_mode=(live_env&&!strcmp(live_env,"1"))||live_self_test;
    unsigned navigation_events=0,confirm_event=0,slot=0;uint32_t key=UINT32_MAX;
    joined_live_context live={0};live.runtime=&runtime;live.root=root;live.self_test=(unsigned)live_self_test;
    if(live_mode){
        unsigned self_test_events[4]={5,4,5,7};
        int opened=live_self_test?
           nflstreet_field_screen_preview_live_self_test(source,source_size,root,
             &state->classes,joined_live_event,joined_live_tick,&live,
             joined_live_after_paint,self_test_events,4,60):
           nflstreet_field_screen_preview_live_after_paint(source,source_size,root,
             &state->classes,joined_live_event,joined_live_tick,&live,
             joined_live_after_paint);
        write_live_status(state,&live,live_self_test,opened);
        if(!opened){
            fprintf(stderr,"optional live field source window failed; no physical-input claim\n");goto fail;
        }
        if(!field_source_cursor(source,source_size,&source_cursor))goto fail;
        slot=source_cursor;confirm_event=state->last_runtime_event;
        if(!nflstreet_field_screen_runtime_key_for_slot(state,slot,&key)||slot>=8){
            fprintf(stderr,"live field cursor%u has no DOL key mapping\n",slot);goto fail;
        }
        navigation_events=state->runtime_event_count;
        /* If the user confirmed immediately before closing the window, keep
           advancing the supplied transition clock until its bounded completion. */
        if(state->transition_request_count&&!state->transition_completion_count)
            for(unsigned tick=0;tick<60&&!state->transition_completion_count;++tick)
                if(!nflstreet_field_screen_runtime_tick(&runtime,16))goto fail;
        if(!capture_field_state(source,source_size,root,state,"window-closed"))goto fail;
        if(confirm_event!=7||state->transition_request_count!=1||
           state->transition_completion_count!=1||!state->transition_completions[0].completed||
           !live.saw_link5){
            printf("Live field window closed cleanly: source cursor=%u; last source event=%u; transition requests/completions=%u/%u; link-to-5 observed=%u; no confirmed field selection record was written.\n",
                slot,confirm_event,state->transition_request_count,
                state->transition_completion_count,live.saw_link5);
            free(source);free(template_bytes);free(dol);free_field_fonts(state);free(state);return 1;
        }
        printf("Live field source confirmation observed: event7, cursor=%u, transition request/completion=1/1, logged link-to-5=%u; audio/broadcast/route effects were inspected and not played/dispatched.\n",
            slot,live.saw_link5);
        if(live_self_test&&(!live.finish_completed||!state->runtime_finish_completed||
           live.finish_slot!=slot||live.finish_key!=key||live.successful_paint_callbacks<4||
           state->resource_effect_count||state->main_menu.resource_count||
           state->main_menu.audio_count||state->main_menu.broadcast_count||
           state->main_menu.link_count||state->focus_effect_count)){
            fprintf(stderr,"Win32 self-test did not finish matching commit after successful paints and queue handling\n");goto fail;}
        if(live_self_test)printf("Win32 source self-test passed: successful post-paint callbacks=%u; resource/button/audio/broadcast/link/focus queues are empty.\n",live.successful_paint_callbacks);
    } else {
        unsigned events[32],event_count=0;const char *p=field_sequence;
        while(*p){if(*p<'2'||*p>'7'||event_count>=32)goto fail;events[event_count++]=(unsigned)(*p++-'0');
            if(*p&&*p++!=',')goto fail;
        }
        if(event_count<2||(events[event_count-1]!=6&&events[event_count-1]!=7)){
            fprintf(stderr,"field sequence must include navigation and final numeric source confirm/back event6 or7\n");goto fail;
        }
        uint32_t expected_cursor=0;
        for(unsigned i=0;i+1<event_count;++i){unsigned event=events[i];
            if(event==6||event==7){fprintf(stderr,"confirmation must be the final supplied event\n");goto fail;}
            if(!nflstreet_field_screen_runtime_event(&runtime,event)){
                fprintf(stderr,"field source event%u rejected; native recipient state retained\n",event);goto fail;}
            ++navigation_events;
            if(event==5){if(expected_cursor<7)++expected_cursor;}
            else if(event==4){if(expected_cursor>0)--expected_cursor;}
            if(!field_source_cursor(source,source_size,&source_cursor)||source_cursor!=expected_cursor){
                fprintf(stderr,"field event%u source root+0x90 cursor=%u, expected=%u\n",event,source_cursor,expected_cursor);goto fail;}
            printf("Field source numeric event%u accepted; callback observed UI slot=%u; independent root+0x90 cursor=%u.\n",
                   event,state->supplied_selected_index,source_cursor);
            for(unsigned tick=0;tick<60;++tick)
                if(!nflstreet_field_screen_runtime_tick(&runtime,16)){fprintf(stderr,"field event%u follow-up tick%u rejected\n",event,tick);goto fail;}
            char label[32];snprintf(label,sizeof label,"event%02u-step%u",event,i+1);
            if(!capture_field_state(source,source_size,root,state,label)){
                fprintf(stderr,"optional field event%u render/capture failed\n",event);goto fail;}
        }
        if(!navigation_events){fprintf(stderr,"no field navigation event supplied\n");goto fail;}
        slot=source_cursor;confirm_event=events[event_count-1];
        if(!nflstreet_field_screen_runtime_event(&runtime,confirm_event)){fprintf(stderr,"field confirmation event%u rejected; no native selection record\n",confirm_event);goto fail;}
        for(unsigned tick=0;tick<60;++tick)
            if(!nflstreet_field_screen_runtime_tick(&runtime,16)){fprintf(stderr,"field confirmation settle tick%u rejected\n",tick);goto fail;}
        if(!capture_field_state(source,source_size,root,state,"confirmed")){
            fprintf(stderr,"optional field confirmation render/capture failed\n");goto fail;}
        if(confirm_event!=7){
            printf("Field source back event6 processed; no native field commit or selection snapshot was created.\n");
            free(source);free(template_bytes);free(dol);free_field_fonts(state);free(state);return 1;
        }
        printf("Field source after event%u: transition requests=%u completions=%u link effects=%u runtime events=%u.\n",
           confirm_event,state->transition_request_count,state->transition_completion_count,
           state->main_menu.link_count,state->runtime_event_count);
        if(state->transition_request_count!=1||state->transition_completion_count!=1||
           !state->transition_completions[0].completed||state->main_menu.link_count!=1||
           state->main_menu.links[0]!=5){
            fprintf(stderr,"source confirm did not produce exactly one completed transition and link-to-5 effect\n");goto fail;}
    }
    if(!nflstreet_field_screen_runtime_key_for_slot(state,slot,&key)||slot>=8){fprintf(stderr,"field observed slot%u has no DOL key mapping\n",slot);goto fail;}
    const uint8_t *field_record=state->field_bootstrap.records+
        slot*NFLSTREET_FIELD_CACHE_RECORD_SIZE;
    if(!field_record[0xca]||field_record[0xc9]){
        fprintf(stderr,"source-selected slot%u/key%u is not eligible in supplied no-profile snapshot (CA=%02x C9=%02x)\n",slot,key,field_record[0xca],field_record[0xc9]);goto fail;}
    state->supplied_selected_index=slot;
    if(!state->runtime_finish_completed&&!nflstreet_field_screen_runtime_finish(&runtime)){
        fprintf(stderr,"source FFFD finish rejected; no field commit request\n");goto fail;
    }
    if(state->field_commit_request_count!=1||
       state->field_commit_requests[0].ui_slot!=slot||
       state->field_commit_requests[0].raw_key!=key||
       state->field_commit_requests[0].supplied_owner_value!=1||
       !state->runtime_finish_completed){
        fprintf(stderr,"source FFFD commit request disagrees with observed field slot/key\n");goto fail;
    }
    if(state->main_menu.link_count){
        unsigned count=state->main_menu.link_count;uint32_t routes[16];
        if(count>16)goto fail;memcpy(routes,state->main_menu.links,count*sizeof routes[0]);
        if(!nflstreet_field_screen_runtime_ack_links(state,routes,count))goto fail;
    }
    if(live_mode&&field_capture_prefix){
        if(!capture_field_state(source,source_size,root,state,"source-finished")||
           !joined_live_after_paint(&live))goto fail;
    }

    nflstreet_quick_match_selection selection={0};
    if(!nflstreet_quick_match_selection_build(teams->game.startup.selected_id,
           &teams->activation,&state->field_bootstrap,slot,&selection)){fprintf(stderr,"native quick-match selection builder rejected team/field inputs; no session created\n");goto fail;}
    if(selection.field_key!=key||memcmp(selection.formation_players[0],
       teams->activation.sides[0].active_formation,sizeof selection.formation_players[0])||
       memcmp(selection.formation_players[1],teams->activation.sides[1].active_formation,
       sizeof selection.formation_players[1]))goto fail;
    if(!write_selection_snapshot(&selection)){fprintf(stderr,"could not atomically write private native selection snapshot\n");goto fail;}
    printf("Joined native field source commit request: slot=%u raw DIVE key=%u owner=%u; field cache release requests=%u; FFFD completed.\n",
       state->field_commit_requests[0].ui_slot,state->field_commit_requests[0].raw_key,
       state->field_commit_requests[0].supplied_owner_value,state->field_cache_release_requests);
    printf("Joined native field selection record: teams=%u/%u; selected roster counts=7/7; formation arrays retained; source field UI slot=%u raw DIVE key=%u; cache CA=%02x C9=%02x; stat42=%d/no-profile IVID; archive mask=0x%08x; navigation events=%u; confirm event%u; field FFFE/FFFC completed; no 801EA480 or match-start claim.\n",
       selection.team_ids[0],selection.team_ids[1],slot,key,field_record[0xca],field_record[0xc9],
       stat42_input,field_archive_mask,navigation_events,confirm_event);
    free(source);free(template_bytes);free(dol);free_field_fonts(state);free(state);return 1;
fail:
    free(source);free(template_bytes);free(dol);if(state){free_field_fonts(state);free(state);}return 0;
}
static int join_capture(uint8_t *b,size_t n,nflstreet_team_screen *s,const char *root) {
    (void)b;(void)n;
    if(!s||!root||s->classes.shared.broadcast_count<1)return 0;
    unsigned field_requests=0;
    for(unsigned i=0;i<s->classes.shared.broadcast_count;++i){
        const nflstreet_ui_service_request *q=&s->classes.shared.broadcasts[i];
        if(q->property==1&&q->argc==2)++field_requests;
    }
    if(field_requests!=1){fprintf(stderr,"expected one source completion forwarded to coordinator5\n");return 0;}
    if(!verify_coordinator_field8(s)){fprintf(stderr,"coordinator did not load/activate field8 from team completion\n");return 0;}
    return run_field_recipient(s,root);
}
int main(int argc,char **argv) {
    if(argc!=9&&argc!=10)return 2;
    field_source_path=argv[5];field_template_path=argv[6];
    main_dol_path=argv[3];if(!join_i32(argv[7],&stat42_input))return 2;
    field_sequence=argv[8];if(argc==10)team_sequence=argv[9];
    field_capture_prefix=getenv("DAYWATER_FIELD_JOIN_CAPTURE_PREFIX");
    char *team_args[5]={argv[0],argv[1],argv[2],argv[3],argv[4]};
    return team_probe_main(5,team_args);
}
