#ifndef __EVIEO__VSIGNAL_H__
#define __EVIEO__VSIGNAL_H__


enum {
    LTS_MASK_SIGSTOP = (1L << 0),
    LTS_MASK_SIGCHLD = (1L << 1),
    LTS_MASK_SIGPIPE = (1L << 2),
    LTS_MASK_SIGALRM = (1L << 3),
};

typedef struct {
    int         signo;
    char const  *signame;
    void        (*handler)(int signo);
} lts_signal_t;


extern int lts_init_sigactions(void);
#endif // __EVIEO__VSIGNAL_H__
