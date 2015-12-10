/**
 * latasia
 * Copyright (c) 2015 e7 <jackzxty@126.com>
 * */


#include "simple_json.h"


// 状态机
enum {
    SJSON_EXP_START = 1,
    SJSON_EXP_K_QUOT_START_OR_END,
    SJSON_EXP_K_QUOT_START,
    SJSON_EXP_K_QUOT_END,
    SJSON_EXP_V_QUOT_OR_BRACKET_START,
    SJSON_EXP_V_QUOT_START,
    SJSON_EXP_V_QUOT_END,
    SJSON_EXP_V_BRACKET_END,
    SJSON_EXP_ARRAY_START,
    SJSON_EXP_ARRAY_END,
    SJSON_EXP_COLON,
    SJSON_EXP_COMMA,
    SJSON_EXP_COMMA_OR_END,
    SJSON_EXP_END,
    SJSON_EXP_NOTHING,
};


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
    static const uint8_t invisible[] = {'\t', '\n', '\r', '\x20'};

    int in_bracket = FALSE;
    int current_stat = SJSON_EXP_START;

    // 过滤不可见字符
    (void)lts_str_filter_multi(src, invisible, ARRAY_COUNT(invisible));

    for (size_t i = 0; i < src->len; ++i) {
        switch (current_stat) {
        case SJSON_EXP_START:
        {
            if ('{' != src->data[i]) {
                return -1;
            }

            current_stat = SJSON_EXP_K_QUOT_START_OR_END;
            continue;
        }

        case SJSON_EXP_K_QUOT_START_OR_END:
        {
            if ('"' == src->data[i]) {
                current_stat = SJSON_EXP_K_QUOT_END;
            } else if ("}" == src->data[i]) {
                current_stat = SJSON_EXP_NOTHING;
            } else {
                NOP;
            }
            continue;
        }

        case SJSON_EXP_K_QUOT_START:
        {
            if ('"' == src->data[i]) {
                current_stat = SJSON_EXP_K_QUOT_END;
            }
            continue;
        }

        case SJSON_EXP_K_QUOT_END:
        {
            if ('"' == src->data[i]) {
                current_stat = SJSON_EXP_COLON;
            }
            continue;
        }

        case SJSON_EXP_COLON:
        {
            if (':' != src->data[i]) {
                return -1;
            }

            current_stat = SJSON_EXP_V_QUOT_OR_BRACKET_START;
            continue;
        }

        case SJSON_EXP_V_QUOT_OR_BRACKET_START:
        {
            if ('"' == src->data[i]) {
                current_stat = SJSON_EXP_V_QUOT_END;
            } else if ('[' == src->data[i]) {
                current_stat = SJSON_EXP_V_BRACKET_END;
            } else {
                return -1;
            }
            continue;
        }

        case SJSON_EXP_V_QUOT_START:
        {
            if ('"' == src->data[i]) {
                current_stat = SJSON_EXP_V_QUOT_END;
            }
            continue;
        }

        case SJSON_EXP_V_QUOT_END:
        {
            if ('"' == src->data[i]) {
                current_stat = SJSON_EXP_COMMA_OR_END;
            }
            continue;
        }

        case SJSON_EXP_V_BRACKET_END:
        {
            if (']' != src->data[i]) {
                return -1;
            }

            current_stat = SJSON_EXP_COMMA_OR_END;
            continue;
        }

        case SJSON_EXP_COMMA_OR_END:
        {
            if (',' == src->data[i]) {
                current_stat = SJSON_EXP_K_QUOT_START;
            } else if ('}' == src->data[i]) {
                current_stat = SJSON_EXP_NOTHING;
            } else {
                return -1;
            }

            continue;
        }

        case SJSON_EXP_NOTHING:
        {
            return -1;
        }

        default:
        {
            abort();
        }}
    }

    return (SJSON_EXP_NOTHING == current_stat) ? 0 : -1;
}
