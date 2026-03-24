#ifndef __LOGS_H__
#define __LOGS_H__
#include "request.h"
typedef struct log {
    struct log *follow;
    typereq_t type;
    char path[MAXBUF];
} log_t;

/**
 * @fn add(log_t *log, typereq_t type, const char *path)
 * @brief ajoute une entree a un journal de log
 * @param log un journal 
 * @param type le type d'evenement
 * @param path l'endroit affecte par l'evenement
 * @return 0 si reussi 1 si echec
 */
int add(log_t **log, typereq_t type, const char *path);

/**
 * @fn void free_log(log_t *log)
 * @brief free l'entierete d'un journal de log
 * @param log le journal a free
 */
void free_log(log_t *log);

/**
 * @fn log_t *follow(log_t *log)
 * @brief renvoie l'entree suivante du journal 
 * @param log un journal
 * @return NULL si le journal est vide ou si il n'y a pas de suivant log_t * sinon
 */
log_t *follow(log_t *log);
#endif