#include <sys/types.h>
#include <sys/mman.h>
#include "shmem.h"


int lts_shm_alloc(lts_shm_t *shm)
{
    shm->addr = (uint8_t *)mmap(NULL, shm->size,
                                PROT_READ | PROT_WRITE,
                                MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    if (MAP_FAILED == shm->addr) {
        return LTS_E_SYS;
    }

    return LTS_E_OK;
}


void lts_shm_free(lts_shm_t *shm)
{
    if (-1 == munmap(shm->addr, shm->size)) {
    }

    return;
}
