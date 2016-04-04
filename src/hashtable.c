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
