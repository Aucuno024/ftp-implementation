#include "request.h"
#include "csapp.h"
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <utils.h>

#define IS_LITTLE_ENDIAN ((uint16_t)1 & 0xFF == 1)

int swap_endian_request(request_t *request) {
    if (request == NULL) {
        return 1;
    }
    // Pour l'instant on swap que le champ typereq (les autres champs sont des octets)
    assert(sizeof(typereq_t) == 4);
    request->typereq = (typereq_t) ((request->typereq >> 24) | ((request->typereq << 8) & 0x00FF0000) | ((request->typereq >> 8) & 0x0000FF00) | (request->typereq << 24));
    return 0;
}

int read_request(request_t *request, int connfd)
{
    size_t n = 0;
    if (request == NULL) {
        return 1;
    }
    rio_t rio;

    Rio_readinitb(&rio, connfd);
    n = Rio_readnb(&rio, request, sizeof(request_t));
    return n != sizeof(request_t);
}

void write_request(request_t *request, int connfd) {
    Rio_writen(connfd, request, sizeof(request_t));
}

int encode_request(request_t *request, typereq_t typereq, const char *path) {
    if (request == NULL || path == NULL) {
        return 1;
    }
    request->endian = get_endianess();
    request->typereq = typereq;
    strncpy(request->path, path, MAXLINE);
    return 0;
}

int decode_request(request_t *request, typereq_t *typereq, char *path) {
    if (request == NULL || typereq == NULL || path == NULL) {
        return 1;
    }
    if (request->endian != get_endianess() && swap_endian_request(request) != 0) return 1;
    
    *typereq = request->typereq;
    strncpy(path, request->path, MAXLINE);
    return 0;
}

