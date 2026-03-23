/*
 * echoclient.c - An echo client
 */
#include "csapp.h"
#include "request.h"
#include "response.h"
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include "utils.h"

#define PORT 2121
#define PART_SUFFIX ".part"
#define META_SUFFIX ".part.meta"
#define SPEAKER "Al"


/**
 * @brief Construit le chemin local de destination à partir du chemin distant
 * @param path  le chemin distant fourni par le client
 * @param dest  le variable où stocker le chemin local construit
 * @param dest_size  la taille de la variable dest
 * @return int 0 si le chemin a été construit, 1 sinon
 */
static int build_client_dest_path(const char *path, char *dest, size_t dest_size) {
    if (path == NULL || dest == NULL || dest_size == 0) {
        return 1;
    }

    if (is_relative_path((char *)path)) {
        if (snprintf(dest, dest_size, "%s%s", DEFAULT_CLIENT_DIR, path) >= (int)dest_size) {
            return 1;
        }
    } else {
        if (snprintf(dest, dest_size, "%s", path) >= (int)dest_size) {
            return 1;
        }
    }

    return 0;
}

/**
 * @brief  Construit les chemins de fichier partiel et de métadonnées à partir du chemin distant
 * 
 * @param remote_path  le chemin distant du fichier à télécharger
 * @param part_path  le variable où stocker le chemin du fichier partiel avec suffixe .part
 * @param part_size  la taille de la variable part_path
 * @param meta_path  le variable où stocker le chemin des métadonnées
 * @param meta_size  la taille de la variable meta_path
 * @return int 0 si les chemins ont été construits, 1 sinon
 */
static int build_partial_paths(const char *remote_path, char *part_path, size_t part_size, char *meta_path, size_t meta_size) {
    char final_path[MAXLINE];

    if (build_client_dest_path(remote_path, final_path, sizeof(final_path)) != 0) {
        return 1;
    }

    if (snprintf(part_path, part_size, "%s%s", final_path, PART_SUFFIX) >= (int)part_size) {
        return 1;
    }

    if (snprintf(meta_path, meta_size, "%s%s", final_path, META_SUFFIX) >= (int)meta_size) {
        return 1;
    }

    return 0;
}

/**
 * @brief  Vérifie si une chaîne de caractères se termine par un suffixe donné
 * @param value  la chaîne de caractères à vérifier
 * @param suffix  le suffixe à rechercher
 * @return int  1 si value se termine par suffix, 0 sinon
 */
static int has_suffix(const char *value, const char *suffix) {
    size_t len;
    size_t suffix_len;

    if (value == NULL || suffix == NULL) {
        return 0;
    }

    len = strlen(value);
    suffix_len = strlen(suffix);
    if (len < suffix_len) {
        return 0;
    }

    return strcmp(value + len - suffix_len, suffix) == 0;
}


/**
 * @brief Lit les métadonnées de transfert depuis un fichier .part.meta
 * @param meta_path  le chemin du fichier de métadonnées
 * @param remote_path  le buffer où stocker le chemin distant lu
 * @param remote_size  la taille du buffer remote_path
 * @param offset  un pointeur où stocker l'offset déjà reçu
 * @return int 0 si les métadonnées ont été lues, 1 sinon
 */
static int read_meta_file(const char *meta_path, char *remote_path, size_t remote_size, uint32_t *offset) {
    FILE *f;
    char offset_buf[64];
    char *endptr;
    unsigned long parsed;
    size_t n;

    if (meta_path == NULL || remote_path == NULL || offset == NULL) {
        return 1;
    }

    f = fopen(meta_path, "r");
    if (f == NULL) {
        return 1;
    }

    if (fgets(remote_path, (int)remote_size, f) == NULL) {
        fclose(f);
        return 1;
    }

    n = strlen(remote_path);
    while (n > 0 && (remote_path[n - 1] == '\n' || remote_path[n - 1] == '\r')) {
        remote_path[n - 1] = '\0';
        n--;
    }

    if (fgets(offset_buf, sizeof(offset_buf), f) == NULL) {
        fclose(f);
        return 1;
    }

    fclose(f);

    parsed = strtoul(offset_buf, &endptr, 10);
    if (endptr == offset_buf || (*endptr != '\0' && *endptr != '\n') || parsed > UINT32_MAX) {
        return 1;
    }

    *offset = (uint32_t)parsed;
    return 0;
}

