#ifndef NFLSTREET_FIELD_SCREEN_PREVIEW_H
#define NFLSTREET_FIELD_SCREEN_PREVIEW_H
#include "team_classes.h"
#include "field_screen_runtime.h"
#include <stddef.h>
#include <stdint.h>

/* Render an already relocated and constructed field source snapshot. The
   caller retains source, font bytes, line plans and image/query ownership.
   The host rasterizer is diagnostic; this does not emulate GX. */
int nflstreet_field_screen_preview_render(uint8_t *source,size_t source_size,
    const char *unpacked_root,const nflstreet_team_classes *classes,
    const char *capture_path);
/* The preview has no persistent image cache. This validates every queued
   acquire by opening/decoding its source image; release effects are covered by
   render cleanup. Call only after a successful render, then ACK that exact
   prefix through the source runtime API. */
int nflstreet_field_screen_preview_consume_resource_effects(const char *unpacked_root,
    const nflstreet_field_resource_effect *effects,unsigned count);

/* A host owns the source VM, class plans, rate state and all effects. This
   window only forwards the displayed numeric source event and elapsed tick to
   those callbacks, then rasterizes their resulting source snapshot. */
typedef int (*nflstreet_field_preview_event_fn)(void *context,unsigned event);
typedef int (*nflstreet_field_preview_tick_fn)(void *context,uint32_t elapsed);
typedef int (*nflstreet_field_preview_painted_fn)(void *context);
int nflstreet_field_screen_preview_live(uint8_t *source,size_t source_size,
    const char *unpacked_root,nflstreet_team_classes *classes,
    nflstreet_field_preview_event_fn source_event,
    nflstreet_field_preview_tick_fn source_tick,void *context);
/* Variant for hosts that must handle/acknowledge effects only after GDI has
   successfully painted the updated source snapshot. */
int nflstreet_field_screen_preview_live_after_paint(uint8_t *source,size_t source_size,
    const char *unpacked_root,nflstreet_team_classes *classes,
    nflstreet_field_preview_event_fn source_event,
    nflstreet_field_preview_tick_fn source_tick,void *context,
    nflstreet_field_preview_painted_fn after_paint);
/* Deterministic Win32 message-loop test. It posts the supplied numeric source
   events after `settled_ticks` successful timer/update stages between events. */
int nflstreet_field_screen_preview_live_self_test(uint8_t *source,size_t source_size,
    const char *unpacked_root,nflstreet_team_classes *classes,
    nflstreet_field_preview_event_fn source_event,
    nflstreet_field_preview_tick_fn source_tick,void *context,
    nflstreet_field_preview_painted_fn after_paint,
    const unsigned *events,unsigned event_count,unsigned settled_ticks);
#endif
