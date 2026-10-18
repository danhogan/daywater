#ifndef NFLSTREET_MATRIX_STORAGE_H
#define NFLSTREET_MATRIX_STORAGE_H
#include "object_pool.h"
#include "matrix_stack.h"
typedef struct {
 void *user;
 uint32_t (*heap)(void *,uint32_t owner,uint32_t size,uint32_t flags);
 uint32_t (*get_error)(void *);
 void (*error)(void *,uint32_t code);
} nflstreet_matrix_storage_services;
typedef struct {
 nflstreet_pool_ram ram;
 uint32_t flag_slot,table_slot,count_slot,current_slot;
 uint32_t zero_slot,one_slot;
 const nflstreet_matrix_storage_services *services;
} nflstreet_matrix_storage_context;
/* 8018908C. Adds eight reserved table entries. Publishes table0 even on
   allocation failure; count/current remain unchanged on failure. Returns host
   status separately from original error code. Nonzero flag returns50001.
   Host contract: bounded accessed words and nonwrapping (requested+8)*12. */
int nflstreet_matrix_storage_open(const nflstreet_matrix_storage_context *,uint32_t requested,uint32_t *code);
/* 80189068. Original table-entry identity, or0 when index>=count. */
int nflstreet_matrix_storage_entry(const nflstreet_matrix_storage_context *,uint32_t index,uint32_t *entry);
/* 801891E8. Allocates depth*64, replaces one entry without freeing old slots,
   and initializes ONLY its first matrix. A successful depth0 allocation still
   writes64 bytes, as original; backing RAM must cover that write. Invalid
   index and wrapping sizes are host rejection, not original validation. */
int nflstreet_matrix_storage_allocate(const nflstreet_matrix_storage_context *,uint32_t index,uint32_t depth,uint32_t *code);
/* Refresh only table/count/current in an existing consumer configuration.
   Caller retains index/sine/angle constants and RAM/service lifetimes. */
int nflstreet_matrix_storage_bind(const nflstreet_matrix_storage_context *,nflstreet_matrix_stacks *);
#endif
