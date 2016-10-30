#include "rbmap.h"


// link，节点不存在时是否挂到树上
static void *__rbmap_search_and_link(
    lts_rbmap_t *rbmap, void *obj, int link
)
{
    void *pre_obj;
    lts_rb_node_t *parent, **iter;

    parent = NULL;
    iter = &rbmap->root.rb_node;
    while (*iter) {
        parent = *iter;
        pre_obj = (lts_rb_node_t *)(((uint8_t *)parent) + rbmap->neg_ofst);

        if ((*rbmap->hash)(obj) < (*rbmap->hash)(pre_obj)) {
            iter = &(parent->rb_left);
        } else if ((*rbmap->hash)(obj) > (*rbmap->hash)(pre_obj)) {
            iter = &(parent->rb_right);
        } else {
            return pre_obj;
        }
    }

    if (link) {
        rb_link_node((lts_rb_node_t *)(((uint8_t *)obj) - rbmap->neg_ofst),
                     parent, iter);
        rb_insert_color(
            (lts_rb_node_t *)(((uint8_t *)obj) - rbmap->neg_ofst),
            &rbmap->root
        );
    }

    return obj;
}


int lts_rbmap_add(lts_rbmap_t *rbmap, void *obj)
{
    if (! RB_EMPTY_NODE((lts_rb_node_t *)(
            ((uint8_t *)obj) - rbmap->neg_ofst))) {
        return -1;
    }

    if (obj != __rbmap_search_and_link(rbmap, obj, TRUE)) {
        return -1;
    }

    ++rbmap->nsize;

    return 0;
}


void lts_rbmap_del(lts_rbmap_t *rbmap, void *obj)
{
    rb_erase((lts_rb_node_t *)(((uint8_t *)obj) - rbmap->neg_ofst),
             &rbmap->root);
    RB_CLEAR_NODE((lts_rb_node_t *)(((uint8_t *)obj) - rbmap->neg_ofst));
    --rbmap->nsize;

    return;
}


void *lts_rbmap_get(lts_rbmap_t *rbmap, void *obj)
{
    return __rbmap_search_and_link(rbmap, obj, FALSE);
}
