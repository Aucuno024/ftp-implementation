#include "csapp.h"
#include "request.h"
#include "response.h"

#define PORT 2121
#define MAX_NAME_LEN 256
#ifndef POOL_SIZE
#define POOL_SIZE 20
#endif

int i;


void handler_chld(int signal) 
{
    wait(NULL);
    i--;
}

void handler_int(int signal)
{
    Kill(0, SIGKILL);
    while(waitpid(0, NULL, WNOHANG));
    exit(0);
}
/* 
 * Note that this code only works with IPv4 addresses
 * (IPv6 is not supporteded)
 */
int main(int argc, char **argv)
{
    Signal(SIGCHLD, handler_chld);
    Signal(SIGINT, handler_int);
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_in clientaddr;
    char client_ip_string[INET_ADDRSTRLEN];
    char client_hostname[MAX_NAME_LEN];

    clientlen = (socklen_t)sizeof(clientaddr);

    listenfd = Open_listenfd(PORT);
    for(i = 0; i < POOL_SIZE; i++)
    {
        if(Fork() == 0) {
            while (1) {
                connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
                Getnameinfo((SA *) &clientaddr, clientlen,
                            client_hostname, MAX_NAME_LEN, 0, 0, 0);
                
                Inet_ntop(AF_INET, &clientaddr.sin_addr, client_ip_string,
                        INET_ADDRSTRLEN);
                
                printf("server connected to %s (%s)\n", client_hostname,
                    client_ip_string);
                
                request_t* request = malloc(sizeof(request_t));

                if (request == NULL) {
                    Close(connfd);
                    continue;
                }
                if (read_request(request, connfd)) {
                    printf("Failed to read request (size: %ld)\n", sizeof(request_t));
                    free(request);
                    Close(connfd);
                    continue;
                }

                // Traitement de la requete temporaire
                char path[MAXLINE];
                typereq_t typereq;

                decode_request(request, &typereq, path);
                free(request);

                printf("%ld bytes reçu\n", strlen(path));
                printf("\t- type de requete : %d\n", typereq);
                printf("\t- chemin : %s\n", path);

                // Creation de la réponse temporaire
                response_t* response = malloc(sizeof(response_t));
                if (response == NULL) {
                    Close(connfd);
                    continue;
                }
                encode_response(response, (const uint8_t *)"OK\n");
                write_response(response, connfd);
                free(response);

                Close(connfd);
            }
        }
    }
    while(1);
}

