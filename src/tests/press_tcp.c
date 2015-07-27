#include <common.h>
#include <pthread.h>


#define MAX_THREADS     9999
#define USAGE "usage: %s -s server -p port -c num -t time\n"


static volatile sig_atomic_t keep_running = 1;


void sec_sleep(long sec)
{
    int err;
    struct timeval tv;

    if (sec <= 0) {
        abort();
    }

    tv.tv_sec = sec;
    tv.tv_usec = 0;
    do {
        err = select(0, NULL, NULL, NULL, &tv);
    } while (err < 0 && errno == EINTR);

    return;
}

void msec_sleep(long msec)
{
    int err;
    struct timeval tv;

    if (msec <= 0) {
        abort();
    }

    tv.tv_sec = msec / 1000;
    tv.tv_usec = (msec % 1000) * 1000;
    do {
        err = select(0, NULL, NULL, NULL, &tv);
    } while (err < 0 && errno == EINTR);

    return;
}

void usec_sleep(long usec)
{
    int err;
    struct timeval tv;

    if (usec <= 0) {
        abort();
    }

    tv.tv_sec = usec / 1000000;
    tv.tv_usec = usec % 1000000;
    do {
        err = select(0, NULL, NULL, NULL, &tv);
    } while (err < 0 && errno == EINTR);

    return;
}

static void *thread_proc(void *args)
{
    while (keep_running) {
        fprintf(stderr, "lalala\n");
    }

    return NULL;
}


int main(int argc, char *argv[], char *env[])
{
    int opt;
    int nthreads = 1; // 默认一个线程
    int nsec = 5; // 默认运行5秒
    int nport = 0;
    char const *srv_name = NULL;
    pthread_t *pids;

    if (1 == argc) {
        (void)fprintf(stderr, USAGE, argv[0]);
        return EXIT_SUCCESS;
    }

    while ((opt = getopt(argc, argv, "s:p:c:t:")) != -1) {
        switch (opt) {
        case 's': {
            srv_name = optarg;
            break;
        }

        case 'p': {
            nport = atoi(optarg);
            if ((nport < 1) || (nport > 65535)) {
                (void)fprintf(stderr, "invalid argument for -p\n");
                return EXIT_FAILURE;
            }

            break;
        }

        case 'c': {
            nthreads = atoi(optarg);
            if ((nthreads < 1) || (nthreads > MAX_THREADS)) {
                (void)fprintf(stderr, "invalid argument for -c\n");
                return EXIT_FAILURE;
            }

            break;
        }

        case 't': {
            nsec = atoi(optarg);
            if ((nsec < 1) || (nsec > 99999)) {
                (void)fprintf(stderr, "invalid argument for -t\n");
                return EXIT_FAILURE;
            }

            break;
        }

        case '?': {
            break;
        }

        default: {
            abort();
            break;
        }
        }
    }

    if (NULL == srv_name) {
        (void)fprintf(stderr, "need to provide valid -s option\n");
        return EXIT_FAILURE;
    }

    if (0 == nport) {
        (void)fprintf(stderr, "need to provide valid -p option\n");
        return EXIT_FAILURE;
    }

    // 打印执行配置
    (void)fprintf(stderr, "target: %s:%d\n", srv_name, nport);
    (void)fprintf(stderr, "threads: %d\n", nthreads);
    (void)fprintf(stderr, "duration of time: %ds\n", nsec);

    // 创建线程组
    pids = (pthread_t *)malloc(sizeof(pthread_t) * nthreads);
    for (int i = 0; i < nthreads; ++i) {
        switch (pthread_create(&pids[i], NULL, thread_proc, NULL)) {
            case 0: {
                break;
            }

            case EAGAIN: {
                break;
            }

            case EINVAL: {
                break;
            }

            case EPERM: {
                break;
            }

            default: {
                abort();
                break;
            }
        }
    }

    // 计时
    sec_sleep(nsec);
    keep_running = 0;

    // 等待线程组退出
    for (int i = 0; i < nthreads; ++i) {
        switch(pthread_join(pids[i], NULL)) {
            case 0: {
                break;
            }

            case EINVAL: {
                break;
            }

            case ESRCH: {
                break;
            }

            default: {
                abort();
                break;
            }
        }
    }
    free(pids);

    return EXIT_SUCCESS;
}
