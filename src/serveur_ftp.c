#include "csapp.h"
#include "request.h"
#include "response.h"

#define PORT 2121
#define MAX_NAME_LEN 256
#ifndef POOL_SIZE
#define POOL_SIZE 20
#endif

#define SPEAKER "Raccoon"


#ifdef DEBUG
void handler_pipe(int signal) 
{
    printf("%s say \"Received SIGPIPE\"\n", SPEAKER);
}
#endif

void handler_chld(int signal) 
{
    wait(NULL);
}

void handler_int(int signal)
{
    #ifdef DEBUG
        Kill(0, SIGKILL);
        while(printf("%s say \" Catch%d\"\n",SPEAKER, waitpid(0, NULL, WNOHANG)));
    #else 
        Kill(0, SIGKILL);
        while(waitpid(0, NULL, WNOHANG));
    #endif
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
    #ifdef DEBUG
    Signal(SIGPIPE, handler_pipe);
    #else
    Signal(SIGPIPE, SIG_IGN);
    #endif
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_in clientaddr;
    char client_ip_string[INET_ADDRSTRLEN];
    char client_hostname[MAX_NAME_LEN];

    clientlen = (socklen_t)sizeof(clientaddr);

    listenfd = Open_listenfd(PORT);
    #ifdef DEBUG
        printf("%s say \"client len :%u listenfd : %d\"\n", SPEAKER, clientlen, listenfd);
    #endif
    for(int i = 0; i < POOL_SIZE; i++)
    {
        if(Fork() == 0) {
            while (1) {
                connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
                Getnameinfo((SA *) &clientaddr, clientlen,
                                client_hostname, MAX_NAME_LEN, 0, 0, 0);
                    
                Inet_ntop(AF_INET, &clientaddr.sin_addr, client_ip_string,
                        INET_ADDRSTRLEN);

                #ifdef DEBUG
                    printf("%s say \"Server connected to %s (%s)\"\n", SPEAKER, client_hostname, client_ip_string);
                #endif
                while(1) {
                    request_t* request = malloc(sizeof(request_t));

                    if (request == NULL) {
                        #ifdef DEBUG
                            printf("%s say \"Request can't allocate\"\n", SPEAKER);
                        #endif
                        Close(connfd);
                        continue;
                    }
                    if (read_request(request, connfd)) {
                        #ifdef DEBUG
                            printf("%s say \"Failed to read request (size: %ld)\"\n", SPEAKER, sizeof(request_t));
                        #endif
                        free(request);
                        break;
                    }

                    char path[MAXLINE];
                    typereq_t typereq;

                    decode_request(request, &typereq, path);
                    free(request);

                    #ifdef DEBUG
                    printf("%s say \"%ld bytes reçu\"\n", SPEAKER, strlen(path));
                    printf("%s say \"\t- type de requete : %d\"\n", SPEAKER, typereq);
                    printf("%s say \"\t- chemin : %s\"\n", SPEAKER, path);
                    #endif
                    if (send_response(connfd, path, typereq) == CLIENT_DISCONNECTED_R && (typereq == GET || typereq == RESUME)) {
                        #ifdef DEBUG
                            printf("%s say \"Client disconnected during transfer\"\n", SPEAKER);
                        #endif
                        break;
                    }
                    

                    #ifdef DEBUG
                        printf("%s say \"Request send\"\n", SPEAKER);
                    #endif

                    if(typereq == BYE)
                        break;
                }
                #ifdef DEBUG
                    printf("%s say \"End of connection\"\n", SPEAKER);
                #endif
                Close(connfd);
            }
        }
    }
    while(1);
}

