#include <zlib.h>
#include <arpa/inet.h>
#include <dirent.h>

#include <strings.h>

#include "latasia.h"
#include "rbtree.h"
#include "logger.h"
#include "conf.h"

#define __THIS_FILE__       "src/modules/mod_app_asyn_backend.c"


static int init_asyn_backend_module(lts_module_t *module)
{
    return 0;
}


static void exit_asyn_backend_module(lts_module_t *module)
{
    return;
}


static void asyn_backend_service(lts_socket_t *s)
{
    return;
}


static void asyn_backend_send_more(lts_socket_t *s)
{
    return;
}


static lts_app_module_itfc_t asyn_backend_itfc = {
    &asyn_backend_service,
    &asyn_backend_send_more,
};

lts_module_t lts_app_asyn_backend_module = {
    lts_string("lts_app_asyn_backend_module"),
    LTS_APP_MODULE,
    &asyn_backend_itfc,
    NULL,
    NULL,
    // interfaces
    NULL,
    &init_asyn_backend_module,
    &exit_asyn_backend_module,
    NULL,
};
