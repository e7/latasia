/**
 * latasia
 * Copyright (c) 2015 e7 <jackzxty@126.com>
 * */


#ifndef __LATASIA__PROTOCOL_SJSONB_H__
#define __LATASIA__PROTOCOL_SJSONB_H__


#include "buffer.h"
#include "simple_json.h"


#ifdef __cplusplus
extern "C" {
#endif

extern ssize_t lts_proto_naked_encode_size(lts_buffer_t *content);
extern int lts_proto_naked_encode(
    lts_buffer_t *content, uint16_t proto_type, lts_buffer_t *dst
);
extern lts_buffer_t *lts_proto_naked_decode(
    lts_buffer_t *buf, uint16_t *proto_type
);

extern int lts_proto_sjsonb_encode(lts_sjson_t *sjson,
                                   lts_buffer_t *buf, lts_pool_t *pool);
extern
lts_sjson_t *lts_proto_sjsonb_decode(lts_buffer_t *buf, lts_pool_t *pool);

#ifdef __cplusplus
}
#endif

#endif // __LATASIA__PROTOCOL_SJSONB_H__
