#include "utils.h"
#include "csapp.h"
#include <stdint.h>
#include "string.h"


int get_endianess() {
    static uint32_t one = 1;
    return ((* (uint8_t *) &one) == 0);
}

int open_file_r(char path[], int *fd)
{
    if(is_relative_path(path))
    {
        char *newpath = malloc(strlen(path) + strlen(DEFAULT_SERVER_DIR) + 1);
        int i;
        for(i = 0; DEFAULT_SERVER_DIR[i] != '\0'; i++)
        {
            newpath[i] = DEFAULT_SERVER_DIR[i];
        }
        for(int j = 0; path[j] != '\0'; j++)
        {
            newpath[i++] = path[j];
        }
        newpath[i] = '\0';
        int r = (*fd = open(newpath, O_RDONLY, 0)) != -1;
        free(newpath);
        return r;
    }
    return (*fd = open(path, O_RDONLY, 0)) != -1;
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
