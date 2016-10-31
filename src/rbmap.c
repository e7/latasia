#include "rbmap.h"


static lts_rbmap_node_t *__rbmap_search(lts_rb_root_t *root, uintptr_t key)
{
    lts_rbmap_node_t *s;
    lts_rb_node_t *iter = root->rb_node;

    while (iter) {
        s = rb_entry(iter, lts_rbmap_node_t, rbnode);

        if (key < s->key) {
            iter = iter->rb_left;
        } else if (key > s->key) {
            iter = iter->rb_right;
        } else {
            return s;
        }
    }

    return NULL;
}


static int __rbmap_link(lts_rb_root_t *root, lts_rbmap_node_t *node)
{
    lts_rbmap_node_t *s;
    lts_rb_node_t *parent, **iter;

    parent = NULL;
    iter = &root->rb_node;
    while (*iter) {
        parent = *iter;
        s = rb_entry(parent, lts_rbmap_node_t, rbnode);

        if (node->key < s->key) {
            iter = &(parent->rb_left);
        } else if (node->key > s->key) {
            iter = &(parent->rb_right);
        } else {
            return -1;
        }
    }

    rb_link_node(&node->rbnode, parent, iter);
    rb_insert_color(&node->rbnode, root);

    return 0;
}


int lts_rbmap_add(lts_rbmap_t *rbmap, lts_rbmap_node_t *node)
{
    if (! RB_EMPTY_NODE(&node->rbnode)) {
        return -1;
    }

    if (-1 == __rbmap_link(&rbmap->root, node)) {
        return -1;
    }

    ++rbmap->nsize;

    return 0;
}


lts_rbmap_node_t *lts_rbmap_del(lts_rbmap_t *rbmap, uintptr_t key)
{
    lts_rbmap_node_t *key_node = __rbmap_search(&rbmap->root, key);

    if (key_node) {
        rb_erase(&key_node->rbnode, &rbmap->root);
        RB_CLEAR_NODE(&key_node->rbnode);
        --rbmap->nsize;

        return key_node;
    }

    return NULL;
}


lts_rbmap_node_t *lts_rbmap_get(lts_rbmap_t *rbmap, uintptr_t key)
{
    return __rbmap_search(&rbmap->root, key);
}
