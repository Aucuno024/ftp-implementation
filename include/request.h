#ifndef __REQUEST_H__
#define __REQUEST_H__
#include "csapp.h"
#include <stdint.h>

typedef enum {
    GET = 0,
    PUT = 1,
    LS = 2,
    RM = 3
} typereq_t;

typedef struct {
    uint8_t endian;
    typereq_t typereq;
    char path[MAXLINE];
} request_t;
/**
 * @fn int read_request(request_t *request, int connfd)
 * @brief lit une requete depuis un descripteur de fichier
 * @param request la variable ou ecrire ce qui est lu du descripteur
 * @param connfd le descripteur de fichier
 * @return 1 si il n'y pas d'erreur 0 sinon.
 */
int read_request(request_t *request, int connfd);
#endif