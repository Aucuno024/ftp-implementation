#ifndef __REQUEST_H__
#define __REQUEST_H__
#include "csapp.h"
#include <stdint.h>

typedef enum {
    GET = 0,
    PUT = 1,
    LS = 2,
    RM = 3
} typereq_t;

typedef struct {
    typereq_t typereq;
    uint8_t endian; // 0 pour little endian, 1 pour big endian
    char path[MAXLINE];
} request_t;

/**
 * @brief Permet de swap les endians d'une requete
 * @param request la requete a swap
 * @return int 0 si le swap a été effectué 1 sinon
 */
int swap_endian_request(request_t *request);

/**
 * @fn int read_request(request_t *request, int connfd)
 * @brief lit une requete depuis un descripteur de fichier
 * @param request la variable ou ecrire ce qui est lu du descripteur
 * @param connfd le descripteur de fichier
 * @return 0 si il n'y pas d'erreur 1 sinon.
 */
int read_request(request_t *request, int connfd);

/**
 * @brief Ecrit une requête dans le socket de connexion
 * @param request la requête à envoyer
 * @param connfd le socket de connexion
 */
void write_request(request_t *request, int connfd);

/**
 * @brief Encode une requete en remplissant les champs de la structure request_t
 * @param request la requete a remplir
 * @param typereq  le type de la requete a encoder
 * @param path  le chemin de la requete a encoder
 * @return int 0 si l'encodage a été effectué 1 sinon
 */
int encode_request(request_t *request, typereq_t typereq, const char *path);

/**
 * @brief Decode une requete en lisant les champs de la structure request_t
 * @param request la requete a decoder
 * @param typereq  un pointeur vers le type de la requete a decoder
 * @param path  le chemin de la requete a decoder
 * @return int 0 si le décodage a été effectué 1 sinon
 */
int decode_request(request_t *request, typereq_t *typereq, char *path);

#endif
