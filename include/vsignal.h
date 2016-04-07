/**
 * latasia
 * Copyright (c) 2015 e7 <jackzxty@126.com>
 * */


#ifndef __LATASIA__VSIGNAL_H__
#define __LATASIA__VSIGNAL_H__


#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

enum {
    LTS_MASK_SIGEXIT = (1L << 0),
    LTS_MASK_SIGCHLD = (1L << 1),
    LTS_MASK_SIGPIPE = (1L << 2),
    LTS_MASK_SIGALRM = (1L << 3),
};

typedef struct {
    int         signo;
    char const  *signame;
    void        (*handler)(int signo);
} lts_signal_t;


extern int lts_init_sigactions(int ismaster);

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // __LATASIA__VSIGNAL_H__
