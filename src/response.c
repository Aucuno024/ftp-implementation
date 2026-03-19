#include "response.h"
#include "request.h"
#include "csapp.h"
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include "utils.h"



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

int decode_response(response_t *response, uint8_t *content, uint8_t *error) {
    if (response == NULL || content == NULL || error == NULL) {
        return 1;
    }
    if (response->endian != get_endianess() && swap_endian_response(response) != 0) return 1;
    
    strncpy((char *)content, (const char *)response->content, MAXLINE);
    *error = response->error;
    return 0;
}

int send_response(int connfd, char path[], typereq_t type)
{
    int fd;
    response_t *response = malloc(sizeof(response_t));
    response->error = NO_ERROR_R;
    switch(type)
    {
        case GET:
            if(!open_file_r(path, &fd))
            {
                response->error = PATH_ERROR_R;
                write_response(response, connfd);
                free(response);
                return PATH_ERROR_R;
            }
            uint8_t buf[MAXBUF];
            rio_t rio;
            Rio_readinitb(&rio, fd);
            
            size_t n = Rio_readnb(&rio, buf, MAXBUF - 1);
            buf[n] = '\0';

            encode_response(response, buf);
            write_response(response, connfd);
            free(response);
            return NO_ERROR_R;
        case BYE:
            encode_response(response, (const uint8_t*) "BYE\n");
            write_response(response, connfd);
            free(response);
            return NO_ERROR_R;
        default:
            response->error = TYPE_ERROR_R;
            write_response(response, connfd);
            free(response);
            return TYPE_ERROR_R;
    }
}

void send_error(int connfd, uint8_t error) 
{
    response_t *response = malloc(sizeof(response_t));
    response->error = error;
    response->endian = get_endianess();
    write_response(response, connfd);
    free(response);
}