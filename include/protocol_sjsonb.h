/**
 * latasia
 * Copyright (c) 2015 e7 <jackzxty@126.com>
 * */


#ifndef __LATASIA__PROTOCOL_SJSONB_H__
#define __LATASIA__PROTOCOL_SJSONB_H__


#include "buffer.h"
#include "adv_string.h"


#ifdef __cplusplus
extern "C" {
#endif

extern ssize_t lts_proto_sjsonb_encode_size(lts_str_t *content);
extern void lts_proto_sjsonb_encode(
    lts_str_t *content, uint16_t content_type, lts_buffer_t *dst
);
extern int lts_proto_sjsonb_decode(
    lts_buffer_t *buf, uint16_t *content_type, lts_str_t *content
);

#ifdef __cplusplus
}
#endif

#endif // __LATASIA__PROTOCOL_SJSONB_H__
