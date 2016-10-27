/**
 * latasia
 * Copyright (c) 2015 e7 <jackzxty@126.com>
 * */


#include "adv_string.h"

#define __THIS_FILE__       "src/adv_string.c"


// 区间反转，i和j分别为起始和结束下标
static void __reverse_region(uint8_t *data, int i, int j)
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

static void __kmp_next(lts_str_t *str, int *next, ssize_t sz)
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


char *lts_uint322cstr(uint32_t u32)
{
    static char rslt[16];

    int count = 0;

    if (0 == u32) {
        rslt[0] = '0';
        rslt[1] = 0;
        return rslt;
    }

    while (u32 > 0) {
        uint32_t c = u32 % 10;

        u32 = u32 / 10;
        rslt[count++] = c + 0x30;
    }
    rslt[count] = 0;

    __reverse_region((uint8_t *)rslt, 0, count - 1);

    return rslt;
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


void lts_str_hollow(lts_str_t *src, ssize_t start, ssize_t len)
{
    if ((start + len) > src->len) {
        abort();
    }

    (void)memmove(
        &src->data[start], &src->data[start + len], src->len - start - len
    );
    src->len -= len;
    src->data[src->len] = '\0';

    return;
}


// 子串查询
/*
int lts_str_find(lts_str_t *text, lts_str_t *pattern)
{
    int rslt;
    ssize_t next_sz = pattern->len * sizeof(int);
    int *next = (int *)malloc(next_sz);

    if (NULL == next) {
        return;
    }

    __kmp_next(pattern, next, next_sz);

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
    ssize_t next_sz = pattern->len * sizeof(int);
    int *next = (int *)malloc(next_sz);
    lts_str_t region;

    if (offset < 0) {
        abort();
    }

    region.data = text->data + offset;
    region.len = text->len - offset;
    __kmp_next(pattern, next, next_sz);

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
    ssize_t i, cmp_len = MIN(a->len, b->len);

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
    __reverse_region(src->data, 0, src->len - 1);
}


ssize_t lts_str_filter(lts_str_t *src, uint8_t c)
{
    ssize_t i, m, j;
    ssize_t c_count;

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
            ssize_t tmp_len = src->len - c_count;

            src->data[tmp_len] = 0;
            src->len = tmp_len;
            break;
        }

        // m >= i
        for (j = m; j < src->len; ++j) {
            if (c == src->data[j]) {
                break;
            }
        }
        --j;

        __reverse_region(src->data, m, j);
        __reverse_region(src->data, i, j);

        i += j - m + 1;
    }

    return c_count;
}


ssize_t lts_str_filter_multi(lts_str_t *src, uint8_t *c, ssize_t len)
{
    charmap_t cm;
    ssize_t i, m, j;
    ssize_t c_count;

    charmap_clean(&cm);
    for (ssize_t i = 0; i < len; ++i) {
        charmap_set(&cm, c[i]);
    }

    c_count = 0;
    for (i = 0; i < src->len; ++i) {
        if (charmap_isset(&cm, src->data[i])) {
            ++c_count;
        }
    }
    if (0 == c_count) {
        return 0;
    }

    for (i = 0; i < src->len; ++i) {
        if (charmap_isset(&cm, src->data[i])) {
            break;
        }
    }

    while (TRUE) {
        for (m = i; m < src->len; ++m) {
            if (! charmap_isset(&cm, src->data[m])) {
                break;
            }
        }
        if (m == src->len) {
            ssize_t tmp_len = src->len - c_count;

            src->data[tmp_len] = 0;
            src->len = tmp_len;
            break;
        }

        // m >= i
        for (j = m; j < src->len; ++j) {
            if (charmap_isset(&cm, src->data[j])) {
                break;
            }
        }
        --j;

        __reverse_region(src->data, m, j);
        __reverse_region(src->data, i, j);

        i += j - m + 1;
    }

    return c_count;
}


static ssize_t long_width(long x)
{
    ssize_t rslt = ((x < 0) ? 1 : 0);

    do {
        ++rslt;
        x /= 10;
    } while (x);

    return rslt;
}

int lts_l2str(lts_str_t *str, long x)
{
    ssize_t last = 0;
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
    __reverse_region(str->data, 0, last - 1);
    str->data[last] = 0;
    str->len = last;

    return 0;
}


void lts_str_println(FILE *stream, lts_str_t *s)
{
    for (int i = 0; i < s->len; ++i) {
        (void)fputc(s->data[i], stream);
    }
    (void)fputc('\n', stream);
}


#ifdef ADV_STRING_ENHANCE
#include "base64.h"


lts_str_t *lts_str_clone(lts_str_t *s, lts_pool_t *pool)
{
    lts_str_t *rslt;
    uint8_t *str_mem;

    rslt = (lts_str_t *)lts_palloc(pool, sizeof(lts_str_t));
    str_mem = (uint8_t *)lts_palloc(pool, s->len + 1);
    memcpy(str_mem, s->data, s->len);
    str_mem[s->len] = 0x00;

    rslt->data = str_mem;
    rslt->len = s->len;

    return rslt;
}


// 格式化
lts_str_t *lts_str_sprintf(lts_pool_t *pool, char const *format, ...)
{
    return NULL;
}


lts_str_t **lts_str_split(lts_str_t *src, uint8_t c, lts_pool_t *pool)
{
    int cur, count;
    lts_str_t **rslt;

    // 统计项数
    count = (c == src->data[0]) ? 0 : 1;
    for (ssize_t i = 0; i < src->len - 1; ++i) {
        if (c == src->data[i]) {
            if (c == src->data[i + 1]) {
                continue;
            }

            ++count;
        }
    }

    rslt = (lts_str_t **)lts_palloc(pool, sizeof(lts_str_t *) * (count + 1));

    // 分割
    cur = 0;
    for (ssize_t j = 0; j < src->len; ++j) {
        if (c != src->data[j]) {
            rslt[cur] = (lts_str_t *)lts_palloc(pool, sizeof(lts_str_t));
            rslt[cur]->data = &src->data[j];
            rslt[cur]->len = 1;

            // 计算单项长度
            while ((j < src->len) && (c != src->data[j + 1])) {
                ++j;
                ++rslt[cur]->len;
            }

            ++cur;

            continue;
        }
    }
    rslt[cur] = NULL;

    return rslt;
}


// base64编解码
lts_str_t *lts_str_base64_en(char const *src, lts_pool_t *pool)
{
    lts_str_t tmpsrc = {(uint8_t *)src, strlen(src)};

    return lts_str_base64_en2(&tmpsrc, pool);
}


lts_str_t *lts_str_base64_en2(lts_str_t *src, lts_pool_t *pool)
{
    lts_str_t *rslt = (lts_str_t *)lts_palloc(pool, sizeof(lts_str_t));
    ssize_t encode_len = base64_encode_len(src->len);

    rslt->data = (uint8_t *)lts_palloc(pool, encode_len);
    rslt->len = encode_len - 1;

    (void)base64_encode((char *)rslt->data, (char const *)src->data, src->len);

    return rslt;
}


lts_str_t *lts_str_base64_de(char const *src, lts_pool_t *pool)
{
    lts_str_t tmpsrc = {(uint8_t *)src, strlen(src)};

    return lts_str_base64_de2(&tmpsrc, pool);
}


lts_str_t *lts_str_base64_de2(lts_str_t *src, lts_pool_t *pool)
{
    lts_str_t *rslt = (lts_str_t *)lts_palloc(pool, sizeof(lts_str_t));
    ssize_t decode_len = base64_decode_len((char const *)src->data);

    rslt->data = (uint8_t *)lts_palloc(pool, decode_len);
    rslt->len = decode_len - 1;

    (void)base64_decode((char *)rslt->data, (char const *)src->data);

    return rslt;
}
#endif // ADV_STRING_ENHANCE
