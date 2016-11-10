#include "socket.h"
#include "rbt_timer.h"

#define __THIS_FILE__       "test/test_rbt_timer.c"


int main(void)
{
    (void)fprintf(stderr, "%s\n", SRC2STR(PROJECT_ROOT));

    lts_timer_t rt = {0, RB_ROOT};
    lts_socket_t s[1000];
    int count = ARRAY_COUNT(s);

    for (int i = 0; i < count; ++i) {
        lts_init_socket(&s[i]);
        s[i].timer_node.mapnode.key = i + 10;
    }
    for (int i = 0; i < count; ++i) {
        lts_timer_add(&rt, &s[i].timer_node);
    }
    for (int i = count - 1; i >= 0; --i) {
        lts_timer_del(&rt, &s[i].timer_node);
    }

    return 0;
}
