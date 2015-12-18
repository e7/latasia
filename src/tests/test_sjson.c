#include "simple_json.h"
#include "extra_errno.h"


size_t lts_sys_pagesize = 4096;


static lts_sjson_obj_node_t *lts_sjson_pop_min(lts_rb_root_t *root)
{
    lts_rb_node_t *p;
    lts_sjson_obj_node_t *obj_node;

    p = rb_first(root);
    if (NULL == p) {
        return NULL;
    }

    obj_node = CONTAINER_OF(p, lts_sjson_obj_node_t, rb_node);
    rb_erase(p, root);

    return obj_node;
}


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
    while ((it = lts_sjson_pop_min(&output->val))) {
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

    pool = lts_create_pool(256);
    if (NULL == pool) {
        fprintf(stderr, "errno:%d\n", errno);
        return EXIT_FAILURE;
    }

    s = lts_string(t1);
    output = lts_empty_json;
    assert(0 == lts_sjson_decode(&s, pool, &output));
    (void)fprintf(stderr, "======== t1\n");
    (void)fprintf(stderr, "size: %ld\n", lts_sjson_encode_size(&output));
    assert(0 == lts_sjson_encode(&output, pool, &s));
    for (int i = 0; i < s.len; ++i) {
        (void)fputc(s.data[i], stderr);
    }
    (void)fputc('\n', stderr);
    dump_sjson(&output);

    s = lts_string(t2);
    output = lts_empty_json;
    assert(0 == lts_sjson_decode(&s, pool, &output));
    (void)fprintf(stderr, "======== t2\n");
    (void)fprintf(stderr, "size: %ld\n", lts_sjson_encode_size(&output));
    assert(0 == lts_sjson_encode(&output, pool, &s));
    for (int i = 0; i < s.len; ++i) {
        (void)fputc(s.data[i], stderr);
    }
    (void)fputc('\n', stderr);
    dump_sjson(&output);

    s = lts_string(t3);
    output = lts_empty_json;
    assert(-1 == lts_sjson_decode(&s, pool, &output));

    s = lts_string(t4);
    output = lts_empty_json;
    assert(0 == lts_sjson_decode(&s, pool, &output));
    (void)fprintf(stderr, "======== t4\n");
    (void)fprintf(stderr, "size: %ld\n", lts_sjson_encode_size(&output));
    assert(0 == lts_sjson_encode(&output, pool, &s));
    for (int i = 0; i < s.len; ++i) {
        (void)fputc(s.data[i], stderr);
    }
    (void)fputc('\n', stderr);
    dump_sjson(&output);

    s = lts_string(t5);
    output = lts_empty_json;
    assert(-1 == lts_sjson_decode(&s, pool, &output));

    s = lts_string(t6);
    output = lts_empty_json;
    assert(0 == lts_sjson_decode(&s, pool, &output));
    (void)fprintf(stderr, "======== t6\n");
    (void)fprintf(stderr, "size: %ld\n", lts_sjson_encode_size(&output));
    assert(0 == lts_sjson_encode(&output, pool, &s));
    for (int i = 0; i < s.len; ++i) {
        (void)fputc(s.data[i], stderr);
    }
    (void)fputc('\n', stderr);
    dump_sjson(&output);

    s = lts_string(t7);
    output = lts_empty_json;
    assert(0 == lts_sjson_decode(&s, pool, &output));
    (void)fprintf(stderr, "======== t7\n");
    (void)fprintf(stderr, "size: %ld\n", lts_sjson_encode_size(&output));
    assert(0 == lts_sjson_encode(&output, pool, &s));
    for (int i = 0; i < s.len; ++i) {
        (void)fputc(s.data[i], stderr);
    }
    (void)fputc('\n', stderr);
    dump_sjson(&output);

    s = lts_string(t8);
    output = lts_empty_json;
    assert(-1 == lts_sjson_decode(&s, pool, &output));

    s = lts_string(t9);
    output = lts_empty_json;
    assert(-1 == lts_sjson_decode(&s, pool, &output));

    s = lts_string(t10);
    output = lts_empty_json;
    assert(0 == lts_sjson_decode(&s, pool, &output));
    (void)fprintf(stderr, "======== t10\n");
    (void)fprintf(stderr, "size: %ld\n", lts_sjson_encode_size(&output));
    assert(0 == lts_sjson_encode(&output, pool, &s));
    for (int i = 0; i < s.len; ++i) {
        (void)fputc(s.data[i], stderr);
    }
    (void)fputc('\n', stderr);
    dump_sjson(&output);

    s = lts_string(t11);
    output = lts_empty_json;
    assert(0 == lts_sjson_decode(&s, pool, &output));
    (void)fprintf(stderr, "======== t11\n");
    (void)fprintf(stderr, "size: %ld\n", lts_sjson_encode_size(&output));
    assert(0 == lts_sjson_encode(&output, pool, &s));
    for (int i = 0; i < s.len; ++i) {
        (void)fputc(s.data[i], stderr);
    }
    (void)fputc('\n', stderr);
    dump_sjson(&output);

    s = lts_string(t12);
    output = lts_empty_json;
    assert(0 == lts_sjson_decode(&s, pool, &output));
    (void)fprintf(stderr, "======== t12\n");
    (void)fprintf(stderr, "size: %ld\n", lts_sjson_encode_size(&output));
    assert(0 == lts_sjson_encode(&output, pool, &s));
    for (int i = 0; i < s.len; ++i) {
        (void)fputc(s.data[i], stderr);
    }
    (void)fputc('\n', stderr);
    dump_sjson(&output);

    s = lts_string(t13);
    output = lts_empty_json;
    assert(0 == lts_sjson_decode(&s, pool, &output));
    (void)fprintf(stderr, "======== t13\n");
    (void)fprintf(stderr, "size: %ld\n", lts_sjson_encode_size(&output));
    assert(0 == lts_sjson_encode(&output, pool, &s));
    for (int i = 0; i < s.len; ++i) {
        (void)fputc(s.data[i], stderr);
    }
    (void)fputc('\n', stderr);
    dump_sjson(&output);

    lts_destroy_pool(pool);

    return 0;
}
