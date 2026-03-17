/*
 * echoclient.c - An echo client
 */
#include "csapp.h"
#include "request.h"
#include "response.h"
#define PORT 2121



/**
 * @brief Analyse une commande entrée par l'utilisateur et remplit les champs typereq et path
 * @param cmd la commande entrée par l'utilisateur
 * @param typereq  un pointeur vers le type de requete a remplir
 * @param path  un pointeur vers le chemin a remplir
 * @return 0 si la commande est valide et a été parsée, 1 si le nom de la commande est invalide, 2 si le format de la commande est incorrect 
 */
int command_parser(const char *cmd, typereq_t *typereq, char *path) {
    if (cmd == NULL || typereq == NULL || path == NULL) {
        return 1;
    }
    uint8_t command[MAXLINE];
    if (sscanf(cmd, "%s %s", command, path) != 2) {
        return 2;
    }

    if (strcmp((char *) command, "get") == 0) {
        *typereq = GET;
    } else if (strcmp((char *) command, "put") == 0) {
        *typereq = PUT;
    } else if (strcmp((char *) command, "ls") == 0) {
        *typereq = LS;
    } else if (strcmp((char *) command, "rm") == 0) {
        *typereq = RM;
    } else {
        return 1;
    }
    return 0;
}

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
    if (Fgets(buf, MAXLINE, stdin) != NULL) {
        typereq_t typereq;
        int err = command_parser(buf, &typereq, buf);
        if (err != 0) {
            printf("Commande invalide : ");
            if (err == 2) {
                printf("Format incorrect. Usage: <command> <path>\n");
            } else if (err == 1) {
                printf("Nom de commande invalide. Commandes valides: get, put, ls, rm\n");
            }
        } else {
            size_t n = strlen(buf);
            if (n > 0 && buf[n-1] == '\n') buf[n-1] = '\0';
            encode_request(&request, typereq, buf);
            write_request(&request, clientfd);
            
            // Lecture de la réponse du serveur
            uint8_t content[MAXLINE];
            if (read_response(&response, clientfd) == 0 && decode_response(&response, content) == 0) {
                printf("Reponse: %s", content);
            }
        }
    }
    Close(clientfd);
    exit(0);
}
