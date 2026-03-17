#include "response.h"
#include "csapp.h"
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <utils.h>



int swap_endian_response(response_t *response) {
    if (response == NULL) {
        return 1;
    }
    // Pour l'instant rien a swap dans la reponse (que des octets)
    return 0;
}

int read_response(response_t *response, int connfd) {
    size_t n = 0;
    if (response == NULL) {
        return 1;
    }
    rio_t rio;

    Rio_readinitb(&rio, connfd);
    n = Rio_readnb(&rio, response, sizeof(response_t));
    return n != sizeof(response_t);
}

void write_response(response_t *response, int connfd) {
    Rio_writen(connfd, response, sizeof(response_t));
}

int encode_response(response_t *response, const uint8_t *content) {
    if (response == NULL || content == NULL) {
        return 1;
    }
    response->endian = get_endianess();
    strncpy((char *)response->content, (const char *)content, MAXLINE);
    return 0;
}

int decode_response(response_t *response, uint8_t *content) {
    if (response == NULL || content == NULL) {
        return 1;
    }
    if (response->endian != get_endianess() && swap_endian_response(response) != 0) return 1;
    
    strncpy((char *)content, (const char *)response->content, MAXLINE);
    return 0;
}
