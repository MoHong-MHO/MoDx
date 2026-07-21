// modx_lib.c - MoDx multi-thread downloader library with HTTPS support

#include "modx_lib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <time.h>

/* mbedTLS headers */
#include <mbedtls/net_sockets.h>
#include <mbedtls/ssl.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/error.h>

#define HTTP_PORT 80
#define HTTPS_PORT 443
#define MAX_HEADER_SIZE 4096
#define MAX_RETRIES 3
#define PROGRESS_SUFFIX ".progress"

typedef struct {
    int id;
    char url[512];
    char filename[256];
    char tmp_filename[512];
    long long start_byte;
    long long end_byte;
    long long downloaded;
    int complete;
    int error;
    int is_https;
    pthread_t thread;
    struct ModxDownloader *downloader;
} DownloadThread;

typedef struct ModxDownloader {
    DownloadThread *threads;
    int thread_count;
    long long total_size;
    volatile long long total_downloaded;
    volatile int should_stop;
    volatile int error_flag;
    char user_agent[256];
    char server_ip[64];
    pthread_mutex_t file_mutex;
    pthread_mutex_t progress_mutex;
    modx_progress_callback progress_cb;
    void *progress_userdata;
    time_t last_speed_time;
    long long last_speed_bytes;
    double current_speed;
    /* mbedTLS contexts */
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config ssl_conf;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_x509_crt cacert;
    mbedtls_net_context server_fd;
    int tls_initialized;
} ModxDownloader;

/* ---------- Progress file helpers ---------- */
static void save_thread_progress(DownloadThread *td) {
    char progress_path[512];
    snprintf(progress_path, sizeof(progress_path), "%s.progress.%d", td->filename, td->id);
    FILE *fp = fopen(progress_path, "w");
    if (!fp) return;
    fprintf(fp, "%lld %lld %lld", td->start_byte, td->end_byte, td->downloaded);
    fclose(fp);
}

static long long load_thread_progress(const char *filename, int id, long long *start, long long *end) {
    char progress_path[512];
    snprintf(progress_path, sizeof(progress_path), "%s.progress.%d", filename, id);
    FILE *fp = fopen(progress_path, "r");
    if (!fp) return -1;
    long long downloaded = 0;
    if (fscanf(fp, "%lld %lld %lld", start, end, &downloaded) != 3) {
        fclose(fp);
        return -1;
    }
    fclose(fp);
    return downloaded;
}

static void remove_thread_progress(const char *filename, int id) {
    char progress_path[512];
    snprintf(progress_path, sizeof(progress_path), "%s.progress.%d", filename, id);
    remove(progress_path);
}

static void remove_all_progress(const char *filename, int thread_count) {
    for (int i = 0; i < thread_count; i++) {
        remove_thread_progress(filename, i);
    }
}

static void save_total_progress(const char *filename, long long downloaded) {
    char progress_path[512];
    snprintf(progress_path, sizeof(progress_path), "%s.progress", filename);
    FILE *fp = fopen(progress_path, "w");
    if (!fp) return;
    fprintf(fp, "%lld", downloaded);
    fclose(fp);
}

static long long load_total_progress(const char *filename) {
    char progress_path[512];
    snprintf(progress_path, sizeof(progress_path), "%s.progress", filename);
    FILE *fp = fopen(progress_path, "r");
    if (!fp) return -1;
    long long downloaded = 0;
    if (fscanf(fp, "%lld", &downloaded) != 1) {
        fclose(fp);
        return -1;
    }
    fclose(fp);
    return downloaded;
}

static void remove_total_progress(const char *filename) {
    char progress_path[512];
    snprintf(progress_path, sizeof(progress_path), "%s.progress", filename);
    remove(progress_path);
}

int modx_can_resume(const char *filename) {
    return load_total_progress(filename) >= 0;
}

/* ---------- URL parsing ---------- */
static int parse_url(const char *url, char *host, char *path, int *port, int *is_https) {
    const char *ptr = url;
    *is_https = 0;

    if (strncmp(ptr, "https://", 8) == 0) {
        *is_https = 1;
        ptr += 8;
        *port = HTTPS_PORT;
    } else if (strncmp(ptr, "http://", 7) == 0) {
        ptr += 7;
        *port = HTTP_PORT;
    } else {
        return -1;
    }

    int host_len = 0;
    while (*ptr && *ptr != '/' && *ptr != ':' && host_len < 255) {
        host[host_len++] = *ptr++;
    }
    host[host_len] = '\0';

    if (*ptr == ':') {
        ptr++;
        *port = 0;
        while (*ptr && *ptr != '/') {
            *port = *port * 10 + (*ptr - '0');
            ptr++;
        }
    }

    if (*ptr == '/') {
        strcpy(path, ptr);
    } else {
        strcpy(path, "/");
    }

    return 0;
}

