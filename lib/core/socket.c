// lib/core/socket.c

#include "core.h"
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

static char g_dns_server[64] = {0};

void modx_set_dns_server(const char *dns)
{
    if (dns) {
        strncpy(g_dns_server, dns, sizeof(g_dns_server) - 1);
        g_dns_server[sizeof(g_dns_server) - 1] = '\0';
    } else {
        g_dns_server[0] = '\0';
    }
}

int modx_resolve_host(const char *host, char *ip_out, size_t ip_size)
{
    struct hostent *he;

    /* 如果设置了自定义 DNS，可以用 getaddrinfo 指定，这里简化处理 */
    /* 实际应使用 getaddrinfo 或直接调用 DNS 协议 */
    he = gethostbyname(host);
    if (!he) return -1;

    strncpy(ip_out, inet_ntoa(*(struct in_addr *)he->h_addr_list[0]), ip_size - 1);
    ip_out[ip_size - 1] = '\0';
    return 0;
}

int modx_tcp_connect(const char *host, int port, char *ip_out, size_t ip_size)
{
    struct hostent *he;
    struct sockaddr_in addr;
    int sock;

    he = gethostbyname(host);
    if (!he) return -1;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr = *(struct in_addr *)he->h_addr_list[0];

    if (ip_out) {
        strncpy(ip_out, inet_ntoa(addr.sin_addr), ip_size - 1);
        ip_out[ip_size - 1] = '\0';
    }

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(sock);
        return -1;
    }

    return sock;
}

void modx_tcp_close(int sock)
{
    if (sock >= 0) close(sock);
}