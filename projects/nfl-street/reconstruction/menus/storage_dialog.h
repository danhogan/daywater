#ifndef NFLSTREET_STORAGE_DIALOG_H
#define NFLSTREET_STORAGE_DIALOG_H
#include <stdint.h>
/* Original group6/entry4 callback 80122140 and selector 80121F1C.
   Supplied queries and retained external calls; no card I/O or result enum. */
typedef struct { uint32_t function,argc,args[2]; } nflstreet_storage_request;
typedef struct {
 uint32_t word_5a10,word_573c,callbacks[5],source_callbacks[5];
 uint32_t query_slot; /* raw result of80006DC4 */
 uint32_t count;nflstreet_storage_request requests[16];
} nflstreet_storage_dialog;
int nflstreet_storage_common(nflstreet_storage_dialog*,uint32_t property,uint32_t argument,uint32_t*result);
int nflstreet_storage_select(nflstreet_storage_dialog*,uint32_t*delay);
/* Bounded supplied responses to the original80121FF8 polling callback.
   Device calls are retained with scalar arguments; scratch pointers omitted. */
typedef struct {
 uint32_t desired[2],sample_count,samples[8][2];
 uint32_t slot,status,type2_query,probe_result;
} nflstreet_storage_poll_inputs;
typedef struct {uint32_t slot,type,status,id,iterations;} nflstreet_storage_poll_output;
int nflstreet_storage_poll(nflstreet_storage_dialog*,const nflstreet_storage_poll_inputs*,nflstreet_storage_poll_output*);
#endif