static int tcp_connect(const char *host, int port, char *ip_out) {
    struct hostent *he = gethostbyname(host);
    if (!he) return -1;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr = *(struct in_addr *)he->h_addr_list[0];

    if (ip_out) {
        strncpy(ip_out, inet_ntoa(addr.sin_addr), 63);
        ip_out[63] = '\0';
    }

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(sock);
        return -1;
    }

    return sock;
}

/* ---------- mbedTLS HTTPS ---------- */
static int tls_handshake(ModxDownloader *dl, const char *host, int port) {
    int ret;
    char port_str[8];

    mbedtls_net_init(&dl->server_fd);
    mbedtls_ssl_init(&dl->ssl);
    mbedtls_ssl_config_init(&dl->ssl_conf);
    mbedtls_x509_crt_init(&dl->cacert);
    mbedtls_entropy_init(&dl->entropy);
    mbedtls_ctr_drbg_init(&dl->ctr_drbg);

    ret = mbedtls_ctr_drbg_seed(&dl->ctr_drbg, mbedtls_entropy_func,
                                 &dl->entropy, NULL, 0);
    if (ret != 0) {
        dl->tls_initialized = 0;
        return -1;
    }

    ret = mbedtls_ssl_config_defaults(&dl->ssl_conf,
                                       MBEDTLS_SSL_IS_CLIENT,
                                       MBEDTLS_SSL_TRANSPORT_STREAM,
                                       MBEDTLS_SSL_PRESET_DEFAULT);
    if (ret != 0) {
        dl->tls_initialized = 0;
        return -1;
    }

    /* Skip certificate verification for simplicity */
    mbedtls_ssl_conf_authmode(&dl->ssl_conf, MBEDTLS_SSL_VERIFY_NONE);
    mbedtls_ssl_conf_rng(&dl->ssl_conf, mbedtls_ctr_drbg_random, &dl->ctr_drbg);

    ret = mbedtls_ssl_setup(&dl->ssl, &dl->ssl_conf);
    if (ret != 0) {
        dl->tls_initialized = 0;
        return -1;
    }

    ret = mbedtls_ssl_set_hostname(&dl->ssl, host);
    if (ret != 0) {
        dl->tls_initialized = 0;
        return -1;
    }

    snprintf(port_str, sizeof(port_str), "%d", port);
    ret = mbedtls_net_connect(&dl->server_fd, host, port_str, MBEDTLS_NET_PROTO_TCP);
    if (ret != 0) {
        dl->tls_initialized = 0;
        return -1;
    }

    mbedtls_ssl_set_bio(&dl->ssl, &dl->server_fd,
                        mbedtls_net_send, mbedtls_net_recv, NULL);

    do {
        ret = mbedtls_ssl_handshake(&dl->ssl);
    } while (ret == MBEDTLS_ERR_SSL_WANT_READ ||
             ret == MBEDTLS_ERR_SSL_WANT_WRITE);

    if (ret != 0) {
        dl->tls_initialized = 0;
        return -1;
    }

    dl->tls_initialized = 1;
    return 0;
}

static int tls_send(ModxDownloader *dl, const char *data, int len) {
    if (!dl->tls_initialized) return -1;
    return mbedtls_ssl_write(&dl->ssl, (const unsigned char *)data, len);
}

static int tls_recv(ModxDownloader *dl, char *buf, int len) {
    if (!dl->tls_initialized) return -1;
    return mbedtls_ssl_read(&dl->ssl, (unsigned char *)buf, len);
}

static void tls_close(ModxDownloader *dl) {
    if (!dl->tls_initialized) return;
    mbedtls_ssl_free(&dl->ssl);
    mbedtls_ssl_config_free(&dl->ssl_conf);
    mbedtls_x509_crt_free(&dl->cacert);
    mbedtls_entropy_free(&dl->entropy);
    mbedtls_ctr_drbg_free(&dl->ctr_drbg);
    mbedtls_net_free(&dl->server_fd);
    dl->tls_initialized = 0;
}

