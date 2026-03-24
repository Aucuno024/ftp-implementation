#include "utils.h"
#include "csapp.h"
#include <stdint.h>
#include "string.h"
#include <dirent.h>

#define SPEAKER "Gyro"

int get_endianess() {
    static uint32_t one = 1;
    return ((* (uint8_t *) &one) == 0);
}

/**
 * @brief Vérification si le fichier abs_path a ouvrir est dans le dossier dirpath 
 * @param abs_path le chemin absolu du fichier a ouvrir
 * @param dirpath le chemin du dossier du serveur
 * @return int 1 si le fichier est dans le dossier du serveur, 0 sinon
 */
int is_path_in_dirpath(const char *abs_path, const char * dirpath) {
    char server_dir_abs[MAXLINE];
    if (realpath(dirpath, server_dir_abs) == NULL) {
        return 0;
    }
    size_t dir_len = strlen(server_dir_abs);
    return strncmp(abs_path, server_dir_abs, dir_len) == 0 && (abs_path[dir_len] == '/' || abs_path[dir_len] == '\0');
}


/** 
 * @brief Convertir un chemin en chemin absolu depuis le dossier dirpath
 * @param path le chemin à convertir
 * @param abs_path le buffer où stocker le chemin absolu converti
 * @param dirpath le chemin du dossier du serveur
 */
void convert_to_abs_path(const char *path, char *abs_path, const char *dirpath) {
    char temp_path[MAXLINE];
    if (path == NULL || abs_path == NULL) {
        abs_path[0] = '\0'; // chemin invalide
        return;
    }
    if (path[0] == '/') {
        // chemin absolu
        snprintf(temp_path, MAXLINE, "%s%s", dirpath, path + 1);
    } else {
        // chemin relatif
        snprintf(temp_path, MAXLINE, "%s/%s", dirpath, path);
    }
    // convertir le chemin du serveur en chemin absolu
    if (realpath(temp_path, abs_path) == NULL) {
        abs_path[0] = '\0'; // chemin invalide
    }
}
/**
 * @brief Fourni le chemin absolu vers le serveur à partir d'un chemin donné, en vérifiant que le chemin résultant est bien dans le dossier du dirpath
 * @param path le chemin à convertir
 * @param server_path le buffer où stocker le chemin absolu vers le serveur
 * @param dirpath le chemin du dossier du serveur
 * @return int 1 si le chemin est valide et dans le dossier du serveur, 0 sinon
 */
int get_abs_dest_path_from_src_path(const char *path, char *server_path, const char *dirpath) {
    convert_to_abs_path(path, server_path, dirpath);
    if (server_path[0] == '\0' || !is_path_in_dirpath(server_path, dirpath)) {
        return 0;
    }
    return 1;
}


int open_file_r(char path[], int *fd, const char *dirpath)
{
    if (path == NULL || fd == NULL) {
        return 0;
    }
    char abs_path[MAXLINE];
    if (!get_abs_dest_path_from_src_path(path, abs_path, dirpath)) {
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

int write_file_to_dest_dir(char path[], const uint8_t *content, const char *dirpath)
{
    if(is_relative_path(path))
    {
        char *newpath = malloc(strlen(path) + strlen(dirpath) + 1);
        int i;
        for(i = 0; dirpath[i] != '\0'; i++)
        {
            newpath[i] = dirpath[i];
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

int update(char **content, char *element)
{
    size_t s = strlen(element);
    if(!(*content))
    {
        #ifdef DEBUG
            printf("%s say \"First element added\"\n", SPEAKER);
        #endif
        (*content) = malloc(s + 1);
        if(!(*content))
            return 1;
        for(int i = 0; i < s; i++ )
        {
            (*content)[i] = element[i];
        }
        (*content)[s] = '\0';
        #ifdef DEBUG
            printf("%s say \"content value : %s\"\n", SPEAKER, content);
        #endif
        return 0;
    }
    size_t sc = strlen((*content));
    char * tmp = realloc((*content), sc + s + 2);
    if(!tmp)
        return 1;
    (*content) = tmp;
    (*content)[sc] = '\n';
    for(int i = sc + 1; i < sc + s + 1; i++)
    {
        (*content)[i] = element[i - sc - 1];
    }
    (*content)[sc + s + 1] = '\0';
    #ifdef DEBUG
            printf("%s say \"Content value : %s\"\n", SPEAKER, content);
    #endif
    return 0;
}

int list_dir(char *path, char **content) 
{
    char server_path[MAXBUF];
    if(!get_abs_dest_path_from_src_path(path, server_path, DEFAULT_SERVER_DIR))
    {
        #ifdef DEBUG
            printf("%s say \"Path non etendu : %s\"\n", SPEAKER, path);
        #endif
        return 1;
    }
    #ifdef DEBUG
            printf("%s say \"Path etendu : %s -> %s\"\n", SPEAKER, path, server_path);
     #endif
    
    struct dirent *de;
    DIR *dr = opendir(server_path);
    if(!dr)
    {
        #ifdef DEBUG
            printf("%s say \"Ne peux pas ouvrir: %s\"\n", SPEAKER, server_path);
        #endif
        int fd = open(server_path, O_RDONLY, 0);
        if(fd != -1)
        {
            #ifdef DEBUG
                printf("%s say \"Fichier ouvert : %d \"\n", SPEAKER, fd);
            #endif
            *content= malloc(sizeof(path) + 1);
            strcpy(*content, path);
            return 0;
        }
        return 1;
    }
     while ((de = readdir(dr)) != NULL)
    {
        #ifdef DEBUG
            printf("%s say \"Dir value : %s\"\n", SPEAKER, de->d_name);
        #endif
        if(update(content, de->d_name))
        {
             #ifdef DEBUG
                printf("%s say \"Erreur: %s\"\n", SPEAKER, content);
            #endif
            return 1;
        }
    }
    closedir(dr);    
    return 0;
}

