#include "csapp.h"
#include "request.h"
#include "response.h"
#include "logs.h"

#ifndef SLAVE_PORT
#define SLAVE_PORT 2121
#endif

#ifndef POOL_SIZE
#define POOL_SIZE 20
#endif

#define MAX_NAME_LEN 256
#define SPEAKER "Raccoon"

#ifndef MASTER_PATH
#define MASTER_PATH "config_slave"
#endif

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

int coherence(log_t *log, char *master_ip);
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
    char master_ip[INET_ADDRSTRLEN];
    clientlen = (socklen_t)sizeof(clientaddr);

    int fd = Open(MASTER_PATH, O_RDONLY, 0);
    rio_t rio;
    Rio_readinitb(&rio, fd);
    char buf[MAXLINE];
    int n = 0;
    int j = 0;
    Rio_readlineb(&rio, buf, MAXLINE);
    for(; j < INET_ADDRSTRLEN - 1; j++)
    {
        #ifdef DEBUG
            printf("%s say \"Caractere lu : %c\"\n", SPEAKER, buf[j]);
        #endif
        if(buf[j] != '\n')
            master_ip[j] = buf[j];
        else
            master_ip[j]='\0';
    }
    master_ip[j] = '\0';
    Close(fd);

    listenfd = Open_listenfd(SLAVE_PORT);
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
                log_t *log;
                int is_update = 0;
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
                    if(typereq == UPDATE)
                        is_update = 1;
                    #ifdef DEBUG
                    printf("%s say \"%ld bytes reçu\"\n", SPEAKER, strlen(path));
                    printf("%s say \"\t- type de requete : %d\"\n", SPEAKER, typereq);
                    printf("%s say \"\t- chemin : %s\"\n", SPEAKER, path);
                    #endif
                    
                    if (send_server_response(connfd, path, typereq, log) == CLIENT_DISCONNECTED_R && (typereq == GET || typereq == RESUME)) {
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
                if(!is_update)
                {
                    coherence(log, master_ip);
                }
                free_log(log);
            }
        }
    }
    while(1);
}

