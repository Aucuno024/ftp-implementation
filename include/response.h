#ifndef __RESPONSE_H__
#define __RESPONSE_H__
#include "csapp.h"
#include <stdint.h>
#include "request.h"
#include "logs.h"

#define BLOCK_SIZE 512  // Taille d'un bloc pour les transferts

typedef struct {
    uint8_t endian; // 0 pour little endian, 1 pour big endian
    uint8_t content[MAXLINE];
    uint8_t error;
} response_t;

typedef struct {
    uint32_t total_size;   // Taille totale du fichier
    uint16_t block_size;   // Taille d'un bloc (BLOCK_SIZE)
    uint8_t endian;        // 0 pour little endian, 1 pour big endian
    uint8_t error;         // Code d'erreur
} transfer_header_t;

typedef struct {
    uint16_t block_num;    // Numéro du bloc
    uint16_t data_size;    // Taille réelle des données dans ce bloc
    uint8_t data[BLOCK_SIZE];  // Données du bloc
} data_block_t;


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
 * @param error le code d'erreur de la réponse a decoder
 * @return int 0 si le décodage a été effectué 1 sinon
 */
int decode_response(response_t *response, uint8_t *content, uint8_t *error);
/**
 * @fn int send_reponse(int connfd, char path[], typereq_t type);
 * @brief Traite une requete et envoie la reponse appropriee
 * @param connfd le socket ou envoyer la reponse
 * @param path le chemin contenu dans la requete
 * @param type le type de requete
 * @return code d'erreur associe a la reponse
 */
int send_server_response(int connfd, char path[], typereq_t type, log_t *log);

/**
 * @fn void send_error(int connfd, int error)
 * @brief envoie une reponse contenant uniquement un code erreur au client
 * @param connfd le socket où ecrire la reponse
 * @param error le code erreur
 */
void send_error(int connfd, uint8_t error);

/**
 * @brief Envoie un header de transfert pour préparer un envoi par blocs
 * @param connfd socket de connexion
 * @param total_size taille totale du fichier
 * @return 0 si ok, 1 sinon
 */
int send_transfer_header(int connfd, uint32_t total_size);

/**
 * @brief Envoie un bloc de données au client
 * @param connfd socket de connexion
 * @param block_num numéro du bloc
 * @param data pointeur vers les données
 * @param data_size taille des données
 * @return 0 si ok, 1 sinon
 */
int send_data_block(int connfd, uint16_t block_num, const uint8_t *data, uint16_t data_size);

/**
 * @brief Envoie un fichier complet par blocs
 * @param connfd socket de connexion
 * @param path chemin du fichier
 * @param srcdir le dossier source pour les chemins relatifs
 * @return 0 si ok, code erreur sinon
 */
int send_file_by_blocks(int connfd, char path[], char *srcdir);

/**
 * @brief Reçoit un header de transfert depuis le serveur
 * @param connfd socket de connexion
 * @param header pointeur où stocker le header reçu
 * @param rio buffer rio pour les lectures
 * @return 0 si ok, 1 sinon
 */
int receive_transfer_header(int connfd, transfer_header_t *header, rio_t *rio);

/**
 * @brief Reçoit un bloc de données depuis le serveur
 * @param connfd socket de connexion
 * @param block pointeur où stocker le bloc reçu
 * @param rio buffer rio pour les lectures
 * @return 0 si ok, 1 sinon (par exemple si le client s'est déconnecté)
 */
int receive_data_block(int connfd, data_block_t *block, rio_t *rio);

/**
 * @brief Reçoit des blocs et les écrit dans un descripteur de fichier
 * @param rio buffer rio déjà initialisé sur la connexion
 * @param fd descripteur de destination
 * @param total_size taille totale attendue
 * @param start_offset offset de départ déjà présent dans fd
 * @param total_received_out total reçu à la fin
 * @return 0 si ok, code erreur sinon
 */
int receive_file_by_blocks_to_fd(rio_t *rio, int fd, uint32_t total_size, uint32_t start_offset, uint32_t *total_received_out);

/**
 * @brief Reçoit un fichier complet vers un chemin avec base_dir en creant le fichier si nécessaire
 * @param connfd socket de connexion
 * @param path chemin logique reçu de la requête
 * @param base_dir dossier de base pour la résolution
 * @param require_existing 1 si le chemin doit exister, 0 si le chemin peut être créé
 * @param header_out pointeur pour récupérer le header
 * @return 0 si ok, code erreur sinon
 */
int receive_file_by_blocks_to_path(int connfd, char path[], char *base_dir, int require_existing, transfer_header_t *header_out);

/**
 * @brief Reçoit un fichier complet par blocs et l'écrit 
 * @param connfd socket de connexion
 * @param path chemin où écrire le fichier
 * @param header_out pointeur pour récupérer le header
 * @return 0 si ok, code erreur sinon
 */
int receive_file_by_blocks(int connfd, char path[], transfer_header_t *header_out);

/**
 * @brief Reçoit un fichier par blocs avec reprise automatique depuis start_offset
 * @param connfd socket de connexion
 * @param remote_path chemin distant (utilisé dans le .part.meta)
 * @param local_path chemin local cible (sans suffixe .part)
 * @param start_offset offset déjà reçu côté client
 * @param header_out pointeur pour récupérer le header
 * @return 0 si ok, code erreur sinon
 */
int receive_file_by_blocks_resume(int connfd, char remote_path[], char local_path[], uint32_t start_offset, transfer_header_t *header_out);
/**
 * @fn receive_content(int connfd, int out)
 * @brief Recoit un contenu par block sans reprise automatique
 * @param connfd socket de connexion
 * @param outfd sortie du contenu
 * @return 0 si ok, code erreur sinon
 */
int receive_content(int connfd, int outfd);

/**
 * @fn int send_content(int connfd, char *content, size_t size)
 * @brief envoie un contenu quelconque avec une taille defini dans un socket
 * @param connfd le descripteur ou ecrire le contenu
 * @param content le contenu a ecrire
 * @param size la taille du contenu
 * @return NO_ERROR_R si tout va bien, une erreur sinon
 */
int send_content(int connfd, char *content, size_t size);
#define NO_ERROR_R 0
#define PATH_ERROR_R 1
#define TYPE_ERROR_R 2
#define ERROR_READ_REQUEST 3
#define CLIENT_DISCONNECTED_R 4
#define OFFSET_ERROR_R 5
#define SLAVE_ERROR 6
#endif