/* ---------- HTTP/HTTPS request ---------- */
static long long http_get_file_size(const char *url, int *is_https) {
    char host[256] = {0}, path[512] = {0};
    int port = 80;
    char ip[64];
    ModxDownloader tmp_dl = {0};

    if (parse_url(url, host, path, &port, is_https) < 0) return -1;

    if (*is_https) {
        if (tls_handshake(&tmp_dl, host, port) < 0) {
            tls_close(&tmp_dl);
            return -1;
        }
    }

    int sock = -1;
    if (!*is_https) {
        sock = tcp_connect(host, port, NULL);
        if (sock < 0) return -1;
    }

    char request[1024];
    snprintf(request, sizeof(request),
        "HEAD %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n",
        path, host);

    int sent;
    if (*is_https) {
        sent = tls_send(&tmp_dl, request, strlen(request));
    } else {
        sent = send(sock, request, strlen(request), 0);
    }
    if (sent < 0) {
        if (!*is_https) close(sock);
        tls_close(&tmp_dl);
        return -1;
    }

    char response[MAX_HEADER_SIZE];
    int received;
    if (*is_https) {
        received = tls_recv(&tmp_dl, response, sizeof(response) - 1);
    } else {
        received = recv(sock, response, sizeof(response) - 1, 0);
    }

    if (!*is_https) close(sock);
    tls_close(&tmp_dl);

    if (received <= 0) return -1;
    response[received] = '\0';

    char *cl = strstr(response, "Content-Length:");
    if (cl) {
        cl += 15;
        return atoll(cl);
    }

    return -1;
}

static int http_download_chunk(const char *url, char *buffer, long long start,
                                long long end, const char *user_agent,
                                int *is_https, ModxDownloader *dl) {
    char host[256] = {0}, path[512] = {0};
    int port = 80;
    char ip[64];

    if (parse_url(url, host, path, &port, is_https) < 0) return -1;

    int sock = -1;
    if (*is_https) {
        if (tls_handshake(dl, host, port) < 0) {
            tls_close(dl);
            return -1;
        }
    } else {
        sock = tcp_connect(host, port, NULL);
        if (sock < 0) return -1;
    }

    char request[2048];
    snprintf(request, sizeof(request),
        "GET %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Range: bytes=%lld-%lld\r\n"
        "User-Agent: %s\r\n"
        "Connection: close\r\n"
        "\r\n",
        path, host, start, end, user_agent ? user_agent : "MoDx/1.5");

    int sent;
    if (*is_https) {
        sent = tls_send(dl, request, strlen(request));
    } else {
        sent = send(sock, request, strlen(request), 0);
    }
    if (sent < 0) {
        if (!*is_https) close(sock);
        tls_close(dl);
        return -1;
    }

    char response[MAX_HEADER_SIZE];
    int total_received = 0;
    int header_end = 0;

    while (1) {
        int received;
        if (*is_https) {
            received = tls_recv(dl, response, sizeof(response));
        } else {
            received = recv(sock, response, sizeof(response), 0);
        }
        if (received <= 0) break;

        if (!header_end) {
            char *body = strstr(response, "\r\n\r\n");
            if (body) {
                header_end = 1;
                body += 4;
                int body_len = received - (body - response);
                memcpy(buffer + total_received, body, body_len);
                total_received += body_len;
            }
        } else {
            memcpy(buffer + total_received, response, received);
            total_received += received;
        }
    }

    if (!*is_https) close(sock);
    tls_close(dl);

    return total_received;
}

/* ---------- File writing ---------- */
static int write_data(const char *filename, const char *data, int len,
                       long long offset, pthread_mutex_t *mutex) {
    pthread_mutex_lock(mutex);

    FILE *fp = fopen(filename, "r+b");
    if (!fp) {
        fp = fopen(filename, "w+b");
        if (!fp) {
            pthread_mutex_unlock(mutex);
            return 0;
        }
    }

    if (fseek(fp, offset, SEEK_SET) != 0) {
        fclose(fp);
        pthread_mutex_unlock(mutex);
        return 0;
    }

    size_t written = fwrite(data, 1, len, fp);
    fclose(fp);

    pthread_mutex_unlock(mutex);
    return (int)written;
}

