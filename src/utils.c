#include "utils.h"
#include "csapp.h"
#include <stdint.h>
#include "string.h"


int get_endianess() {
    static uint32_t one = 1;
    return ((* (uint8_t *) &one) == 0);
}

/**
 * @brief Vérification si le fichier a ouvrir est dans le dossier du serveur
 * @param abs_path le chemin absolu du fichier a ouvrir
 * @return int 1 si le fichier est dans le dossier du serveur, 0 sinon
 */
int is_path_in_server_dir(const char *abs_path) {
    char server_dir_abs[MAXLINE];
    if (realpath(DEFAULT_SERVER_DIR, server_dir_abs) == NULL) {
        return 0;
    }
    size_t dir_len = strlen(server_dir_abs);
    return strncmp(abs_path, server_dir_abs, dir_len) == 0 && (abs_path[dir_len] == '/' || abs_path[dir_len] == '\0');
}


/** 
 * @brief Convertir un chemin en chemin absolu depuis le dossier du serveur
 * @param path le chemin à convertir
 * @param abs_path le buffer où stocker le chemin absolu converti
 */
void convert_to_abs_path(const char *path, char *abs_path) {
    char temp_path[MAXLINE];
    if (path == NULL || abs_path == NULL) {
        abs_path[0] = '\0'; // chemin invalide
        return;
    }
    if (path[0] == '/') {
        // chemin absolu
        snprintf(temp_path, MAXLINE, "%s%s", DEFAULT_SERVER_DIR, path + 1);
    } else {
        // chemin relatif
        snprintf(temp_path, MAXLINE, "%s/%s", DEFAULT_SERVER_DIR, path);
    }
    // convertir le chemin du serveur en chemin absolu
    if (realpath(temp_path, abs_path) == NULL) {
        abs_path[0] = '\0'; // chemin invalide
    }
}
/**
 * @brief Fourni le chemin vers le serveur à partir d'un chemin donné, en vérifiant que le chemin résultant est bien dans le dossier du serveur
 * @param path le chemin à convertir
 * @param server_path le buffer où stocker le chemin vers le serveur
 * @return int 1 si le chemin est valide et dans le dossier du serveur, 0 sinon
 */
int build_server_path(const char *path, char *server_path) {
    convert_to_abs_path(path, server_path);
    if (server_path[0] == '\0' || !is_path_in_server_dir(server_path)) {
        return 0;
    }
    return 1;
}


int open_file_r(char path[], int *fd)
{
    if (path == NULL || fd == NULL) {
        return 0;
    }
    char abs_path[MAXLINE];
    if (!build_server_path(path, abs_path)) {
        return 0;
    }
    return (*fd = open(abs_path, O_RDONLY, 0)) != -1;
}

int write_file_from_content(char path[], const uint8_t *content)
{
    int fd;
    if ((fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644)) == -1) {
        return 0;
    }
    write(fd, content, strlen((char *)content));
    close(fd);
    return 1;
}

int write_file_to_client_dir(char path[], const uint8_t *content)
{
    if(is_relative_path(path))
    {
        char *newpath = malloc(strlen(path) + strlen(DEFAULT_CLIENT_DIR) + 1);
        int i;
        for(i = 0; DEFAULT_CLIENT_DIR[i] != '\0'; i++)
        {
            newpath[i] = DEFAULT_CLIENT_DIR[i];
        }
        for(int j = 0; path[j] != '\0'; j++)
        {
            newpath[i++] = path[j];
        }
        newpath[i] = '\0';
        int r = write_file_from_content(newpath, content);
        free(newpath);
        return r;
    }
    return write_file_from_content(path, content);
}
int is_relative_path(char path[])
{
    return path[0] == '~' || path[0] == '/'? 0: 1;
}
