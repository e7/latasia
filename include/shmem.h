#ifndef __EVIEO__SHMEM_H__
#define __EVIEO__SHMEM_H__


#include "common.h"


typedef struct {
    uint8_t *addr;
    size_t size;
} lts_shm_t;


extern int lts_shm_alloc(lts_shm_t *shm);
extern void lts_shm_free(lts_shm_t *shm);
#endif // __EVIEO__SHMEM_H__
