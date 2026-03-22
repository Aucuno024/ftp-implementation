#include "response.h"
#include "request.h"
#include "csapp.h"
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include "utils.h"

#ifdef DELAY
#include <time.h>
#endif

/**
 * @brief Ecrit tous les octets du buffer dans le socket, avec gestion des erreurs
 * @param fd le descripteur de fichier du socket
 * @param buf le buffer à écrire
 * @param len la taille du buffer
 * @return int 0 si l'écriture a réussi, 1 sinon
 */
static int socket_write_all(int fd, const void *buf, size_t len) {
    ssize_t nw = rio_writen(fd, (void *)buf, len);
    if (nw != (ssize_t)len) {
        return 1;
    }
    return 0;
}


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
    if (response == NULL) {
        return;
    }
    socket_write_all(connfd, response, sizeof(response_t));
}

int encode_response(response_t *response, const uint8_t *content) {
    if (response == NULL || content == NULL) {
        return 1;
    }
    memset(response, 0, sizeof(*response));
    response->endian = get_endianess();
    response->error = NO_ERROR_R;
    strncpy((char *)response->content, (const char *)content, MAXLINE - 1);
    response->content[MAXLINE - 1] = '\0';
    return 0;
}

int decode_response(response_t *response, uint8_t *content, uint8_t *error) {
    if (response == NULL || content == NULL || error == NULL) {
        return 1;
    }
    if (response->endian != get_endianess() && swap_endian_response(response) != 0) return 1;
    
    strncpy((char *)content, (const char *)response->content, MAXLINE - 1);
    content[MAXLINE - 1] = '\0';
    *error = response->error;
    return 0;
}

// Swap les bytes du endian du header
int swap_endian_header(transfer_header_t *header) {
    if (!header) return 1;

    header->total_size = ntohl(header->total_size);
    header->block_size = ntohs(header->block_size);

    return 0;
}

int send_transfer_header(int connfd, uint32_t total_size) {
    transfer_header_t header;
    memset(&header, 0, sizeof(header));
    header.endian = get_endianess();
    header.total_size = total_size;
    header.block_size = BLOCK_SIZE;
    header.error = NO_ERROR_R;
    
    return socket_write_all(connfd, &header, sizeof(transfer_header_t));
}

int send_data_block(int connfd, uint16_t block_num, const uint8_t *data, uint16_t data_size) {
    if (data == NULL || data_size > BLOCK_SIZE) {
        return 1;
    }
    
    data_block_t block;
    memset(&block, 0, sizeof(block));
    block.block_num = block_num;
    block.data_size = data_size;
    memcpy(block.data, data, data_size);
    
    return socket_write_all(connfd, &block, sizeof(data_block_t));
}

int send_file_by_blocks(int connfd, char path[]) {
    int fd;
    struct stat st;
    uint32_t file_size = 0;
    
    // Ouvrir le fichier en lecture d'abord
    if (!open_file_r(path, &fd)) {
        // Envoyer un header avec erreur
        transfer_header_t error_header;
        memset(&error_header, 0, sizeof(error_header));
        error_header.endian = get_endianess();
        error_header.total_size = 0;
        error_header.block_size = BLOCK_SIZE;
        error_header.error = PATH_ERROR_R;
        if (socket_write_all(connfd, &error_header, sizeof(transfer_header_t)) != 0) {
            return CLIENT_DISCONNECTED_R;
        }
        return PATH_ERROR_R;
    }
    
    // Obtenir la taille du fichier via fstat
    if (fstat(fd, &st) < 0) {
        Close(fd);
        transfer_header_t error_header;
        memset(&error_header, 0, sizeof(error_header));
        error_header.endian = get_endianess();
        error_header.total_size = 0;
        error_header.block_size = BLOCK_SIZE;
        error_header.error = PATH_ERROR_R;
        if (socket_write_all(connfd, &error_header, sizeof(transfer_header_t)) != 0) {
            return CLIENT_DISCONNECTED_R;
        }
        return PATH_ERROR_R;
    }
    
    file_size = (uint32_t)st.st_size;
    
    // Envoyer le header de transfert
    if (send_transfer_header(connfd, file_size) != 0) {
        Close(fd);
        return CLIENT_DISCONNECTED_R;
    }
    
    // Envoyer le fichier par blocs
    uint8_t buffer[BLOCK_SIZE];
    rio_t rio;
    Rio_readinitb(&rio, fd);
    
    uint16_t block_num = 0;
    uint32_t total_sent = 0;
    
    while (total_sent < file_size) {
        // Lire un bloc
        size_t to_read = (file_size - total_sent > BLOCK_SIZE) ? BLOCK_SIZE : (file_size - total_sent);
        size_t n = Rio_readnb(&rio, buffer, to_read);
        
        if (n == 0) {
            // EOF atteint
            Close(fd);
            return 1;
        }
        
        // Envoyer le bloc
        if (send_data_block(connfd, block_num, buffer, (uint16_t)n) != 0) {
            Close(fd);
            return CLIENT_DISCONNECTED_R;
        }
        
        total_sent += n;
        block_num++;
        #ifdef DELAY
        // Simuler un delai
        if (total_sent < file_size) {
            sleep(DELAY);
        }
        #endif
    }
    
    Close(fd);
    return NO_ERROR_R;
}


