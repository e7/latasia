#include <assert.h>

#include "simple_json.h"
#include "extra_errno.h"


size_t lts_sys_pagesize = 4096;


void dump_string(lts_str_t *str)
{
    (void)fputc('"', stderr);
    for (int i = 0; i < str->len; ++i) {
        (void)fputc(str->data[i], stderr);
    }
    (void)fputc('"', stderr);

    return;
}


void dump_sjson(lts_sjson_t *output)
{
    lts_sjson_obj_node_t *it;

    (void)fputc('{', stderr);
    while ((it = lts_sjson_pop_min(output))) {
        if (STRING_VALUE == it->node_type) {
            lts_sjson_kv_t *kv = CONTAINER_OF(it, lts_sjson_kv_t, _obj_node);
            dump_string(&it->key);
            fprintf(stderr, ":");
            dump_string(&kv->val);
        } else if (LIST_VALUE == it->node_type) {
            lts_sjson_list_t *lv = CONTAINER_OF(
                it, lts_sjson_list_t, _obj_node
            );
            dump_string(&it->key);
            (void)fprintf(stderr, ":[");
            while (! list_is_empty(&lv->val)) {
                list_t *l_it = list_first(&lv->val);
                lts_sjson_li_node_t *li_node = CONTAINER_OF(
                    l_it, lts_sjson_li_node_t, node
                );
                list_rm_node(&lv->val, l_it);
                dump_string(&li_node->val);
                (void)fprintf(stderr, ",");
            }
            (void)fprintf(stderr, "]");
        } else if (OBJ_VALUE == it->node_type) {
            lts_sjson_t *ov = CONTAINER_OF(
                it, lts_sjson_t, _obj_node
            );
            dump_string(&it->key);
            (void)fprintf(stderr, ":");
            dump_sjson(ov);
        } else {
            abort();
        }
        (void)fprintf(stderr, ",");
    }
    (void)fprintf(stderr, "}\n");

    return;
}


void test_t1(lts_pool_t *pool)
{
    lts_str_t s;
    uint8_t t1[] = "{}";
    lts_sjson_t output = lts_empty_sjson(pool);

    s = (lts_str_t)lts_string(t1);
    assert(0 == lts_sjson_decode(&s, &output));
    (void)fprintf(stderr, "\n======== t1\n");
    (void)fprintf(stderr, "orig: %s\n", t1);
    (void)fprintf(stderr, "encode size: %ld\n", lts_sjson_encode_size(&output));
    assert(0 == lts_sjson_encode(&output, &s));
    for (int i = 0; i < s.len; ++i) {
        (void)fputc(s.data[i], stderr);
    }
    (void)fputc('\n', stderr);
    (void)fprintf(stderr, "len:%ld\n", s.len);
    dump_sjson(&output);
}


void test_t2(lts_pool_t *pool)
{
    lts_str_t s;
    uint8_t t2[] = "\t {}";
    lts_sjson_t output = lts_empty_sjson(pool);

    s = (lts_str_t)lts_string(t2);
    assert(0 == lts_sjson_decode(&s, &output));
    (void)fprintf(stderr, "\n======== t2\n");
    (void)fprintf(stderr, "orig: %s\n", t2);
    (void)fprintf(stderr, "encode size: %ld\n", lts_sjson_encode_size(&output));
    assert(0 == lts_sjson_encode(&output, &s));
    for (int i = 0; i < s.len; ++i) {
        (void)fputc(s.data[i], stderr);
    }
    (void)fputc('\n', stderr);
    (void)fprintf(stderr, "len:%ld\n", s.len);
    dump_sjson(&output);
}


void test_t3(lts_pool_t *pool)
{
    lts_str_t s;
    uint8_t t3[] = "\ta{}";
    lts_sjson_t output = lts_empty_sjson(pool);

    s = (lts_str_t)lts_string(t3);
    assert(-1 == lts_sjson_decode(&s, &output));
}


