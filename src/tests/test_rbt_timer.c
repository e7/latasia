#include "rbt_timer.h"


int main(void)
{
    (void)fprintf(stderr, "%s\n", SRC2STR(PROJECT_ROOT));

    lts_rb_root_t rt = RB_ROOT;
    lts_socket_t s[1000];
    int count = ARRAY_COUNT(s);

    for (int i = 0; i < count; ++i) {
        lts_init_socket(&s[i]);
        s[i].timeout = i + 10;
    }
    for (int i = 0; i < count; ++i) {
        lts_timer_heap_add(&rt, &s[i]);
    }
    for (int i = count - 1; i >= 0; --i) {
        lts_timer_heap_del(&rt, &s[i]);
    }

    return 0;
}
