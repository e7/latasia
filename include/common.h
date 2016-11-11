/**
 * latasia
 * Copyright (c) 2015 e7 <jackzxty@126.com>
 * */


#ifndef __LATASIA__COMMON_H__
#define __LATASIA__COMMON_H__


#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <sysexits.h>

#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "build.h"


#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// 布尔值
#define FALSE           0
#define TRUE            (!FALSE)
#define NOP             (void)0

#define LTS_CPU_PAUSE()         __asm__ ("pause")
#define LTS_MEMORY_FENCE()      __asm__ __volatile__( "" : : : "memory" )

// 平台字长
#define LTS_WORD                sizeof(unsigned long)
#define LTS_ALIGN(d, a) (\
    ((uintptr_t)(d) + ((uintptr_t)(a) - 1)) & (~((uintptr_t)(a) - 1))\
)

#define ASSERT(c)           if (c) {;} else {raise(SIGSEGV);}
#define ARRAY_COUNT(a)      ((int)(sizeof(a) / sizeof(a[0])))
#define OFFSET_OF(s, m)     ((size_t)&(((s *)0)->m ))
#define CONTAINER_OF(ptr, type, member)     \
            ({\
                const __typeof__(((type *)0)->member) *p_mptr = (ptr);\
                (type *)((uint8_t *)p_mptr - OFFSET_OF(type, member));\
             })

#define MIN(a, b)           (((a) > (b)) ? (b) : (a))
#define MAX(a, b)           (((a) < (b)) ? (b) : (a))

#define STDIN_NAME          "stdin"
#define STDOUT_NAME         "stdout"
#define STDERR_NAME         "stderr"

#define SRC2STR_ARG(arg)    #arg
#define SRC2STR(src)        SRC2STR_ARG(src)


typedef volatile sig_atomic_t lts_atomic_t;
typedef struct timeval lts_timeval_t;


static inline
int lts_atomic_cmp_set(lts_atomic_t *lock, sig_atomic_t old, sig_atomic_t set)
{
    uint8_t res;

    __asm__ volatile (
        "lock;"
        "cmpxchgl %3, %1;"
        "sete %0;"
        : "=a" (res)
        : "m" (*lock), "a" (old), "r" (set)
        : "cc", "memory");

    return res;
}

extern char const **lts_errno_desc;

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // __LATASIA__COMMON_H__
