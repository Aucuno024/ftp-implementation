#include "csapp.h"
#include "request.h"
#include "response.h"
#include "utils.h"

#define MAX_SIZE_PORT 5
#define PORT 2121
#define MAX_NAME_LEN 256
#define SPEAKER "hal 9000"
#define COMM_SLAVE_PORT 2222

#ifndef NB_SLAVE
#define NB_SLAVE 2
#endif

#ifndef SLAVE_PORT
#define SLAVE_PORT 2121
#endif


#ifndef SLAVE_PATH
#define SLAVE_PATH "config_master"
#endif

#ifndef MAX_PASS
#define MAX_PASS 51
#endif

#ifndef PASSWORD
#define PASSWORD "Pa$$word"
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


void write_cred(char *cred, char *port, char *ip)
{
    int i = 0;
    size_t n = strlen(ip);
    for(; i < n; i++)
    {
        cred[i] = ip[i];
    }
    n = strlen(port);
    cred[i++] = ':';
    for(int j = 0; j < n; j++)
    {
        cred[i++] = port[j]; 
    }
    cred[i] ='\0';
    #ifdef DEBUG
        printf("%s say \" Cred = %s\"\n", SPEAKER, cred);
    #endif
}

/* 
 * Note that this code only works with IPv4 addresses
 * (IPv6 is not supporteded)
 */
int main(int argc, char **argv)
{
   

    Signal(SIGCHLD, handler_chld);
    Signal(SIGINT, handler_int);
    char port[5];
    sprintf(port, "%d", SLAVE_PORT);
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_in clientaddr;
    char client_ip_string[INET_ADDRSTRLEN];
    char client_hostname[MAX_NAME_LEN];
    char slave_ip[NB_SLAVE][INET_ADDRSTRLEN];
    int i = 0, connected = 0;
    clientlen = (socklen_t)sizeof(clientaddr);
   
    int fd = Open(SLAVE_PATH, O_RDONLY, 0);
    rio_t rio;
    Rio_readinitb(&rio, fd);
    char buf[MAXLINE];
    int n = 0;
    for(int i = 0; i < NB_SLAVE && (n = Rio_readlineb(&rio, buf, MAXLINE)); i++)
    {
        int j = 0;
        for(; j < INET_ADDRSTRLEN - 1; j++)
        {
            #ifdef DEBUG
                printf("%s say \"Caractere lu : %c\"\n", SPEAKER, buf[j]);
            #endif
            if(buf[j] == '\n' || buf[j] == '\0')
            {
                slave_ip[i][j]='\0';
                break;
            }
            if(buf[j] == '\r')
                slave_ip[i][j] = '\0';
            else
                slave_ip[i][j] = buf[j];
        }
        slave_ip[i][j] = '\0';
        connected++;
    }
    Close(fd);
    pid_t pid = Fork();
    if(pid) {
        listenfd = Open_listenfd(PORT);
    } else 
    {
        listenfd = Open_listenfd(COMM_SLAVE_PORT);
    }

    #ifdef DEBUG
        printf("%s (%d) say \"client len :%u listenfd : %d\"\n", SPEAKER, pid, clientlen, listenfd);
    #endif
   
    while (1) {
        if(pid)
        {
            connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
            Getnameinfo((SA *) &clientaddr, clientlen,
                            client_hostname, MAX_NAME_LEN, 0, 0, 0);
                
            Inet_ntop(AF_INET, &clientaddr.sin_addr, client_ip_string,
                    INET_ADDRSTRLEN); 
            #ifdef DEBUG
                printf("%s say \"Server connected to %s (%s)\"\n", SPEAKER, client_hostname, client_ip_string);
            #endif

            request_t* request = malloc(sizeof(request_t));
            if (request == NULL) 
            {
                #ifdef DEBUG
                    printf("%s say \"Request can't allocate\"\n", SPEAKER);
                #endif
                Close(connfd);
                continue;
            }
            if (read_request(request, connfd)) 
            {
            #ifdef DEBUG
                printf("%s say \"Failed to read request (size: %ld)\"\n", SPEAKER, sizeof(request_t));
            #endif
            free(request);
            send_error(connfd, ERROR_READ_REQUEST);
            break;
            }
            char content[MAX_PASS];
            typereq_t typereq;
            decode_request(request, &typereq, content);
            free(request);
            if(strcmp(content, PASSWORD))
            {
                #ifdef DEBUG
                    printf("%s say \"Password not correct %s, %s, %d\"\n", SPEAKER, PASSWORD, content, strcmp(content, PASSWORD));
                #endif
                send_error(connfd, PASSWORD_ERROR);
                break;
            }
            #ifdef DEBUG
                    printf("%s say \"check request type\"\n", SPEAKER);
            #endif
            if(connected && typereq == GET)
            {
            
                i = (i + 1) % connected;
                #ifdef DEBUG
                    printf("%s say \"Provide ip %s\" and PORT (%s)\"\n", SPEAKER, slave_ip[i], port);
                #endif
                response_t *r = malloc(sizeof(response_t));
                r->endian = get_endianess();
                r->error = 0;

                write_cred((char *)r->content, port, (char *)slave_ip[i]);
                write_response(r, connfd);
                free(r);
            } else 
            {
                send_error(connfd, SLAVE_ERROR);
            }
            
        } else
        {
            connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
            Getnameinfo((SA *) &clientaddr, clientlen,
                            client_hostname, MAX_NAME_LEN, 0, 0, 0);
                
            Inet_ntop(AF_INET, &clientaddr.sin_addr, client_ip_string,
                    INET_ADDRSTRLEN); 
            #ifdef DEBUG
                printf("%s say \"Server connected to slave %s (%s)\"\n", SPEAKER, client_hostname, client_ip_string);
            #endif
            request_t* request = malloc(sizeof(request_t));
            if (request == NULL) 
            {
                #ifdef DEBUG
                    printf("%s say \"Request can't allocate\"\n", SPEAKER);
                #endif
                Close(connfd);
                continue;
            }
            if (read_request(request, connfd)) 
            {
                #ifdef DEBUG
                    printf("%s say \"Failed to read request (size: %ld)\"\n", SPEAKER, sizeof(request_t));
                #endif
                free(request);
                send_error(connfd, ERROR_READ_REQUEST);
                break;
            }
            char content[MAXLINE];
            typereq_t typereq;
            decode_request(request, &typereq, content);
            free(request);
            
            if(connected && typereq == GET)
            {
                char *content = NULL;
                for(int j = 0; j < connected; j++)
                {
                    if(slave_ip[j][0] == '\0')
                        continue;
                    if(strcmp(slave_ip[j], client_ip_string) != 0)
                    {
                        update(&content, slave_ip[j]);
                    }
                }
                #ifdef DEBUG
                    printf("%s say \"Send %s to %s\"\n", SPEAKER, content, client_ip_string);
                #endif
                if(content)
                {
                    send_content(connfd, content, strlen(content));
                    free(content);
                } else {
                    send_content(connfd, "", 0);
                }
            } else 
            {
                send_error(connfd, SLAVE_ERROR);
            }
        }
        Close(connfd);
    }
}

