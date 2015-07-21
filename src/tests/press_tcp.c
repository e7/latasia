#if 1
#include <common.h>
#include <pthread.h>


#define USAGE "usage: %s -s server -p port -c num -t time\n"


static void *thread_proc(void *args)
{
    return NULL;
}


int main(int argc, char *argv[], char *env[])
{
    int opt;
    int nthreads = 1; // 默认一个线程
    int nsec = 5; // 默认运行5秒
    int nport;

    if (1 == argc) {
        (void)fprintf(stderr, USAGE, argv[0]);
        return EXIT_SUCCESS;
    }

    while ((opt = getopt(argc, argv, "s:p:c:t:")) != -1) {
        switch (opt) {
        case 's': {
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
            if ((nthreads < 1) || (nthreads > 99999)) {
                (void)fprintf(stderr, "invalid argument for -c\n");
                return EXIT_FAILURE;
            }

            (void)fprintf(stderr, "nthreads=%d", nthreads);
            break;
        }

        case 't': {
            nsec = atoi(optarg);
            if ((nsec < 1) || (nsec > 99999)) {
                (void)fprintf(stderr, "invalid argument for -t\n");
                return EXIT_FAILURE;
            }

            (void)fprintf(stderr, "nsec=%d\n", nsec);
            break;
        }

        default: {
            (void)fprintf(stderr, USAGE, argv[0]);
            break;
        }
        }
    }

    return EXIT_SUCCESS;
}

#else

      #include <unistd.h>
       #include <stdlib.h>
       #include <stdio.h>

       int
       main(int argc, char *argv[])
       {
           int flags, opt;
           int nsecs, tfnd;

           nsecs = 0;
           tfnd = 0;
           flags = 0;
           while ((opt = getopt(argc, argv, "nt:")) != -1) {
               switch (opt) {
               case 'n':
                   flags = 1;
                   break;
               case 't':
                   nsecs = atoi(optarg);
                   tfnd = 1;
                   break;
               default: /* '?' */
                   fprintf(stderr, "Usage: %s [-t nsecs] [-n] name\n",
                           argv[0]);
                   exit(EXIT_FAILURE);
               }
           }

           printf("flags=%d; tfnd=%d; optind=%d\n", flags, tfnd, optind);

           if (optind >= argc) {
               fprintf(stderr, "Expected argument after options\n");
               exit(EXIT_FAILURE);
           }

           printf("name argument = %s\n", argv[optind]);

           /* Other code omitted */

           exit(EXIT_SUCCESS);
       }

#endif
