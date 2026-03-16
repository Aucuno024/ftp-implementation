#include "request.h"
#include "csapp.h"
#include <stdint.h>

int read_request(request_t *request, int connfd)
{
    size_t n = 0;
    request = malloc(sizeof(request_t));
    rio_t rio;

    Rio_readinitb(&rio, connfd);
    n = rio_readnb(&rio, request, sizeof(request_t));
    return n == sizeof(request_t);
}

void write_request(request_t *request, int connfd) {
    Rio_writen(connfd, request, sizeof(request_t));
}

