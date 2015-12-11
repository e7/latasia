#include "simple_json.h"


int main(void)
{
    lts_str_t s;
    uint8_t t1[] = "{}";
    uint8_t t2[] = "\t {}";
    uint8_t t3[] = "\ta{}";
    uint8_t t4[] = "{\"a\":\"b\"}";
    uint8_t t5[] = "{\"a\":\"b\", }";
    uint8_t t6[] = "{\"a\":\"b\", \"c\":\"d\"}";
    uint8_t t7[] = "{\"a\":[\"b\", \"c\"]}";
    uint8_t t8[] = "{\"a\":[\"b\":\"c\", \"d\":\"e\"]}";
    uint8_t t9[] = "{}{}";
    uint8_t t10[] = "{\"a\":[\"b\", \"c\"], \"d\":\"e\"}";

    s = (lts_str_t)lts_string(t1);
    assert(0 == lts_sjon_decode(&s, NULL));

    s = (lts_str_t)lts_string(t2);
    assert(0 == lts_sjon_decode(&s, NULL));

    s = (lts_str_t)lts_string(t3);
    assert(-1 == lts_sjon_decode(&s, NULL));

    s = (lts_str_t)lts_string(t4);
    assert(0 == lts_sjon_decode(&s, NULL));

    s = (lts_str_t)lts_string(t5);
    assert(-1 == lts_sjon_decode(&s, NULL));

    s = (lts_str_t)lts_string(t6);
    assert(0 == lts_sjon_decode(&s, NULL));

    s = (lts_str_t)lts_string(t7);
    assert(0 == lts_sjon_decode(&s, NULL));

    s = (lts_str_t)lts_string(t8);
    assert(-1 == lts_sjon_decode(&s, NULL));

    s = (lts_str_t)lts_string(t9);
    assert(-1 == lts_sjon_decode(&s, NULL));

    s = (lts_str_t)lts_string(t10);
    assert(0 == lts_sjon_decode(&s, NULL));

    return 0;
}