void test_t4(lts_pool_t *pool)
{
    lts_str_t s;
    lts_sjson_t output = lts_empty_sjson(pool);
    uint8_t t4[] = "{\"a\":\"b\"}";

    s = (lts_str_t)lts_string(t4);
    assert(0 == lts_sjson_decode(&s, &output));
    (void)fprintf(stderr, "\n======== t4\n");
    (void)fprintf(stderr, "orig: %s\n", t4);
    (void)fprintf(stderr, "encode size: %ld\n", lts_sjson_encode_size(&output));
    assert(0 == lts_sjson_encode(&output, &s));
    for (int i = 0; i < s.len; ++i) {
        (void)fputc(s.data[i], stderr);
    }
    (void)fputc('\n', stderr);
    (void)fprintf(stderr, "len:%ld\n", s.len);
    dump_sjson(&output);
}


void test_t5(lts_pool_t *pool)
{
    lts_str_t s;
    lts_sjson_t output = lts_empty_sjson(pool);
    uint8_t t5[] = "{\"a\":\"b\", }";

    s = (lts_str_t)lts_string(t5);
    assert(0 == lts_sjson_decode(&s, &output));
    (void)fprintf(stderr, "\n======== t5\n");
    (void)fprintf(stderr, "orig: %s\n", t5);
    (void)fprintf(stderr, "encode size: %ld\n", lts_sjson_encode_size(&output));
    assert(0 == lts_sjson_encode(&output, &s));
    for (int i = 0; i < s.len; ++i) {
        (void)fputc(s.data[i], stderr);
    }
    (void)fputc('\n', stderr);
    (void)fprintf(stderr, "len:%ld\n", s.len);
    dump_sjson(&output);
}


void test_t6(lts_pool_t *pool)
{
    lts_str_t s;
    lts_sjson_t output = lts_empty_sjson(pool);
    uint8_t t6[] = "{\"a\":\"b\", \"c\":\"d\"}";

    s = (lts_str_t)lts_string(t6);
    assert(0 == lts_sjson_decode(&s, &output));
    (void)fprintf(stderr, "\n======== t6\n");
    (void)fprintf(stderr, "orig: %s\n", t6);
    (void)fprintf(stderr, "encode size: %ld\n", lts_sjson_encode_size(&output));
    assert(0 == lts_sjson_encode(&output, &s));
    for (int i = 0; i < s.len; ++i) {
        (void)fputc(s.data[i], stderr);
    }
    (void)fputc('\n', stderr);
    (void)fprintf(stderr, "len:%ld\n", s.len);
    dump_sjson(&output);
}


void test_t7(lts_pool_t *pool)
{
    lts_str_t s;
    lts_sjson_t output = lts_empty_sjson(pool);
    // uint8_t t7[] = "{\"a\":[\"b\", \"c\"]}";
    uint8_t t7[] = "{\"a\":[]}";

    s = (lts_str_t)lts_string(t7);
    assert(0 == lts_sjson_decode(&s, &output));
    (void)fprintf(stderr, "\n======== t7\n");
    (void)fprintf(stderr, "orig: %s\n", t7);
    (void)fprintf(stderr, "encode size: %ld\n", lts_sjson_encode_size(&output));
    assert(0 == lts_sjson_encode(&output, &s));
    for (int i = 0; i < s.len; ++i) {
        (void)fputc(s.data[i], stderr);
    }
    (void)fputc('\n', stderr);
    (void)fprintf(stderr, "len:%ld\n", s.len);
    dump_sjson(&output);
}


void test_t8(lts_pool_t *pool)
{
    lts_str_t s;
    lts_sjson_t output = lts_empty_sjson(pool);
    uint8_t t8[] = "{\"a\":[\"b\":\"c\", \"d\":\"e\"]}";

    s = (lts_str_t)lts_string(t8);
    (void)fprintf(stderr, "%s\n", t8);
    assert(-1 == lts_sjson_decode(&s, &output));
}


void test_t9(lts_pool_t *pool)
{
    lts_str_t s;
    lts_sjson_t output = lts_empty_sjson(pool);
    uint8_t t9[] = "{}{}";

    s = (lts_str_t)lts_string(t9);
    (void)fprintf(stderr, "%s\n", t9);
    assert(-1 == lts_sjson_decode(&s, &output));
}


