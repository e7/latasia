#include "obj_pool.h"

#define __THIS_FILE__       "test/test_obj_pool.c"


typedef struct {
    int data;
    dlist_t lnd;
} obj_t;


int main(void)
{
    obj_t *cache, *item;
    lts_obj_pool_t objpool;

    cache = (obj_t *)malloc(sizeof(obj_t) * 3);
    for (int i = 0; i < 3; ++i) {
        cache[i].data = i * 10;
    }

    lts_init_opool(&objpool, cache, sizeof(obj_t) * 3, sizeof(obj_t), sizeof(int));
    item = (obj_t *)lts_op_instance(&objpool);
    lts_op_release(&objpool, item);
    item = (obj_t *)lts_op_instance(&objpool);
    lts_op_release(&objpool, item);
    item = (obj_t *)lts_op_instance(&objpool);
    lts_op_release(&objpool, item);
    item = (obj_t *)lts_op_instance(&objpool);
    fprintf(stderr, "%d\n", item->data);
    free(cache);
    return 0;
}