/* ---------- Download thread ---------- */
static void* download_thread_func(void *arg) {
    DownloadThread *td = (DownloadThread *)arg;
    ModxDownloader *dl = td->downloader;

    char *buffer = malloc(MODX_BUFFER_SIZE);
    if (!buffer) {
        td->complete = 1;
        td->error = 1;
        return NULL;
    }

    long long remaining = td->end_byte - td->start_byte + 1;
    long long offset = td->start_byte + td->downloaded;
    int retries = 0;
    int is_https = 0;

    while (remaining > 0 && !dl->should_stop) {
        long long chunk_size = (remaining > MODX_BUFFER_SIZE) ? MODX_BUFFER_SIZE : remaining;
        int downloaded = http_download_chunk(td->url, buffer, offset,
                                              offset + chunk_size - 1,
                                              dl->user_agent, &is_https, dl);

        if (downloaded <= 0) {
            retries++;
            if (retries >= MAX_RETRIES) {
                td->error = 1;
                dl->error_flag = 1;
                dl->should_stop = 1;
                break;
            }
            sleep(1);
            continue;
        }

        int written = write_data(td->filename, buffer, downloaded, offset, &dl->file_mutex);

        if (written != downloaded) {
            td->error = 1;
            dl->error_flag = 1;
            dl->should_stop = 1;
            break;
        }

        td->downloaded += written;

        pthread_mutex_lock(&dl->progress_mutex);
        dl->total_downloaded += written;
        long long total = dl->total_size;

        time_t now = time(NULL);
        if (dl->last_speed_time != 0) {
            double elapsed = difftime(now, dl->last_speed_time);
            if (elapsed >= 0.5) {
                long long delta = dl->total_downloaded - dl->last_speed_bytes;
                dl->current_speed = delta / elapsed;
                dl->last_speed_bytes = dl->total_downloaded;
                dl->last_speed_time = now;
            }
        } else {
            dl->last_speed_bytes = dl->total_downloaded;
            dl->last_speed_time = now;
        }
        pthread_mutex_unlock(&dl->progress_mutex);

        if (dl->progress_cb && total > 0) {
            int eta = 0;
            if (dl->current_speed > 0) {
                eta = (int)((total - dl->total_downloaded) / dl->current_speed);
            }
            dl->progress_cb(dl->total_downloaded, total, dl->current_speed, eta, dl->progress_userdata);
        }

        save_thread_progress(td);
        save_total_progress(td->filename, dl->total_downloaded);

        offset += chunk_size;
        remaining -= chunk_size;
        retries = 0;
    }

    free(buffer);
    td->complete = 1;
    return NULL;
}

/* ---------- Merge temp files ---------- */
static int merge_thread_files(const char *filename, int thread_count) {
    FILE *final_fp = fopen(filename, "wb");
    if (!final_fp) return 0;

    for (int i = 0; i < thread_count; i++) {
        char tmp_path[512];
        snprintf(tmp_path, sizeof(tmp_path), "%s.tmp.%d", filename, i);
        FILE *tmp_fp = fopen(tmp_path, "rb");
        if (!tmp_fp) {
            fclose(final_fp);
            return 0;
        }
        char buf[MODX_BUFFER_SIZE];
        size_t n;
        while ((n = fread(buf, 1, sizeof(buf), tmp_fp)) > 0) {
            if (fwrite(buf, 1, n, final_fp) != n) {
                fclose(tmp_fp);
                fclose(final_fp);
                return 0;
            }
        }
        fclose(tmp_fp);
        remove(tmp_path);
    }

    fclose(final_fp);
    return 1;
}

/* ---------- Public API ---------- */
modx_handle modx_create(int thread_count) {
    if (thread_count < 1) thread_count = 1;

    ModxDownloader *dl = malloc(sizeof(ModxDownloader));
    if (!dl) return NULL;

    dl->threads = malloc(sizeof(DownloadThread) * thread_count);
    if (!dl->threads) {
        free(dl);
        return NULL;
    }

    dl->thread_count = thread_count;
    dl->total_size = 0;
    dl->total_downloaded = 0;
    dl->should_stop = 0;
    dl->error_flag = 0;
    strcpy(dl->user_agent, "MoDx/1.5");
    dl->server_ip[0] = '\0';
    dl->last_speed_time = 0;
    dl->last_speed_bytes = 0;
    dl->current_speed = 0;
    dl->progress_cb = NULL;
    dl->progress_userdata = NULL;
    dl->tls_initialized = 0;

    pthread_mutex_init(&dl->file_mutex, NULL);
    pthread_mutex_init(&dl->progress_mutex, NULL);

    return (modx_handle)dl;
}

void modx_destroy(modx_handle handle) {
    ModxDownloader *dl = (ModxDownloader *)handle;
    if (!dl) return;

    tls_close(dl);
    pthread_mutex_destroy(&dl->file_mutex);
    pthread_mutex_destroy(&dl->progress_mutex);
    free(dl->threads);
    free(dl);
}

