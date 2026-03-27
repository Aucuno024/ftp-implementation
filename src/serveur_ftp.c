#include "csapp.h"
#include "request.h"
#include "response.h"
#include "logs.h"

#ifndef SLAVE_PORT
#define SLAVE_PORT 2121
#endif

#ifndef CLIENT_PATH
#define CLIENT_PATH bin/client
#endif

#define COMM_SLAVE_PORT 2222

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

int coherence(log_t *log, char *master_ip)
{
    int clientfd;
    int fds[2];
    char *content = NULL;
    size_t total = 0;
    ssize_t n;
    char tmp[MAXLINE];

    if (log == NULL || master_ip == NULL) {
        return 0;
    }

    clientfd = Open_clientfd(master_ip, COMM_SLAVE_PORT);
    if (clientfd < 0) {
        return 1;
    }

    request_t request;
    encode_request(&request, GET, "");
    write_request(&request, clientfd);

    if (pipe(fds) == -1) {
        Close(clientfd);
        return 1;
    }
    if (receive_content(clientfd, fds[1]) != NO_ERROR_R) {
        Close(fds[0]);
        Close(fds[1]);
        Close(clientfd);
        return 1;
    }
    Close(fds[1]);
    Close(clientfd);

    while ((n = Read(fds[0], tmp, sizeof(tmp))) > 0) {
        char *new_content = realloc(content, total + (size_t)n + 1);
        if (new_content == NULL) {
            free(content);
            Close(fds[0]);
            return 1;
        }
        content = new_content;
        memcpy(content + total, tmp, (size_t)n);
        total += (size_t)n;
        content[total] = '\0';
    }
    Close(fds[0]);

    if (content == NULL || total == 0) {
        free(content);
        return 0;
    }

    char *slave_ip = content;
    while (slave_ip && *slave_ip) {
        char *next = strchr(slave_ip, '\n');
        if (next) {
            *next = '\0';
            next++;
        }

        size_t ip_len = strlen(slave_ip);
        if (ip_len > 0 && slave_ip[ip_len - 1] == '\r') {
            slave_ip[ip_len - 1] = '\0';
            ip_len--;
        }

        if (ip_len > 0) {
            int slavefd = Open_clientfd(slave_ip, SLAVE_PORT);
            if (slavefd >= 0) {
                log_t *entry = log;
                while (entry) {
                    char update_payload[MAXLINE];
                    response_t ack;
                    if (snprintf(update_payload, sizeof(update_payload), "%d\n%s", (int)entry->type, entry->path) < (int)sizeof(update_payload)) {
                        encode_request(&request, UPDATE, update_payload);
                        write_request(&request, slavefd);
                        if (read_response(&ack, slavefd)) {
                            break;
                        }
                    }
                    entry = follow(entry);
                }
                encode_request(&request, BYE, "");
                write_request(&request, slavefd);
                Close(slavefd);
            }
        }

        slave_ip = next;
    }

    free(content);
    return 0;
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
                log_t *log = NULL;
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

