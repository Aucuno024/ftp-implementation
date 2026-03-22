#include "response.h"
#include "request.h"
#include "csapp.h"
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
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

/**
 * @brief Construit le chemin de destination pour un fichier client
 * @param path le chemin du fichier
 * @param dest le buffer de destination
 * @param dest_size la taille du buffer de destination
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
 * @param base_path  le chemin de base du fichier 
 * @param part_path  le variable où stocker le chemin du fichier partiel avec suffixe .part
 * @param part_size  la taille du buffer part_path
 * @param meta_path  le variable où stocker le chemin du fichier de métadonnées avec suffixe .part.meta
 * @param meta_size  la taille du buffer meta_path
 * @return int 
 */
static int build_partial_paths(const char *base_path, char *part_path, size_t part_size, char *meta_path, size_t meta_size) {
    if (base_path == NULL || part_path == NULL || meta_path == NULL) {
        return 1;
    }

    if (snprintf(part_path, part_size, "%s.part", base_path) >= (int)part_size) {
        return 1;
    }
    if (snprintf(meta_path, meta_size, "%s.part.meta", base_path) >= (int)meta_size) {
        return 1;
    }

    return 0;
}

/**
 * @brief  Ecrit les métadonnées de transfert dans un fichier .part.meta
 * 
 * @param meta_path  le chemin du fichier de métadonnées
 * @param remote_path  le chemin distant du fichier en cours de téléchargement
 * @param offset  l'offset déjà reçu du fichier
 * @return int 
 */
static int write_part_meta(const char *meta_path, const char *remote_path, uint32_t offset) {
    char tmp_path[MAXLINE];
    FILE *f;

    if (meta_path == NULL || remote_path == NULL) {
        return 1;
    }

    if (snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", meta_path) >= (int)sizeof(tmp_path)) {
        return 1;
    }

    f = fopen(tmp_path, "w");
    if (f == NULL) {
        return 1;
    }

    if (fprintf(f, "%s\n%u\n", remote_path, offset) < 0) {
        fclose(f);
        unlink(tmp_path);
        return 1;
    }

    if (fclose(f) != 0) {
        unlink(tmp_path);
        return 1;
    }

    if (rename(tmp_path, meta_path) != 0) {
        unlink(tmp_path);
        return 1;
    }

    return 0;
}

/**
 * @brief Envoie un header d'erreur au client avec un code d'erreur spécifique
 * 
 * @param connfd  socket de connexion
 * @param error  le code d'erreur à envoyer
 * @return int 
 */
static int send_error_header(int connfd, uint8_t error) {
    transfer_header_t error_header;
    memset(&error_header, 0, sizeof(error_header));
    error_header.endian = get_endianess();
    error_header.total_size = 0;
    error_header.block_size = BLOCK_SIZE;
    error_header.error = error;
    if (socket_write_all(connfd, &error_header, sizeof(transfer_header_t)) != 0) {
        return CLIENT_DISCONNECTED_R;
    }
    return error;
}

/**
 * @brief  Parse les données d'une requete de reprise pour en extraire le chemin distant et l'offset de reprise
 * 
 * @param data les datas de la requete de reprise, au format
 * @param path_out  le buffer où stocker le chemin distant extrait
 * @param path_out_size  la taille du buffer path_out
 * @param offset_out  un pointeur où stocker l'offset de reprise extrait
 * @return int 
 */
