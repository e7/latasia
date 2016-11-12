#include "hashtable.h"


void lts_init_hashtable(lts_hashtable_t *ht)
{
    for (int i = 0; i < ht->size; ++i) {
        ht->table[i].first = NULL;
    }

    return;
}


lts_hashtable_t *lts_create_hashtable(lts_pool_t *pool, ssize_t nbucket)
{
    lts_hashtable_t *ht;

    if (nbucket < 2) {
        return NULL;
    }

    ht = (lts_hashtable_t *)lts_palloc(
        pool, sizeof(lts_hashtable_t) * nbucket
    );
    lts_init_hashtable(ht);

    return ht;
}


// 备选哈希函数
uintptr_t time33(void *str, size_t len)
{
    uintptr_t hash = 0;

    for (size_t i = 0; i < len; ++i) {
        hash = hash * 33 + (uintptr_t)((uint8_t *)str)[i];
    }

    return hash;
}


uintptr_t hash_long(uintptr_t val, uintptr_t bits)
{
    uintptr_t hash = val * 0x9e370001UL;

    return hash >> (32 - bits);
}
