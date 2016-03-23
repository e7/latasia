#include "adv_string.h"


int main(void)
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

    return 0;
}
