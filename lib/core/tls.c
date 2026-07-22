// lib/core/tls.c

#include "core.h"
#include <stdlib.h>
#include <string.h>
#include <mbedtls/net_sockets.h>
#include <mbedtls/ssl.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/error.h>

struct modx_tls_ctx {
    mbedtls_net_context net;
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config conf;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_x509_crt cacert;
    int initialized;
    int verify_enabled;
    char ca_cert_file[256];
};

struct modx_tls_ctx* modx_tls_init(void)
{
    struct modx_tls_ctx *ctx = calloc(1, sizeof(struct modx_tls_ctx));
    if (!ctx) return NULL;

    mbedtls_net_init(&ctx->net);
    mbedtls_ssl_init(&ctx->ssl);
    mbedtls_ssl_config_init(&ctx->conf);
    mbedtls_x509_crt_init(&ctx->cacert);
    mbedtls_entropy_init(&ctx->entropy);
    mbedtls_ctr_drbg_init(&ctx->ctr_drbg);

    ctx->initialized = 0;
    ctx->verify_enabled = 0;
    ctx->ca_cert_file[0] = '\0';
    return ctx;
}

void modx_tls_set_ca_cert(struct modx_tls_ctx *ctx, const char *ca_file)
{
    if (!ctx || !ca_file) return;
    strncpy(ctx->ca_cert_file, ca_file, sizeof(ctx->ca_cert_file) - 1);
    ctx->ca_cert_file[sizeof(ctx->ca_cert_file) - 1] = '\0';
    ctx->verify_enabled = 1;
}

int modx_tls_connect(struct modx_tls_ctx **ctx_ptr, const char *host, int port)
{
    struct modx_tls_ctx *ctx = *ctx_ptr;
    int ret;
    char port_str[8];

    ret = mbedtls_ctr_drbg_seed(&ctx->ctr_drbg, mbedtls_entropy_func,
                                 &ctx->entropy, NULL, 0);
    if (ret) goto fail;

    ret = mbedtls_ssl_config_defaults(&ctx->conf,
                                       MBEDTLS_SSL_IS_CLIENT,
                                       MBEDTLS_SSL_TRANSPORT_STREAM,
                                       MBEDTLS_SSL_PRESET_DEFAULT);
    if (ret) goto fail;

    /* 证书验证 */
    if (ctx->verify_enabled && ctx->ca_cert_file[0]) {
        ret = mbedtls_x509_crt_parse_file(&ctx->cacert, ctx->ca_cert_file);
        if (ret == 0) {
            mbedtls_ssl_conf_authmode(&ctx->conf, MBEDTLS_SSL_VERIFY_REQUIRED);
            mbedtls_ssl_conf_ca_chain(&ctx->conf, &ctx->cacert, NULL);
        } else {
            /* 加载失败，降级为不验证 */
            mbedtls_ssl_conf_authmode(&ctx->conf, MBEDTLS_SSL_VERIFY_NONE);
        }
    } else {
        mbedtls_ssl_conf_authmode(&ctx->conf, MBEDTLS_SSL_VERIFY_NONE);
    }

    mbedtls_ssl_conf_rng(&ctx->conf, mbedtls_ctr_drbg_random, &ctx->ctr_drbg);

    ret = mbedtls_ssl_setup(&ctx->ssl, &ctx->conf);
    if (ret) goto fail;

    ret = mbedtls_ssl_set_hostname(&ctx->ssl, host);
    if (ret) goto fail;

    snprintf(port_str, sizeof(port_str), "%d", port);
    ret = mbedtls_net_connect(&ctx->net, host, port_str, MBEDTLS_NET_PROTO_TCP);
    if (ret) goto fail;

    mbedtls_ssl_set_bio(&ctx->ssl, &ctx->net, mbedtls_net_send, mbedtls_net_recv, NULL);

    do {
        ret = mbedtls_ssl_handshake(&ctx->ssl);
    } while (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE);

    if (ret) goto fail;

    ctx->initialized = 1;
    return 0;

fail:
    modx_tls_close(ctx);
    *ctx_ptr = NULL;
    return -1;
}

int modx_tls_send(struct modx_tls_ctx *ctx, const char *data, int len)
{
    if (!ctx || !ctx->initialized) return -1;
    return mbedtls_ssl_write(&ctx->ssl, (const unsigned char *)data, len);
}

int modx_tls_recv(struct modx_tls_ctx *ctx, char *buf, int len)
{
    if (!ctx || !ctx->initialized) return -1;
    return mbedtls_ssl_read(&ctx->ssl, (unsigned char *)buf, len);
}

void modx_tls_close(struct modx_tls_ctx *ctx)
{
    if (!ctx) return;
    mbedtls_ssl_free(&ctx->ssl);
    mbedtls_ssl_config_free(&ctx->conf);
    mbedtls_x509_crt_free(&ctx->cacert);
    mbedtls_entropy_free(&ctx->entropy);
    mbedtls_ctr_drbg_free(&ctx->ctr_drbg);
    mbedtls_net_free(&ctx->net);
    ctx->initialized = 0;
}