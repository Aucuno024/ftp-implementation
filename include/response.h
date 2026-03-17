#ifndef __RESPONSE_H__
#define __RESPONSE_H__
#include "csapp.h"
#include <stdint.h>

typedef struct {
    uint8_t endian; // 0 pour little endian, 1 pour big endian
    uint8_t content[MAXLINE];
} response_t;

/**
 * @brief Permet de swap les endians d'une reponse
 * @param response la réponse a swap
 * @return int 0 si le swap a été effectué 1 sinon
 */
int swap_endian_response(response_t *response);

/**
 * @fn int read_response(response_t *response, int connfd)
 * @brief lit une réponse depuis un descripteur de fichier
 * @param response la variable ou ecrire ce qui est lu du descripteur
 * @param connfd le descripteur de fichier
 * @return 0 si il n'y pas d'erreur 1 sinon.
 */
int read_response(response_t *response, int connfd);

/**
 * @brief Ecrit une réponse dans le socket de connexion
 * @param response la réponse à envoyer
 * @param connfd le socket de connexion
 */
void write_response(response_t *response, int connfd);

/**
 * @brief Encode une réponse en remplissant les champs de la structure response_t
 * @param response la réponse a remplir
 * @param content  le contenu de la réponse a encoder
 * @return int 0 si l'encodage a été effectué 1 sinon
 */
int encode_response(response_t *response, const uint8_t *content);

/**
 * @brief Decode une réponse en lisant les champs de la structure response_t
 * @param response la réponse a decoder
 * @param content  le contenu de la réponse a decoder
 * @return int 0 si le décodage a été effectué 1 sinon
 */
int decode_response(response_t *response, uint8_t *content);

#endif
