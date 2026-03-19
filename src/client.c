/*
 * echoclient.c - An echo client
 */
#include "csapp.h"
#include "request.h"
#include "response.h"
#include <time.h>
#include "utils.h"
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
    
    // Parser le premier mot (commande)
    int n_args = sscanf(cmd, "%s %s", command, path);
    
    if (strcmp((char *) command, "bye") == 0) {
        *typereq = BYE;
        return 0;  // BYE n'a pas d'argument
    } else if (strcmp((char *) command, "get") == 0) {
        if (n_args != 2) return 2;
        *typereq = GET;
    } else if (strcmp((char *) command, "put") == 0) {
        if (n_args != 2) return 2;
        *typereq = PUT;
    } else if (strcmp((char *) command, "ls") == 0) {
        if (n_args != 2) return 2;
        *typereq = LS;
    } else if (strcmp((char *) command, "rm") == 0) {
        if (n_args != 2) return 2;
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
    
    while (1) {
        printf("ftp> ");
        if (Fgets(buf, MAXLINE, stdin) == NULL) {
            // EOF sur stdin
            printf("\n");  // Nouvelle ligne après EOF
            
            // Envoyer BYE pour fermer la connexion
            typereq_t typereq = BYE;
            encode_request(&request, typereq, "");
            write_request(&request, clientfd);
            
            // Lire la réponse BYE du serveur
            response_t response;
            read_response(&response, clientfd);
            break;
        }
        
        typereq_t typereq;
        int err = command_parser(buf, &typereq, buf);
        if (err != 0) {
            printf("Commande invalide : ");
            if (err == 2) {
                printf("Format incorrect. Usage: <command> <path>\n");
            } else if (err == 1) {
                printf("Nom de commande invalide. Commandes valides: get, put, ls, rm, bye\n");
            }
            continue; 
        }

        size_t n = strlen(buf);
        if (n > 0 && buf[n-1] == '\n') buf[n-1] = '\0';

        encode_request(&request, typereq, buf);
        write_request(&request, clientfd);
        
        // Traitement selon le type de requête
        time_t start_time = time(NULL);
        
        if (typereq == GET) {
            // Pour GET réception du fichier par blocs
            char filename[MAXLINE];
            strcpy(filename, buf);
            
            transfer_header_t header;
            int result = receive_file_by_blocks(clientfd, filename, &header);
            
            if (result == NO_ERROR_R) {
                time_t end_time = time(NULL);
                long duration = end_time - start_time;
                if (duration == 0) duration = 1; 
                long speed = (header.total_size / duration) / 1024;
                
                printf("Transfer successfully complete:\n");
                printf("%u bytes received in %ld seconds (%ld Kbytes/s)\n", header.total_size, duration, speed);
            } else {
                printf("File transfer failed with error %d\n", result);
            }
        } else if (typereq == BYE) {
            // Lire la réponse BYE et fermer la connexion
            response_t response;
            if (read_response(&response, clientfd) == 0) {
                uint8_t content[MAXLINE];
                uint8_t error;
                if (decode_response(&response, content, &error) == 0) {
                    printf("Response: %s\n", content);
                }
            }
            break;
        } else {
            // Pour autres requêtes: lecture d'une réponse simple
            uint8_t content[MAXLINE];
            uint8_t error;
            response_t response;
            
            if (read_response(&response, clientfd) == 0 && decode_response(&response, content, &error) == 0) {
                if (error == NO_ERROR_R) {
                    printf("Command completed successfully\n");
                    printf("Response: %s\n", content);
                } else {
                    printf("Command failed with error %d: %s\n", error, (char *)content);
                }
            } else {
                printf("Failed to receive response\n");
            }
        }
    }
    Close(clientfd);
    exit(0);
}
