/*
 * echoclient.c - An echo client
 */
#include "csapp.h"
#include "request.h"
#include "response.h"
#define PORT 2121

int main(int argc, char **argv)
{
    int clientfd;
    char *host, buf[MAXLINE];

    if (argc != 2) {
        fprintf(stderr, "usage: %s <host>\n", argv[0]);
        exit(0);
    }
    host = argv[1];

    /*
     * Note that the 'host' can be a name or an IP address.
     * If necessary, Open_clientfd will perform the name resolution
     * to obtain the IP address.
     */
    clientfd = Open_clientfd(host, PORT);
    
    /*
     * At this stage, the connection is established between the client
     * and the server OS ... but it is possible that the server application
     * has not yet called "Accept" for this connection
     */
    printf("client connected to server OS\n"); 
    
    request_t request;
    response_t response;
    while (Fgets(buf, MAXLINE, stdin) != NULL) {
        encode_request(&request, GET, buf);
        write_request(&request, clientfd);
        
        if (read_response(&response, clientfd) == 0) {
            uint8_t content[MAXLINE];
            if (decode_response(&response, content) == 0) {
                printf("Réponse: %s", content);
            }
        }
    }
    Close(clientfd);
    exit(0);
}
