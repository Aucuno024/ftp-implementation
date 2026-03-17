#include "utils.h"
#include "csapp.h"
#include <stdint.h>

int get_endianess() {
    static uint32_t one = 1;
    return ((* (uint8_t *) &one) == 0);
}

int open_file_r(char path[], int *fd)
{
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