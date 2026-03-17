#ifndef __UTILS_H__
#define __UTILS_H__
#include "csapp.h"

/**
 * @brief Retourne l'endianness de la machine
 * @return int 0 si little endian, 1 si big endian
 */
int get_endianess();
/**
 * @fn int open_file_r(char path[], int *fd)
 * @brief Ouvre un fichier a un chemin donnee dans fd
 * @param path le chemin vers le fichier
 * @param fd le parametre recevant le descripteur
 * @return 1 si tout s'est bien passé 0
 */
int open_file_r(char path[], int *fd);


/**
 * @brief Ecrit le contenu dans un fichier a un chemin donnee
 * @param path le chemin vers le fichier
 * @param content le contenu a ecrire dans le fichier
 * @return 1 si tout s'est bien passé 0 sinon
 */
int write_file_from_content(char path[], const uint8_t *content);


#endif