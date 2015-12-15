#include "simple_json.h"
#include "extra_errno.h"


size_t lts_sys_pagesize = 4096;


int main(void)
{
    lts_str_t s;
    lts_pool_t *pool;
    lts_sjson_t output;
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
    uint8_t t11[] = "{\"a\":{\"b\": \"c\"}}";
    uint8_t t12[] = "{\"a\":{\"b\": {\"c\":\"d\"}}}";
    uint8_t t13[] = "{\"a\":{}}";

    pool = lts_create_pool(40960);
    if (NULL == pool) {
        fprintf(stderr, "errno:%d\n", errno);
        return EXIT_FAILURE;
    }

    s = (lts_str_t)lts_string(t1);
    assert(0 == lts_sjon_decode(&s, pool, &output));

    s = (lts_str_t)lts_string(t2);
    assert(0 == lts_sjon_decode(&s, pool, &output));

    s = (lts_str_t)lts_string(t3);
    assert(-1 == lts_sjon_decode(&s, pool, &output));

    s = (lts_str_t)lts_string(t4);
    assert(0 == lts_sjon_decode(&s, pool, &output));

    s = (lts_str_t)lts_string(t5);
    assert(-1 == lts_sjon_decode(&s, pool, &output));

    s = (lts_str_t)lts_string(t6);
    assert(0 == lts_sjon_decode(&s, pool, &output));

    s = (lts_str_t)lts_string(t7);
    assert(0 == lts_sjon_decode(&s, pool, &output));

    s = (lts_str_t)lts_string(t8);
    assert(-1 == lts_sjon_decode(&s, pool, &output));

    s = (lts_str_t)lts_string(t9);
    assert(-1 == lts_sjon_decode(&s, pool, &output));

    s = (lts_str_t)lts_string(t10);
    assert(0 == lts_sjon_decode(&s, pool, &output));

    s = (lts_str_t)lts_string(t11);
    assert(0 == lts_sjon_decode(&s, pool, &output));

    s = (lts_str_t)lts_string(t12);
    assert(0 == lts_sjon_decode(&s, pool, &output));

    s = (lts_str_t)lts_string(t13);
    assert(0 == lts_sjon_decode(&s, pool, &output));

    lts_destroy_pool(pool);

    return 0;
}