void test_t10(lts_pool_t *pool)
{
    lts_str_t s;
    lts_sjson_t output = lts_empty_sjson(pool);
    uint8_t t10[] = "{\"a\":[\"b\", \"c\"], \"d\":\"e\"}";

    s = (lts_str_t)lts_string(t10);
    assert(0 == lts_sjson_decode(&s, &output));
    (void)fprintf(stderr, "\n======== t10\n");
    (void)fprintf(stderr, "orig: %s\n", t10);
    (void)fprintf(stderr, "encode size: %ld\n", lts_sjson_encode_size(&output));
    assert(0 == lts_sjson_encode(&output, &s));
    for (int i = 0; i < s.len; ++i) {
        (void)fputc(s.data[i], stderr);
    }
    (void)fputc('\n', stderr);
    (void)fprintf(stderr, "len:%ld\n", s.len);
    dump_sjson(&output);
}


void test_t11(lts_pool_t *pool)
{
    lts_str_t s;
    lts_sjson_t output = lts_empty_sjson(pool);
    uint8_t t11[] = "{\"a\":{\"b\": \"c\"}}";

    s = (lts_str_t)lts_string(t11);
    assert(0 == lts_sjson_decode(&s, &output));
    (void)fprintf(stderr, "\n======== t11\n");
    (void)fprintf(stderr, "orig: %s\n", t11);
    (void)fprintf(stderr, "encode size: %ld\n", lts_sjson_encode_size(&output));
    assert(0 == lts_sjson_encode(&output, &s));
    for (int i = 0; i < s.len; ++i) {
        (void)fputc(s.data[i], stderr);
    }
    (void)fputc('\n', stderr);
    (void)fprintf(stderr, "len:%ld\n", s.len);
    dump_sjson(&output);
}


void test_t12(lts_pool_t *pool)
{
    lts_str_t s;
    lts_sjson_t output = lts_empty_sjson(pool);
    uint8_t t12[] = "{\"a\":{\"b\": {\"c\":\"d\"}}, \"e\":{\"f\":\"g\"}}";

    s = (lts_str_t)lts_string(t12);
    assert(0 == lts_sjson_decode(&s, &output));
    (void)fprintf(stderr, "\n======== t12\n");
    (void)fprintf(stderr, "orig: %s\n", t12);
    (void)fprintf(stderr, "encode size: %ld\n", lts_sjson_encode_size(&output));
    assert(0 == lts_sjson_encode(&output, &s));
    for (int i = 0; i < s.len; ++i) {
        (void)fputc(s.data[i], stderr);
    }
    (void)fputc('\n', stderr);
    (void)fprintf(stderr, "len:%ld\n", s.len);
    dump_sjson(&output);
}


void test_t13(lts_pool_t *pool)
{
    lts_str_t s;
    lts_sjson_t output = lts_empty_sjson(pool);
    uint8_t t13[] = "{\"a\":{}}";

    s = (lts_str_t)lts_string(t13);
    assert(0 == lts_sjson_decode(&s, &output));
    (void)fprintf(stderr, "\n======== t13\n");
    (void)fprintf(stderr, "orig: %s\n", t13);
    (void)fprintf(stderr, "encode size: %ld\n", lts_sjson_encode_size(&output));
    assert(0 == lts_sjson_encode(&output, &s));
    for (int i = 0; i < s.len; ++i) {
        (void)fputc(s.data[i], stderr);
    }
    (void)fputc('\n', stderr);
    (void)fprintf(stderr, "len:%ld\n", s.len);
    dump_sjson(&output);
}


int main(void)
{
    lts_pool_t *pool;
    lts_sjson_t output;

    pool = lts_create_pool(256);
    if (NULL == pool) {
        fprintf(stderr, "errno:%d\n", errno);
        return EXIT_FAILURE;
    }

    output = lts_empty_sjson(pool);

    lts_str_t key1 = lts_string("lalala");
    lts_str_t k2 = lts_string("key2"), v2 = lts_string("v2");
    lts_sjson_add_kv2(lts_sjson_add_sjson2(&output, &key1), &k2, &v2);
    dump_sjson(&output);

    test_t1(pool);
    test_t2(pool);
    test_t3(pool);
    test_t4(pool);
    test_t5(pool);
    test_t6(pool);
    test_t7(pool);
    test_t8(pool);
    test_t9(pool);
    test_t10(pool);
    test_t11(pool);
    test_t12(pool);
    test_t13(pool);

    lts_destroy_pool(pool);

    return 0;
}
