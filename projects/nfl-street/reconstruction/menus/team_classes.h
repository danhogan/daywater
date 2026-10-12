#ifndef NFLSTREET_TEAM_CLASSES_H
#define NFLSTREET_TEAM_CLASSES_H
#include "font.h"
#include "script.h"
#include "text_lines.h"

#define NFLSTREET_TEAM_CLASSES_FONT_COUNT 4u
#define NFLSTREET_TEAM_CLASSES_REQUEST_CAPACITY 4096u
#define NFLSTREET_TEAM_CLASSES_ACQUIRE_CAPACITY 8192u
#define NFLSTREET_TEAM_CLASSES_LINE_PLAN_CAPACITY 128u

typedef struct {
 uint32_t target, line_base;
 nflstreet_ui_text_lines plan;
} nflstreet_team_classes_line_plan;

typedef struct {
 nflstreet_font fonts[NFLSTREET_TEAM_CLASSES_FONT_COUNT];
 nflstreet_ui_service_request requests[NFLSTREET_TEAM_CLASSES_REQUEST_CAPACITY];
 uint32_t acquisitions[NFLSTREET_TEAM_CLASSES_ACQUIRE_CAPACITY];
 unsigned request_count, acquisition_count;
 unsigned class_requests[16];
 unsigned image_requests, text_requests;
 nflstreet_team_classes_line_plan line_plans[NFLSTREET_TEAM_CLASSES_LINE_PLAN_CAPACITY];
 unsigned line_plan_count;
} nflstreet_team_classes;

/* Owned entry-7 class-0 image/class-8 text construction only. `state->fonts`
   must point to caller-owned bytes that remain alive through the call. */
int nflstreet_team_classes_prepare_text(nflstreet_team_classes *,uint8_t *,size_t,uint32_t,uint32_t);
int nflstreet_team_classes_construct(uint8_t *,size_t,uint32_t base,
                                     nflstreet_team_classes *state);

#endif
