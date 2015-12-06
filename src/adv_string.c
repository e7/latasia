/**
 * latasia
 * Copyright (c) 2015 e7 <jackzxty@126.com>
 * */


#include "adv_string.h"

#define __THIS_FILE__       "src/adv_string.c"


// 区间反转，i和j分别为起始和结束下标
static void reverse_region(uint8_t *data, int i, int j)
{
    uint8_t c;

    while (i < j) {
        c = data[i];
        data[i] = data[j];
        data[j] = c;
        ++i;
        --j;
    }
}

static void kmp_next(lts_str_t *str, int *next, size_t sz)
{
    int i = 0;
    int j = 1;

    assert(sz >= str->len);

    next[i] = -1;
    next[j] = 0;
    while (j < (int)str->len - 1) {
        if ((str->data[i] == str->data[j])) {
            next[j + 1] = i + 1;
            ++i;
            ++j;
        } else {
            i = next[i];
            if (-1 == i) {
                i = 0;
                next[++j] = 0;
            }
        }
    }
}


void lts_str_trim(lts_str_t *str)
{
    // head
    while (str->len) {
        uint8_t c = *(str->data);

        if ((' ' != c) && ('\t' != c) && ('\r' != c)) {
            break;
        }
        ++(str->data);
        --(str->len);
    }

    // tail
    while (str->len) {
        uint8_t c = str->data[str->len - 1];

        if ((' ' != c) && ('\t' != c) && ('\r' != c)) {
            break;
        }
        --(str->len);
    }

    return;
}


/*
int lts_str_find(lts_str_t *text, lts_str_t *pattern)
{
    int rslt;
    size_t next_sz = pattern->len * sizeof(int);
    int *next = (int *)malloc(next_sz);

    if (NULL == next) {
        return;
    }

    kmp_next(pattern, next, next_sz);

    do {
        int i = 0;
        int j = 0;

        while ((i < (int)text->len) && (j < (int)pattern->len)) {
            if ((-1 == j) || (text->data[i] == pattern->data[j])) {
                ++i;
                ++j;
            } else {
                j = next[j];
            }
        }

        if (j >= (int)pattern->len) {
            rslt = i - pattern->len;
        } else {
            rslt = -1;
        }
    } while (0);

    free(next);

    return rslt;
}
*/
int lts_str_find(lts_str_t *text, lts_str_t *pattern, int offset)
{
    int rslt;
    size_t next_sz = pattern->len * sizeof(int);
    int *next = (int *)malloc(next_sz);
    lts_str_t region;

    if (offset < 0) {
        abort();
    }

    region.data = text->data + offset;
    region.len = text->len - offset;
    kmp_next(pattern, next, next_sz);

    do {
        int i = 0;
        int j = 0;

        while ((i < (int)region.len) && (j < (int)pattern->len)) {
            if ((-1 == j) || (region.data[i] == pattern->data[j])) {
                ++i;
                ++j;
            } else {
                j = next[j];
            }
        }

        if (j >= (int)pattern->len) {
            rslt = i - pattern->len;
        } else {
            rslt = -1;
        }
    } while (0);

    free(next);

    return rslt;
}


int lts_str_compare(lts_str_t *a, lts_str_t *b)
{
    int rslt = 0;
    size_t i, cmp_len = MIN(a->len, b->len);

    for (i = 0; i < cmp_len; ++i) {
        rslt = a->data[i] - b->data[i];
        if (rslt) {
            break;
        }
    }

    if (rslt || a->len == b->len) {
        return rslt;
    }

    // 未分胜负
    if (a->len > b->len) {
        rslt = a->data[i];
    } else {
        rslt = -b->data[i];
    }

    return rslt;
}

void lts_str_reverse(lts_str_t *src)
{
    reverse_region(src->data, 0, src->len - 1);
}


size_t lts_str_filter(lts_str_t *src, uint8_t c)
{
    size_t i, m, j;
    size_t c_count;

    // 计算c_count，即字符c的个数
    c_count = 0;
    for (i = 0; i < src->len; ++i) {
        if (c == src->data[i]) {
            ++c_count;
        }
    }
    if (0 == c_count) {
        return 0;
    }

    // 初始化i，即第一个c的位置
    for (i = 0; i < src->len; ++i) {
        if (c == src->data[i]) {
            break;
        }
    }

    while (TRUE) {
        for (m = i; m < src->len; ++m) {
            if (c != src->data[m]) {
                break;
            }
        }
        if (m == src->len) {
            size_t tmp_len = src->len - c_count;

            src->data[tmp_len] = 0;
            src->len = tmp_len;
            break;
        }

        // m >= i
        for (j = m; j < src->len; ++j) {
            if (c == src->data[j]) {
                --j;
                break;
            }
        }

        reverse_region(src->data, m, j);
        reverse_region(src->data, i, j);

        i += j - m + 1;
    }

    return c_count;
}


static size_t long_width(long x)
{
    size_t rslt = ((x < 0) ? 1 : 0);

    do {
        ++rslt;
        x /= 10;
    } while (x);

    return rslt;
}

int lts_l2str(lts_str_t *str, long x)
{
    size_t last = 0;
    long absx = labs(x);
    uint32_t oct_bit;

    if ((str->len + 1) < long_width(x)) {
        return -1;
    }

    do {
        oct_bit = absx % 10;
        absx /= 10;
        str->data[last++] = '0' + oct_bit;
    } while (absx);
    if (x < 0) {
        str->data[last++] = '-';
    }
    reverse_region(str->data, 0, last - 1);
    str->data[last] = 0;
    str->len = last;

    return 0;
}
