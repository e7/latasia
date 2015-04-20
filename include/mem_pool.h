#ifndef __LATASIA__MEM_POOL_H__
#define __LATASIA__MEM_POOL_H__


#include "common.h"


typedef struct lts_pool_s lts_pool_t;
typedef struct lts_pool_data_s lts_pool_data_t;
typedef struct lts_pool_large_s lts_pool_large_t;


extern lts_pool_t *lts_create_pool(size_t size);
extern void *lts_palloc(lts_pool_t *pool, size_t size);
extern void lts_destroy_pool(lts_pool_t* pool);
#endif // __LATASIA__MEM_POOL_H__
