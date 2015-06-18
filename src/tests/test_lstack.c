#include "list.h"

#define __THIS_FILE__       "test/test_lstack.c"


typedef struct {
    int x;
    lstack_t node;
} my_data_t;


int main(void)
{
    lstack_t *stk;
    int const total = 30;

    (void)fprintf(stderr, "%s\n", SRC2STR(PROJECT_ROOT));

    lstack_set_empty(&stk);
    for (int i = 0; i < total; ++i) {
        my_data_t *d = (my_data_t *)malloc(sizeof(my_data_t));
        d->x = i + 1;
        lstack_push(&stk, &d->node);
    }

    while (! lstack_is_empty(&stk)) {
        my_data_t *d = CONTAINER_OF(lstack_top(&stk), my_data_t, node);
        fprintf(stderr, "%d\n", d->x);
        lstack_pop(&stk);
        free(d);
    }

    return 0;
}
