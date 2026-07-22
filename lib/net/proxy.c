// lib/net/proxy.c

#include "net.h"
#include "../core/core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

static char g_proxy_url[256] = {0};
static char g_proxy_host[128] = {0};
static int g_proxy_port = 0;

int modx_proxy_set_http(const char *proxy_url)
{
    char *p, *host_start;
    int port = 8080;

    if (!proxy_url) return -1;

    /* 解析 http://host:port 格式 */
    if (strncmp(proxy_url, "http://", 7) == 0) {
        host_start = (char *)proxy_url + 7;
    } else {
        host_start = (char *)proxy_url;
    }

    strncpy(g_proxy_url, proxy_url, sizeof(g_proxy_url) - 1);
    g_proxy_url[sizeof(g_proxy_url) - 1] = '\0';

    /* 解析 host:port */
    p = strchr(host_start, ':');
    if (p) {
        int host_len = p - host_start;
        if (host_len > 0 && host_len < (int)sizeof(g_proxy_host)) {
            memcpy(g_proxy_host, host_start, host_len);
            g_proxy_host[host_len] = '\0';
        }
        port = atoi(p + 1);
        if (port <= 0) port = 8080;
    } else {
        strncpy(g_proxy_host, host_start, sizeof(g_proxy_host) - 1);
        g_proxy_host[sizeof(g_proxy_host) - 1] = '\0';
    }

    g_proxy_port = port;
    return 0;
}

int modx_proxy_set_socks5(const char *proxy_url)
{
    /* 简单复用 HTTP 代理解析，实际 SOCKS5 需要不同实现 */
    return modx_proxy_set_http(proxy_url);
}

void modx_proxy_clear(void)
{
    g_proxy_url[0] = '\0';
    g_proxy_host[0] = '\0';
    g_proxy_port = 0;
}

int modx_proxy_connect(const char *target_host, int target_port)
{
    int sock;
    char buf[512];
    int len;

    if (g_proxy_port == 0) return -1;

    /* 连接到代理服务器 */
    sock = modx_tcp_connect(g_proxy_host, g_proxy_port, NULL, 0);
    if (sock < 0) return -1;

    /* 发送 CONNECT 请求（HTTP 代理） */
    len = snprintf(buf, sizeof(buf),
        "CONNECT %s:%d HTTP/1.1\r\n"
        "Host: %s:%d\r\n"
        "Connection: close\r\n"
        "\r\n",
        target_host, target_port, target_host, target_port);

    if (send(sock, buf, len, 0) < 0) {
        close(sock);
        return -1;
    }

    /* 读取响应，检查是否成功 */
    char resp[256];
    int received = recv(sock, resp, sizeof(resp) - 1, 0);
    if (received <= 0) {
        close(sock);
        return -1;
    }
    resp[received] = '\0';

    if (strstr(resp, "200 Connection established") == NULL) {
        close(sock);
        return -1;
    }

    return sock;
}