/**
 * @brief Supprime les fichiers partiels associés à un chemin distant
 * @param remote_path  le chemin distant du fichier à supprimer
 * @return int 0 si les fichiers ont été supprimés, 1 sinon
 */
static int remove_partial_files(const char *remote_path) {
    char part_path[MAXLINE];
    char meta_path[MAXLINE];

    if (build_partial_paths(remote_path, part_path, sizeof(part_path), meta_path, sizeof(meta_path)) != 0) {
        return 1;
    }

    unlink(part_path);
    unlink(meta_path);
    return 0;
}

/**
 * @brief Essaie d'obtenir l'offset de reprise d'un téléchargement à partir d'un fichier de métadonnées 
 * @param remote_path  le chemin distant du fichier à télécharger
 * @param offset_out  un pointeur où stocker l'offset de reprise trouvé, ou 0 si aucun offset de reprise n'est disponible
 * @return int  1 si un offset de reprise a été trouvé et stocké dans offset_out, 0 sinon
 */
static int maybe_get_resume_offset(const char *remote_path, uint32_t *offset_out) {
    char part_path[MAXLINE];
    char meta_path[MAXLINE];
    char meta_remote[MAXLINE];
    struct stat st;
    uint32_t offset;

    if (offset_out == NULL) {
        return 0;
    }

    *offset_out = 0;

    if (build_partial_paths(remote_path, part_path, sizeof(part_path), meta_path, sizeof(meta_path)) != 0) {
        return 0;
    }

    if (read_meta_file(meta_path, meta_remote, sizeof(meta_remote), &offset) != 0) {
        return 0;
    }

    if (strcmp(meta_remote, remote_path) != 0 || offset == 0) {
        return 0;
    }

    if (stat(part_path, &st) != 0) {
        return 0;
    }
    if ((uint32_t)st.st_size < offset) {
        return 0;
    }

    *offset_out = offset;
    return 1;
}

/**
 * @brief  Effectue un téléchargement d'un fichier depuis le serveur, avec reprise automatique si un fichier partiel existe localement
 * @param clientfd  le descripteur de fichier du socket de connexion au serveur
 * @param remote_path  le chemin distant du fichier à télécharger
 * @param start_offset  l'offset de reprise à utiliser pour le téléchargement
 * @param show_prefix  un indicateur pour afficher un préfixe lors de l'affichage des messages
 * @return int  0 en cas de succès, 1 en cas d'erreur
 */
static int perform_download(int clientfd, const char *remote_path, uint32_t start_offset, int show_prefix) {
    request_t request;
    char data[MAXLINE];
    typereq_t wire_type = GET;
    transfer_header_t header;
    int result;
    time_t start_time;

    if (start_offset > 0) {
        wire_type = RESUME;
        if (snprintf(data, sizeof(data), "%s\n%u", remote_path, start_offset) >= (int)sizeof(data)) {
            return 1;
        }
    } else {
        if (snprintf(data, sizeof(data), "%s", remote_path) >= (int)sizeof(data)) {
            return 1;
        }
    }

    encode_request(&request, wire_type, data);
    write_request(&request, clientfd);

    start_time = time(NULL);
    result = receive_file_by_blocks_resume(clientfd, (char *)remote_path, (char *)remote_path, start_offset, &header);

    if (result == NO_ERROR_R) {
        time_t end_time = time(NULL);
        long duration = end_time - start_time;
        long speed;
        if (duration == 0) {
            duration = 1;
        }
        speed = (header.total_size / duration) / 1024;
        if (show_prefix) {
            printf("[AUTO-RESUME] ");
        }
        printf("Transfer successfully complete:\n");
        printf("%u bytes received in %ld seconds (%ld Kbytes/s)\n", header.total_size, duration, speed);
        return NO_ERROR_R;
    }

    if (result == PATH_ERROR_R && start_offset > 0) {
        remove_partial_files(remote_path);
    }

    if (show_prefix) {
        printf("[AUTO-RESUME] ");
    }
    printf("File transfer failed with error %d\n", result);
    return result;
}

