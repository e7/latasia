/**
 * latasia
 * Copyright (c) 2015 e7 <jackzxty@126.com>
 * */


#include "simple_json.h"


// 状态机
enum {
    SJSON_EXP_START = 1,
    SJSON_EXP_K_QUOT_START_OR_END,
    SJSON_EXP_K_QUOT_END,
    SJSON_EXP_COLON,
    SJSON_EXP_V_QUOT_OR_BRACKET_START,
    SJSON_EXP_V_QUOT_START,
    SJSON_EXP_V_QUOT_END,
    SJSON_EXP_COMMA_OR_BRACKET_END,
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

            current_stat = SJSON_EXP_K_QUOT_START_OR_END; // only
            continue;
        }

        case SJSON_EXP_K_QUOT_START_OR_END:
        {
            if ('"' == src->data[i]) {
                current_stat = SJSON_EXP_K_QUOT_END;
            } else if ("}" == src->data[i]) {
                current_stat = SJSON_EXP_NOTHING; // only
            } else {
                return -1;
            }
            continue;
        }

        case SJSON_EXP_K_QUOT_END:
        {
            if ('"' == src->data[i]) {
                current_stat = SJSON_EXP_COLON; // only
            }
            continue;
        }

        case SJSON_EXP_NOTHING:
        {
            return -1;
        }

        case SJSON_EXP_COLON:
        {
            if (':' != src->data[i]) {
                return -1;
            }

            current_stat = SJSON_EXP_V_QUOT_OR_BRACKET_START; // only
            continue;
        }

        case SJSON_EXP_V_QUOT_OR_BRACKET_START:
        {
            if ('"' == src->data[i]) {
                current_stat = SJSON_EXP_V_QUOT_END;
            } else if ('[' == src->data[i]) {
                in_bracket = TRUE;
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
            if (in_bracket) {
                current_stat = SJSON_EXP_COMMA_OR_BRACKET_END; // only
            } else {
                current_stat = SJSON_EXP_COMMA_OR_END; // only
            }
            continue;
        }

        case SJSON_EXP_COMMA_OR_BRACKET_END:
        {
            // 必定在bracket中
            if ('"' == src->data[i]) {
                current_stat = SJSON_EXP_V_QUOT_START;
            } else if (']' == src->data[i]) {
                in_bracket = FALSE;
                current_stat = SJSON_EXP_COMMA_OR_END; // only
            } else {
                return -1;
            }
            continue;
        }

        case SJSON_EXP_COMMA_OR_END:
        {
            // 必定不在bracket中
            if ('"' == src->data[i]) {
                current_stat = SJSON_EXP_V_QUOT_START;
            } else if ('}' == src->data[i]) {
                current_stat = SJSON_EXP_NOTHING;
            } else {
                return -1;
            }
            continue;
        }

        default:
        {
            abort();
        }}
    }

    return (SJSON_EXP_NOTHING == current_stat) ? 0 : -1;
}