int receive_transfer_header(int connfd, transfer_header_t *header, rio_t *rio) {
    if (header == NULL || rio == NULL) {
        return 1;
    }
    
    size_t n = Rio_readnb(rio, header, sizeof(transfer_header_t));
    
    if (n != sizeof(transfer_header_t)) {
        return 1;
    }
    
    // swap endian si nécessaire
    if (header->endian != get_endianess()) {
        if (swap_endian_header(header) != 0) {
            return 1;
        }
    }
    
    return 0;
}

int receive_data_block(int connfd, data_block_t *block, rio_t *rio) {
    if (block == NULL || rio == NULL) {
        return 1;
    }
    
    size_t n = Rio_readnb(rio, block, sizeof(data_block_t));
    
    if (n != sizeof(data_block_t)) {
        return 1;
    }
    
    return 0;
}

int receive_file_by_blocks(int connfd, char path[], transfer_header_t *header_out) {
    transfer_header_t header;
    data_block_t block;
    rio_t rio;
    
    Rio_readinitb(&rio, connfd);
    
    // Recevoir le header
    if (receive_transfer_header(connfd, &header, &rio) != 0) {
        return 1;
    }
    
    // Copier le header en sortie si demandé
    if (header_out != NULL) {
        *header_out = header;
    }
    
    if (header.error != NO_ERROR_R) {
        return header.error;
    }
    
    // Construire le chemin de destination (avec clientdir si chemin relatif)
    char *dest_path = path;
    char full_path[MAXLINE];
    
    if (is_relative_path(path)) {
        snprintf(full_path, sizeof(full_path), "%s%s", DEFAULT_CLIENT_DIR, path);
        dest_path = full_path;
    }
    
    int fd = Open(dest_path, O_CREAT | O_WRONLY | O_TRUNC, DEF_MODE);
    if (fd < 0) {
        return 1;
    }
    
    uint32_t total_received = 0;
    
    while (total_received < header.total_size) {
        if (receive_data_block(connfd, &block, &rio) != 0) {
            Close(fd);
            return 1;
        }
        
        // Ecrire le bloc dans le fichier
        if (Write(fd, block.data, block.data_size) < 0) {
            Close(fd);
            return 1;
        }
        
        total_received += block.data_size;
    }
    
    Close(fd);
    return NO_ERROR_R;
}


int send_response(int connfd, char path[], typereq_t type)
{
    response_t *response;
    
    switch(type)
    {
        case GET:
            return send_file_by_blocks(connfd, path);
            
        case BYE:
            response = malloc(sizeof(response_t));
            if (response == NULL) {
                return 1;
            }
            encode_response(response, (const uint8_t*) "BYE\n");
            write_response(response, connfd);
            free(response);
            return NO_ERROR_R;
            
        default:
            response = malloc(sizeof(response_t));
            if (response == NULL) {
                return 1;
            }
            memset(response, 0, sizeof(*response));
            response->error = TYPE_ERROR_R;
            response->endian = get_endianess();
            write_response(response, connfd);
            free(response);
            return TYPE_ERROR_R;
    }
}

void send_error(int connfd, uint8_t error) 
{
    response_t *response = malloc(sizeof(response_t));
    if (response == NULL) {
        return;
    }
    memset(response, 0, sizeof(*response));
    response->error = error;
    response->endian = get_endianess();
    write_response(response, connfd);
    free(response);
}