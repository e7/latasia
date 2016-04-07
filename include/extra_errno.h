/**
 * latasia
 * Copyright (c) 2015 e7 <jackzxty@126.com>
 * */


#ifndef __LATASIA__EXTRA_ERRNO_H__
#define __LATASIA__EXTRA_ERRNO_H__


#include "errno.h"


#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define LTS_E_BADF                      EBADF
#define LTS_E_INVAL                     EINVAL
#define LTS_E_SRCH                      ESRCH
#define LTS_E_PERM                      EPERM
#define LTS_E_CHILD                     ECHILD
#define LTS_E_NOMEM                     ENOMEM

#define LTS_E_AGAIN                     EAGAIN
#define LTS_E_WOULDBLOCK                EWOULDBLOCK
#define LTS_E_CONNABORTED               ECONNABORTED
#define LTS_E_CONNRESET                 ECONNRESET
#define LTS_E_INTR                      EINTR
#define LTS_E_PIPE                      EPIPE

#define LTS_E_SYS                       200
#define LTS_E_INVALID_FORMAT            201

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // __LATASIA__EXTRA_ERRNO_H__