/**
 * @brief Scanne le répertoire client à la recherche de fichiers partiels et tente de reprendre automatiquement les téléchargements en cours avec le serveur
 * 
 * @param clientfd  le descripteur de fichier du socket de connexion au serveur
 */
static void auto_resume_downloads(int clientfd) {
    DIR *dir;
    struct dirent *entry;

    dir = opendir(DEFAULT_CLIENT_DIR);
    if (dir == NULL) {
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        char meta_path[MAXLINE];
        char remote_path[MAXLINE];
        uint32_t offset = 0;

        if (!has_suffix(entry->d_name, META_SUFFIX)) {
            continue;
        }

        if (snprintf(meta_path, sizeof(meta_path), "%s%s", DEFAULT_CLIENT_DIR, entry->d_name) >= (int)sizeof(meta_path)) {
            continue;
        }

        if (read_meta_file(meta_path, remote_path, sizeof(remote_path), &offset) != 0) {
            unlink(meta_path);
            continue;
        }

        if (offset == 0) {
            unlink(meta_path);
            continue;
        }

        perform_download(clientfd, remote_path, offset, 1);
    }

    closedir(dir);
}



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


int get_cred(int *port, char *host, int clientfd)
{
    request_t request;
    encode_request(&request, GET, "");
    write_request(&request, clientfd);

    response_t response;
    read_response(&response, clientfd);

    uint8_t content[MAXLINE], error;
    if(decode_response(&response, content, &error))
        return -2;
    if(error)
        return error;
    #ifdef DEBUG
            fprintf(stdout, "%s say \"Content : %s\"\n", SPEAKER, content);
        #endif
    int i;
    for(i = 0; content[i] != ':' && i < INET6_ADDRSTRLEN; i++)
    {
        #ifdef DEBUG
            fprintf(stdout, "%s say \"Value : %c\"\n", SPEAKER, content[i]);
        #endif
        host[i] = content[i];
    }
    host[i] = '\0';
    #ifdef DEBUG
        fprintf(stdout, "%s say \"Host get :%s\"\n", SPEAKER, host);
    #endif
    char *strport = strchr((char*) content, ':');
    if(!strport)
        return -1;
    *port = atoi(++strport);
    return error;
}



int main(int argc, char **argv)
{
    int clientfd;
    char *master, buf[MAXLINE], host[INET_ADDRSTRLEN];
    int port;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <host>\n", argv[0]);
        exit(0);
    }
    master = argv[1];

    /*
     * Note that the 'host' can be a name or an IP address.
     * If necessary, Open_clientfd will perform the name resolution
     * to obtain the IP address.
     */
    clientfd = Open_clientfd(master, PORT);
    
    /*
     * At this stage, the connection is established between the client
     * and the server OS ... but it is possible that the server application
     * has not yet called "Accept" for this connection
     */
    printf("client connected to server Master\n"); 
    int err;
    if((err = get_cred(&port, host, clientfd)))
    {
        printf("Erreur : aucun acces disponible a un slave %d\n", err);
        exit(err);
    }
    Close(clientfd);
    clientfd = Open_clientfd(host, port);
    printf("client connected to server OS\n"); 

    auto_resume_downloads(clientfd);
    
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

        if (typereq == GET) {
            uint32_t resume_offset = 0;
            if (maybe_get_resume_offset(buf, &resume_offset)) {
                perform_download(clientfd, buf, resume_offset, 0);
            } else {
                perform_download(clientfd, buf, 0, 0);
            }
        } else if (typereq == BYE) {
            encode_request(&request, typereq, "");
            write_request(&request, clientfd);

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
            encode_request(&request, typereq, buf);
            write_request(&request, clientfd);

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
