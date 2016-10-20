#include "adv_string.h"


void common_test(void)
{
    uint8_t s[] = " abc defg\tasfaf\n";
    uint8_t target[] = " a\t\n";
    lts_str_t str = lts_string(s);

    fprintf(stderr, "[%s]\n", s);

    lts_str_hollow(&str, 1, 2);
    fprintf(stderr, "hollow: [%s]\n", s);

    lts_str_filter(&str, 'f');
    fprintf(stderr, "[%s]\n", s);
    lts_str_filter_multi(&str, target, sizeof(target));
    fprintf(stderr, "[%s]\n", s);

    return;
}


void find_test(void)
{
    uint8_t s[] = "s@#a";
    uint8_t p1[] = "aaaaasfaf32";
    uint8_t p2[] = "@#";

    lts_str_t ls_s = lts_string(s);
    lts_str_t ls_p1 = lts_string(p1);
    lts_str_t ls_p2 = lts_string(p2);

    fprintf(stderr, "find test: %d\n", lts_str_find(&ls_s, &ls_p1, 0));
    fprintf(stderr, "find test: %d\n", lts_str_find(&ls_s, &ls_p2, 0));
}


int main(void)
{
    common_test();
    find_test();

    return 0;
}
