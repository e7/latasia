#include "simple_json.h"


int main(void)
{
    lts_str_t s;
    uint8_t t1[] = "{}";
    uint8_t t2[] = "\t {}";
    uint8_t t3[] = "\ta{}";
    uint8_t t4[] = "{\"a\":\"b\"}";

    s = (lts_str_t)lts_string(t1);
    assert(0 == lts_sjon_decode(&s, NULL));

    s = (lts_str_t)lts_string(t2);
    assert(0 == lts_sjon_decode(&s, NULL));

    s = (lts_str_t)lts_string(t3);
    assert(-1 == lts_sjon_decode(&s, NULL));

    s = (lts_str_t)lts_string(t4);
    assert(0 == lts_sjon_decode(&s, NULL));

    return 0;
}
