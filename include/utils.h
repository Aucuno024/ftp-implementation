#ifndef __UTILS_H__
#define __UTILS_H__
#include "csapp.h"

#ifndef DEFAULT_SERVER_DIR
#define DEFAULT_SERVER_DIR "./serverdir/"
#endif

#ifndef DEFAULT_CLIENT_DIR
#define DEFAULT_CLIENT_DIR "./clientdir/"
#endif
/**
 * @brief Retourne l'endianness de la machine
 * @return int 0 si little endian, 1 si big endian
 */
int get_endianess();

/**
 * @fn int open_file_r(char path[], int *fd)
 * @brief Ouvre un fichier a un chemin donnee dans fd, en verifiant que le chemin est valide et dans le dossier dirpath
 * @param path le chemin vers le fichier
 * @param fd le parametre recevant le descripteur
 * @param dirpath le chemin du dossier du serveur
 * @return 1 si tout s'est bien passé 0
 */
int open_file_r(char path[], int *fd, const char *dirpath);

/**
 * @brief Ecrit le contenu dans un fichier a un chemin donnee
 * @param path le chemin vers le fichier
 * @param content le contenu a ecrire dans le fichier
 * @return 1 si tout s'est bien passé 0 sinon
 */
int write_file_from_content(char path[], const uint8_t *content);

/**
 * @brief Ecrit le contenu dans un fichier a un chemin donnee dans le dossier destdir
 * @param path le chemin vers le fichier
 * @param content le contenu a ecrire dans le fichier
 * @param dirpath le chemin du dossier de destination
 * @return 1 si tout s'est bien passé 0 sinon
 */
int write_file_to_dest_dir(char path[], const uint8_t *content, const char *dirpath);

/**
 * @fn int is_relative_path(char path[])
 * @brief Verifie si un chemin depend du repertoire courant ou non
 * @param path le chemin a tester
 * @return 1 si c'est le cas 0 sinon
 */
int is_relative_path(char path[]);

/**
 * @fn int list_dir(char *path, char *content) 
 * @brief Liste le contenu d'un repetoire a une adresse donnee dans un pointeur passe en parametre
 * @param path l'adresse du repertoire
 * @param content le pointeur qui va obtenir le contenu sous la forme 'content\ncontent2'
 * @return 0 si réussi sans probleme 1 sinon
 */
int list_dir(char *path, char **content);

/**
 * @brief Fourni le chemin absolu à partir d'un chemin donné, en vérifiant que le chemin résultant est bien dans le dossier du dirpath
 * @param path le chemin à convertir
 * @param server_path le buffer où stocker le chemin absolu vers le serveur
 * @param dirpath le chemin du dossier du serveur
 * @return int 1 si le chemin est valide et dans le dossier du serveur, 0 sinon
 */
int get_abs_dest_path_from_src_path(const char *path, char *server_path, const char *dirpath);

/**
 * @fn int update(char **content, char *element)
 * @brief Concatene dynamiquement un tableau de caractere a un autre en les separant de \n
 * @param content un pointeur vers le tableau de caractere principal
 * @param element le tableau de caractere a ajouter
 * @return 1 si erreur 0 sinon
 */
int update(char **content, char *element);

/**
 * @fn void parse(char *str, char chr)
 * @brief slipt une chaine de caractère en plusieurs a chaque occurence d'un caractere (la chaine suivante
 * est accessible avec strchr(str, '\0'))
 * @param str la chaine a split
 * @param chr le caractere de split
 */
void split(char *str, char chr);
#endif
