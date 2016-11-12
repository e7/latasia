#include <stdio.h>
#include <assert.h>
#include "rbmap.h"


typedef struct {
    int val;
    lts_rbmap_node_t rbm_nd;
} test_t;

int main(void)
{
    lts_rbmap_t rbm = lts_rbmap_entity;
    lts_rbmap_node_t *it;

    test_t t1, *pt1;
    t1.val = 11;
    lts_rbmap_node_init(&t1.rbm_nd, 1);

    lts_rbmap_add(&rbm, &t1.rbm_nd);
    it = lts_rbmap_get(&rbm, 1);
    pt1 = CONTAINER_OF(it, test_t, rbm_nd);
    assert(&t1 == pt1);

    test_t t2;
    lts_rbmap_node_init(&t2.rbm_nd, 2);
    lts_rbmap_add(&rbm, &t2.rbm_nd);

    test_t t3;
    lts_rbmap_node_init(&t3.rbm_nd, 3);
    lts_rbmap_add(&rbm, &t3.rbm_nd);

    lts_rbmap_del(&rbm, 1);
    it = lts_rbmap_get(&rbm, 1);
    assert(NULL == it);
    lts_rbmap_del(&rbm, 2);
    lts_rbmap_del(&rbm, 3);

    assert(NULL == lts_rbmap_min(&rbm));

    return 0;
}
