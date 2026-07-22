#ifndef MODX_CORE_H
#define MODX_CORE_H

#include <stddef.h>

/* URL parsing */
int modx_parse_url(const char *url, char *host, size_t host_size,
                   char *path, size_t path_size, int *port, int *is_https);

/* Socket */
int modx_tcp_connect(const char *host, int port, char *ip_out, size_t ip_size);
void modx_tcp_close(int sock);

/* HTTP */
long long modx_http_get_size(const char *url);
const char* modx_http_get_last_headers(void);

/* 写回调类型 */
typedef int (*modx_write_callback)(void *userdata, const char *data, int len);
int modx_http_download_range(const char *url,
                             long long start, long long end,
                             const char *user_agent,
                             modx_write_callback cb, void *userdata,
                             int *out_status_code);

/* TLS (mbedTLS) - opaque context */
struct modx_tls_ctx;
struct modx_tls_ctx* modx_tls_init(void);
int modx_tls_connect(struct modx_tls_ctx **ctx, const char *host, int port);
int modx_tls_send(struct modx_tls_ctx *ctx, const char *data, int len);
int modx_tls_recv(struct modx_tls_ctx *ctx, char *buf, int len);
void modx_tls_close(struct modx_tls_ctx *ctx);

/* DNS (custom) */
int modx_resolve_host(const char *host, char *ip_out, size_t ip_size);

#endif