#include "hashset.h"


// link，节点不存在时是否挂到树上
static void *__hashset_search_and_link(
    lts_hashset_t *hashset, void *obj, int link
)
{
    void *pre_obj;
    lts_rb_node_t *parent, **iter;

    parent = NULL;
    iter = &hashset->root.rb_node;
    while (*iter) {
        parent = *iter;
        pre_obj = (lts_rb_node_t *)(((uint8_t *)parent) + hashset->neg_ofst);

        if ((*hashset->hash)(obj) < (*hashset->hash)(pre_obj)) {
            iter = &(parent->rb_left);
        } else if ((*hashset->hash)(obj) > (*hashset->hash)(pre_obj)) {
            iter = &(parent->rb_right);
        } else {
            return pre_obj;
        }
    }

    if (link) {
        rb_link_node((lts_rb_node_t *)(((uint8_t *)obj) - hashset->neg_ofst),
                     parent, iter);
        rb_insert_color(
            (lts_rb_node_t *)(((uint8_t *)obj) - hashset->neg_ofst),
            &hashset->root
        );
    }

    return obj;
}


int lts_hashset_add(lts_hashset_t *hashset, void *obj)
{
    if (! RB_EMPTY_NODE((lts_rb_node_t *)(
            ((uint8_t *)obj) - hashset->neg_ofst))) {
        return -1;
    }

    if (obj != __hashset_search_and_link(hashset, obj, TRUE)) {
        return -1;
    }

    return 0;
}


void lts_hashset_del(lts_hashset_t *hashset, void *obj)
{
    rb_erase((lts_rb_node_t *)(((uint8_t *)obj) - hashset->neg_ofst),
             &hashset->root);
    RB_CLEAR_NODE((lts_rb_node_t *)(((uint8_t *)obj) - hashset->neg_ofst));

    return;
}


void *lts_hashset_get(lts_hashset_t *hashset, void *obj)
{
    return __hashset_search_and_link(hashset, obj, FALSE);
}