static int parse_resume_data(const char *data, char *path_out, size_t path_out_size, uint32_t *offset_out) {
    const char *sep;
    size_t path_len;
    char *endptr;
    unsigned long offset;

    if (data == NULL || path_out == NULL || offset_out == NULL) {
        return 1;
    }

    sep = strchr(data, '\n');
    if (sep == NULL) {
        return 1;
    }

    path_len = (size_t)(sep - data);
    if (path_len == 0 || path_len >= path_out_size) {
        return 1;
    }

    memcpy(path_out, data, path_len);
    path_out[path_len] = '\0';

    if (*(sep + 1) == '\0') {
        return 1;
    }

    offset = strtoul(sep + 1, &endptr, 10);
    if (endptr == sep + 1 || *endptr != '\0' || offset > UINT32_MAX) {
        return 1;
    }

    *offset_out = (uint32_t)offset;
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


static int send_file_by_blocks_from_offset(int connfd, char path[], uint32_t start_offset) {
    int fd;
    struct stat st;
    uint32_t file_size = 0;
    
    // Ouvrir le fichier en lecture d'abord
    if (!open_file_r(path, &fd)) {
        return send_error_header(connfd, PATH_ERROR_R);
    }
    
    // Obtenir la taille du fichier via fstat
    if (fstat(fd, &st) < 0) {
        Close(fd);
        return send_error_header(connfd, PATH_ERROR_R);
    }
    
    file_size = (uint32_t)st.st_size;

    if (start_offset > file_size) {
        Close(fd);
        return send_error_header(connfd, OFFSET_ERROR_R);
    }
    
    // Envoyer le header de transfert
    if (send_transfer_header(connfd, file_size) != 0) {
        Close(fd);
        return CLIENT_DISCONNECTED_R;
    }

    if (lseek(fd, (off_t)start_offset, SEEK_SET) < 0) {
        Close(fd);
        return 1;
    }
    
    // Envoyer le fichier par blocs
    uint8_t buffer[BLOCK_SIZE];
    rio_t rio;
    Rio_readinitb(&rio, fd);
    
    uint16_t block_num = (uint16_t)(start_offset / BLOCK_SIZE);
    uint32_t total_sent = start_offset;
    
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


int send_file_by_blocks(int connfd, char path[]) {
    return send_file_by_blocks_from_offset(connfd, path, 0);
}

int send_response(int connfd, char path[], typereq_t type)
{
    response_t *response;
    
    switch(type)
    {
        case GET:
            return send_file_by_blocks(connfd, path);

        case RESUME: {
            char resume_path[MAXLINE];
            uint32_t start_offset = 0;
            if (parse_resume_data(path, resume_path, sizeof(resume_path), &start_offset) != 0) {
                send_error(connfd, TYPE_ERROR_R);
                return TYPE_ERROR_R;
            }
            return send_file_by_blocks_from_offset(connfd, resume_path, start_offset);
        }
            
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
    return receive_file_by_blocks_resume(connfd, path, path, 0, header_out);
}

int receive_file_by_blocks_resume(int connfd, char remote_path[], char local_path[], uint32_t start_offset, transfer_header_t *header_out) {
    transfer_header_t header;
    data_block_t block;
    rio_t rio;
    char final_path[MAXLINE];
    char part_path[MAXLINE];
    char meta_path[MAXLINE];
    int fd;
    uint32_t total_received;
    off_t current_size;
    
    Rio_readinitb(&rio, connfd);
    
    // Recevoir le header
    if (receive_transfer_header(connfd, &header, &rio) != 0) {
        return 1;
    }
    
    if (header.error != NO_ERROR_R) {
        return header.error;
    }

    if (start_offset > header.total_size) {
        return OFFSET_ERROR_R;
    }

    if (build_client_dest_path(local_path, final_path, sizeof(final_path)) != 0) {
        return 1;
    }
    if (build_partial_paths(final_path, part_path, sizeof(part_path), meta_path, sizeof(meta_path)) != 0) {
        return 1;
    }

    fd = Open(part_path, O_CREAT | O_WRONLY, DEF_MODE);
    if (fd < 0) {
        return 1;
    }

    current_size = lseek(fd, 0, SEEK_END);
    if (current_size < 0) {
        Close(fd);
        return 1;
    }

    if ((uint32_t)current_size < start_offset) {
        Close(fd);
        return OFFSET_ERROR_R;
    }

    if (ftruncate(fd, (off_t)start_offset) < 0) {
        Close(fd);
        return 1;
    }

    if (lseek(fd, (off_t)start_offset, SEEK_SET) < 0) {
        Close(fd);
        return 1;
    }

    total_received = start_offset;

    if (write_part_meta(meta_path, remote_path, total_received) != 0) {
        Close(fd);
        return 1;
    }
    
    while (total_received < header.total_size) {
        if (receive_data_block(connfd, &block, &rio) != 0) {
            Close(fd);
            return CLIENT_DISCONNECTED_R;
        }
        
        // Ecrire le bloc dans le fichier
        if (Write(fd, block.data, block.data_size) < 0) {
            Close(fd);
            return 1;
        }
        
        total_received += block.data_size;

        if (write_part_meta(meta_path, remote_path, total_received) != 0) {
            Close(fd);
            return 1;
        }
    }

    Close(fd);

    if (rename(part_path, final_path) != 0) {
        return 1;
    }

    unlink(meta_path);

    if (header_out != NULL) {
        *header_out = header;
    }

    return NO_ERROR_R;
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