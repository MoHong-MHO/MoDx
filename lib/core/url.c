#include "core.h"
#include <string.h>

int modx_parse_url(const char *url, char *host, size_t host_size,
                   char *path, size_t path_size, int *port, int *is_https)
{
    const char *p = url;
    int host_len = 0;

    *is_https = 0;

    if (strncmp(p, "https://", 8) == 0) {
        *is_https = 1;
        p += 8;
        *port = 443;
    } else if (strncmp(p, "http://", 7) == 0) {
        p += 7;
        *port = 80;
    } else {
        return -1;
    }

    while (*p && *p != '/' && *p != ':' && host_len < (int)host_size - 1) {
        host[host_len++] = *p++;
    }
    host[host_len] = '\0';

    if (*p == ':') {
        p++;
        *port = 0;
        while (*p && *p != '/') {
            *port = *port * 10 + (*p - '0');
            p++;
        }
    }

    if (*p == '/') {
        strncpy(path, p, path_size - 1);
        path[path_size - 1] = '\0';
    } else {
        strcpy(path, "/");
    }

    return 0;
}