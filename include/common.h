#ifndef __EVIEO__COMMON_H__
#define __EVIEO__COMMON_H__


#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <sysexits.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>


// 布尔值
#define FALSE           0
#define TRUE            (!FALSE)

// 平台字长
#define LTS_ALIGNMENT               sizeof(unsigned long)
#define LTS_ALIGN(d, a) (\
    ((uintptr_t)(d) + ((uintptr_t)a - 1)) & (~((uintptr_t)a - 1))\
)

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


typedef volatile sig_atomic_t lts_atomic_t;


enum {
    LTS_E_OK = 0,

    LTS_E_SYS = -1023, // 系统错误
    LTS_E_NO_MEM, // 内存耗尽

    LTS_E_EXISTS = -10000, // 资源已存在
    LTS_E_PKG_BROKEN, // 损坏的报文
};


static inline
lts_atomic_t lts_atomic_cmp_set(lts_atomic_t *lock,
                                sig_atomic_t old, sig_atomic_t set)
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
#endif // __EVIEO__COMMON_H__
