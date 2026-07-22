#ifndef MODX_NET_H
#define MODX_NET_H

/* Proxy */
int modx_proxy_set_http(const char *proxy_url);
int modx_proxy_set_socks5(const char *proxy_url);
void modx_proxy_clear(void);
int modx_proxy_connect(const char *target_host, int target_port);

/* Mirror */
int modx_mirror_add(const char *url);
void modx_mirror_clear(void);
const char* modx_mirror_next(void);
int modx_mirror_count(void);

#endif