#include "logs.h"
#include "request.h"
#include <string.h>
#include <stdlib.h>

int add(log_t **log, typereq_t type, const char *path)
{
    size_t path_len;

    if(!log || !path)
        return 1;

    path_len = strnlen(path, MAXBUF);
    if(path_len >= MAXBUF)
        return 1;

    if(!(*log))
    {
        (*log) = malloc(sizeof(log_t));
        if(!(*log))
            return 1;
        memcpy((*log)->path, path, path_len + 1);
        (*log)->type=type;
        (*log)->follow = NULL;
        return 0;
    }
    log_t * c = (*log);
    while(c->follow)
        c = c->follow;
    c->follow = malloc(sizeof(log_t));
    if(!c->follow)
        return 1;
    c = c->follow;
    memcpy(c->path, path, path_len + 1);
    c->type = type;
    c->follow = NULL;
    return 0;
}

log_t *follow(log_t *log)
{
    if(!log)
        return NULL;
    return log->follow;
}

void free_log(log_t *log)
{
    while(log)
    {
        log_t *tmp = log->follow;
        free(log);
        log = tmp;
    }
}