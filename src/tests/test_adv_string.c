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


void split_test(void)
{
#ifdef ADV_STRING_ENHANCE
    extern size_t lts_sys_pagesize;

    lts_str_t **rslt;
    uint8_t s1[] = ",,,,asafdas";
    uint8_t s2[] = "asas,,,,asafdas";
    uint8_t s3[] = ",,asa,fdas";
    uint8_t s4[] = ",,asa,fdas,";
    uint8_t s5[] = "asa,fdas,,";
    uint8_t s6[] = "asa,fd,as";
    lts_str_t ls_s1 = lts_string(s1);
    lts_str_t ls_s2 = lts_string(s2);
    lts_str_t ls_s3 = lts_string(s3);
    lts_str_t ls_s4 = lts_string(s4);
    lts_str_t ls_s5 = lts_string(s5);
    lts_str_t ls_s6 = lts_string(s6);
    lts_pool_t *pool;

    lts_sys_pagesize = 4096;
    pool = lts_create_pool(4096);

    rslt = lts_str_split(&ls_s1, ',', pool);
    fprintf(stderr, "s1===================\n");
    for (int i = 0; rslt[i]; ++i) {
        lts_str_println(stderr, rslt[i]);
    }

    rslt = lts_str_split(&ls_s2, ',', pool);
    fprintf(stderr, "s2===================\n");
    for (int i = 0; rslt[i]; ++i) {
        lts_str_println(stderr, rslt[i]);
    }

    rslt = lts_str_split(&ls_s3, ',', pool);
    fprintf(stderr, "s3===================\n");
    for (int i = 0; rslt[i]; ++i) {
        lts_str_println(stderr, rslt[i]);
    }

    rslt = lts_str_split(&ls_s4, ',', pool);
    fprintf(stderr, "s4===================\n");
    for (int i = 0; rslt[i]; ++i) {
        lts_str_println(stderr, rslt[i]);
    }

    rslt = lts_str_split(&ls_s5, ',', pool);
    fprintf(stderr, "s5===================\n");
    for (int i = 0; rslt[i]; ++i) {
        lts_str_println(stderr, rslt[i]);
    }

    rslt = lts_str_split(&ls_s6, ',', pool);
    fprintf(stderr, "s6===================\n");
    for (int i = 0; rslt[i]; ++i) {
        lts_str_println(stderr, rslt[i]);
    }
#endif
}


int main(void)
{
    common_test();
    find_test();
    split_test();

    return 0;
}
