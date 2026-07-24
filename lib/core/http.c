// lib/core/http.c

#include "core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

#define MAX_HEADER_SIZE 4096
#define MAX_RESPONSE_HEADERS 8192

static char g_last_response_headers[MAX_RESPONSE_HEADERS] = {0};

const char* modx_http_get_last_headers(void)
{
    return g_last_response_headers;
}

static int http_send_request(int sock, struct modx_tls_ctx *tls,
                             const char *method, const char *host,
                             const char *path, const char *range,
                             const char *user_agent)
{
    char req[2048];
    int len;

    len = snprintf(req, sizeof(req),
        "%s %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "User-Agent: %s\r\n"
        "%s"
        "Connection: close\r\n"
        "\r\n",
        method, path, host,
        user_agent ? user_agent : "MoDx/1.7",
        range ? range : "");

    if (tls) {
        return modx_tls_send(tls, req, len);
    } else {
        return send(sock, req, len, 0);
    }
}

/* 修复：将变量名从 recv 改为 n */
static int http_read_headers(int sock, struct modx_tls_ctx *tls, char **out_body, int *body_len)
{
    char buf[4096];
    int total = 0;
    int n;
    char *header_end;

    g_last_response_headers[0] = '\0';

    while (1) {
        if (tls) {
            n = modx_tls_recv(tls, buf + total, sizeof(buf) - total - 1);
        } else {
            n = recv(sock, buf + total, sizeof(buf) - total - 1, 0);
        }
        if (n <= 0) return -1;
        total += n;
        buf[total] = '\0';

        header_end = strstr(buf, "\r\n\r\n");
        if (header_end) {
            int header_len = header_end - buf;
            if (header_len > 0 && header_len < sizeof(g_last_response_headers)) {
                memcpy(g_last_response_headers, buf, header_len);
                g_last_response_headers[header_len] = '\0';
            }
            int body_start = header_end - buf + 4;
            int body_len_total = total - body_start;
            if (body_len_total > 0) {
                if (out_body) {
                    *out_body = malloc(body_len_total + 1);
                    if (*out_body) {
                        memcpy(*out_body, buf + body_start, body_len_total);
                        (*out_body)[body_len_total] = '\0';
                        *body_len = body_len_total;
                    }
                }
                return body_len_total;
            } else {
                if (out_body) *out_body = NULL;
                *body_len = 0;
                return 0;
            }
        }
    }
}

long long modx_http_get_size(const char *url)
{
    char host[256], path[512];
    int port, is_https;
    int sock = -1;
    struct modx_tls_ctx *tls = NULL;
    long long size = -1;

    if (modx_parse_url(url, host, sizeof(host), path, sizeof(path), &port, &is_https) < 0)
        return -1;

    if (is_https) {
        tls = modx_tls_init();
        if (!tls) goto done;
        if (modx_tls_connect(&tls, host, port) < 0) goto done;
    } else {
        sock = modx_tcp_connect(host, port, NULL, 0);
        if (sock < 0) goto done;
    }

    if (http_send_request(sock, tls, "HEAD", host, path, NULL, NULL) < 0)
        goto done;

    char head_buf[4096];
    int received;
    if (is_https) {
        received = modx_tls_recv(tls, head_buf, sizeof(head_buf) - 1);
    } else {
        received = recv(sock, head_buf, sizeof(head_buf) - 1, 0);
    }

    if (received <= 0) goto done;
    head_buf[received] = '\0';

    char *cl = strstr(head_buf, "Content-Length:");
    if (cl) {
        cl += 15;
        size = atoll(cl);
    }

done:
    if (sock >= 0) close(sock);
    if (tls) { modx_tls_close(tls); free(tls); }
    return size;
}

int modx_http_download_range(const char *url,
                             long long start, long long end,
                             const char *user_agent,
                             modx_write_callback cb, void *userdata,
                             int *out_status_code)
{
    char host[256], path[512];
    int port, is_https;
    int sock = -1;
    struct modx_tls_ctx *tls = NULL;
    char range[64];
    int total = 0;
    char *body = NULL;
    int body_len = 0;

    if (modx_parse_url(url, host, sizeof(host), path, sizeof(path), &port, &is_https) < 0)
        return -1;

    if (is_https) {
        tls = modx_tls_init();
        if (!tls) goto done;
        if (modx_tls_connect(&tls, host, port) < 0) goto done;
    } else {
        sock = modx_tcp_connect(host, port, NULL, 0);
        if (sock < 0) goto done;
    }

    snprintf(range, sizeof(range), "Range: bytes=%lld-%lld\r\n", start, end);

    if (http_send_request(sock, tls, "GET", host, path, range, user_agent) < 0)
        goto done;

    body_len = http_read_headers(sock, tls, &body, &body_len);
    if (body_len < 0) goto done;

    if (out_status_code && g_last_response_headers[0]) {
        char *http = strstr(g_last_response_headers, "HTTP/");
        if (http) {
            http += 9;
            *out_status_code = atoi(http);
        }
    }

    if (body_len > 0 && body) {
        if (cb(userdata, body, body_len) < 0) {
            free(body);
            goto done;
        }
        total += body_len;
        free(body);
        body = NULL;
    }

    /* 修复：将变量名从 recv 改为 n */
    char buf[8192];
    while (1) {
        int n;
        if (is_https) {
            n = modx_tls_recv(tls, buf, sizeof(buf));
        } else {
            n = recv(sock, buf, sizeof(buf), 0);
        }
        if (n <= 0) break;
        if (cb(userdata, buf, n) < 0) break;
        total += n;
    }

done:
    if (sock >= 0) close(sock);
    if (tls) { modx_tls_close(tls); free(tls); }
    free(body);
    return total;
}