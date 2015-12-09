/**
 * latasia
 * Copyright (c) 2015 e7 <jackzxty@126.com>
 * */


#include "simple_json.h"


ssize_t lts_sjon_encode_size(lts_sjson_t *sjson)
{
    return 0;
}


int lts_sjon_encode(lts_sjson_t *sjson, lts_str_t *output)
{
    return 0;
}


int lts_sjon_decode(lts_str_t *src, lts_sjson_t *output)
{
    uint8_t invisible[] = {'\t', '\n', '\r', '\x20'};

    // 过滤不可见字符
    (void)lts_str_filter_multi(src, invisible, ARRAY_COUNT(invisible));

    return 0;
}