void modx_set_progress_callback(modx_handle handle, modx_progress_callback cb, void *userdata) {
    ModxDownloader *dl = (ModxDownloader *)handle;
    if (!dl) return;
    dl->progress_cb = cb;
    dl->progress_userdata = userdata;
}

void modx_set_user_agent(modx_handle handle, const char *ua) {
    ModxDownloader *dl = (ModxDownloader *)handle;
    if (!dl || !ua) return;
    strncpy(dl->user_agent, ua, sizeof(dl->user_agent) - 1);
    dl->user_agent[sizeof(dl->user_agent) - 1] = '\0';
}

long long modx_get_downloaded(modx_handle handle) {
    ModxDownloader *dl = (ModxDownloader *)handle;
    return dl ? dl->total_downloaded : 0;
}

long long modx_get_total_size(modx_handle handle) {
    ModxDownloader *dl = (ModxDownloader *)handle;
    return dl ? dl->total_size : 0;
}

const char* modx_get_server_ip(const char *url) {
    static char ip[64];
    char host[256] = {0}, path[512] = {0};
    int port = 80, is_https = 0;

    if (parse_url(url, host, path, &port, &is_https) < 0) return NULL;

    int sock = tcp_connect(host, port, ip);
    if (sock < 0) return NULL;
    close(sock);

    return ip;
}

long long modx_get_file_size(const char *url) {
    int is_https = 0;
    return http_get_file_size(url, &is_https);
}

int modx_download(modx_handle handle, const char *url, const char *filename) {
    ModxDownloader *dl = (ModxDownloader *)handle;
    if (!dl || !url || !filename) return -1;

    const char *ip = modx_get_server_ip(url);
    if (ip) {
        strcpy(dl->server_ip, ip);
        printf("[Server IP] %s\n", ip);
    }

    long long existing = load_total_progress(filename);
    if (existing > 0) {
        printf("[Resume] Continuing from %lld bytes\n", existing);
        dl->total_downloaded = existing;
    }

    int is_https = 0;
    dl->total_size = http_get_file_size(url, &is_https);
    if (dl->total_size <= 0) {
        return -1;
    }

    if (existing >= dl->total_size) {
        remove_total_progress(filename);
        remove_all_progress(filename, dl->thread_count);
        return 0;
    }

    FILE *fp = fopen(filename, "rb+");
    if (!fp) {
        fp = fopen(filename, "wb");
        if (!fp) return -1;
        fseek(fp, dl->total_size - 1, SEEK_SET);
        fputc(0, fp);
    }
    fclose(fp);

    long long seg = dl->total_size / dl->thread_count;
    for (int i = 0; i < dl->thread_count; i++) {
        DownloadThread *td = &dl->threads[i];
        td->id = i;
        strncpy(td->url, url, sizeof(td->url) - 1);
        td->url[sizeof(td->url) - 1] = '\0';
        strncpy(td->filename, filename, sizeof(td->filename) - 1);
        td->filename[sizeof(td->filename) - 1] = '\0';
        snprintf(td->tmp_filename, sizeof(td->tmp_filename), "%s.tmp.%d", filename, i);
        td->start_byte = (long long)i * seg;
        td->end_byte = (i == dl->thread_count - 1) ?
            dl->total_size - 1 : (long long)(i + 1) * seg - 1;
        td->downloaded = 0;
        td->complete = 0;
        td->error = 0;
        td->downloader = dl;
        td->is_https = is_https;

        long long start, end;
        long long thread_done = load_thread_progress(filename, i, &start, &end);
        if (thread_done > 0) {
            td->start_byte = start;
            td->end_byte = end;
            td->downloaded = thread_done;
            dl->total_downloaded += thread_done;
        }
    }

    dl->should_stop = 0;
    dl->error_flag = 0;
    dl->last_speed_time = 0;
    dl->last_speed_bytes = 0;
    dl->current_speed = 0;

    for (int i = 0; i < dl->thread_count; i++) {
        pthread_create(&dl->threads[i].thread, NULL, download_thread_func, &dl->threads[i]);
    }

    for (int i = 0; i < dl->thread_count; i++) {
        pthread_join(dl->threads[i].thread, NULL);
    }

    if (dl->error_flag || dl->total_downloaded < dl->total_size) {
        save_total_progress(filename, dl->total_downloaded);
        return -1;
    }

    if (!merge_thread_files(filename, dl->thread_count)) {
        return -1;
    }

    remove_total_progress(filename);
    remove_all_progress(filename, dl->thread_count);
    return 0;
}