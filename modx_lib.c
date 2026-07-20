// modx_lib.c - MoDx multi-thread downloader library

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

#define HTTP_PORT 80
#define HTTPS_PORT 443
#define MAX_HEADER_SIZE 4096
#define MAX_RETRIES 3
#define PROGRESS_SUFFIX ".progress"

// Thread data
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
    pthread_t thread;
    struct ModxDownloader *downloader;
} DownloadThread;

// Main downloader structure
typedef struct ModxDownloader {
    DownloadThread *threads;
    int thread_count;
    long long total_size;
    volatile long long total_downloaded;
    volatile int should_stop;
    volatile int error_flag;
    pthread_mutex_t file_mutex;
    pthread_mutex_t progress_mutex;
    modx_progress_callback progress_cb;
    void *progress_userdata;
} ModxDownloader;

// ---------- Progress file helpers ----------

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

// ---------- HTTP helpers ----------

static int parse_url(const char *url, char *host, char *path, int *port) {
    const char *ptr = url;
    if (strncmp(ptr, "https://", 8) == 0) {
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

static int connect_to_host(const char *host, int port) {
    struct hostent *he = gethostbyname(host);
    if (!he) return -1;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr = *(struct in_addr *)he->h_addr_list[0];

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(sock);
        return -1;
    }

    return sock;
}

long long modx_get_file_size(const char *url) {
    char host[256] = {0}, path[512] = {0};
    int port = 80;

    if (parse_url(url, host, path, &port) < 0) return -1;

    int sock = connect_to_host(host, port);
    if (sock < 0) return -1;

    char request[1024];
    snprintf(request, sizeof(request),
        "HEAD %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", path, host);

    if (send(sock, request, strlen(request), 0) < 0) {
        close(sock);
        return -1;
    }

    char response[MAX_HEADER_SIZE];
    int received = recv(sock, response, sizeof(response) - 1, 0);
    close(sock);

    if (received <= 0) return -1;
    response[received] = '\0';

    char *cl = strstr(response, "Content-Length:");
    if (cl) {
        cl += 15;
        return atoll(cl);
    }

    return -1;
}

static int download_chunk(const char *url, char *buffer, long long start, long long end) {
    char host[256] = {0}, path[512] = {0};
    int port = 80;

    if (parse_url(url, host, path, &port) < 0) return -1;

    int sock = connect_to_host(host, port);
    if (sock < 0) return -1;

    char request[1024];
    snprintf(request, sizeof(request),
        "GET %s HTTP/1.1\r\nHost: %s\r\nRange: bytes=%lld-%lld\r\nConnection: close\r\n\r\n",
        path, host, start, end);

    if (send(sock, request, strlen(request), 0) < 0) {
        close(sock);
        return -1;
    }

    char response[MAX_HEADER_SIZE];
    int total_received = 0;
    int header_end = 0;

    while (1) {
        int received = recv(sock, response, sizeof(response), 0);
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

    close(sock);
    return total_received;
}

// ---------- File writing ----------

static int write_data(const char *filename, const char *data, int len, long long offset, pthread_mutex_t *mutex) {
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

// ---------- Download thread ----------

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

    while (remaining > 0 && !dl->should_stop) {
        long long chunk_size = (remaining > MODX_BUFFER_SIZE) ? MODX_BUFFER_SIZE : remaining;
        int downloaded = download_chunk(td->url, buffer, offset, offset + chunk_size - 1);

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
        pthread_mutex_unlock(&dl->progress_mutex);

        if (dl->progress_cb && total > 0) {
            dl->progress_cb(dl->total_downloaded, total, dl->progress_userdata);
        }

        // Save thread progress
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

// ---------- Merge temp files ----------

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

// ---------- Public API ----------

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
    dl->progress_cb = NULL;
    dl->progress_userdata = NULL;

    pthread_mutex_init(&dl->file_mutex, NULL);
    pthread_mutex_init(&dl->progress_mutex, NULL);

    return (modx_handle)dl;
}

void modx_destroy(modx_handle handle) {
    ModxDownloader *dl = (ModxDownloader *)handle;
    if (!dl) return;

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

long long modx_get_downloaded(modx_handle handle) {
    ModxDownloader *dl = (ModxDownloader *)handle;
    return dl ? dl->total_downloaded : 0;
}

long long modx_get_total_size(modx_handle handle) {
    ModxDownloader *dl = (ModxDownloader *)handle;
    return dl ? dl->total_size : 0;
}

int modx_download(modx_handle handle, const char *url, const char *filename) {
    ModxDownloader *dl = (ModxDownloader *)handle;
    if (!dl || !url || !filename) return -1;

    // Check for existing progress
    long long existing = load_total_progress(filename);
    if (existing > 0) {
        printf("[Resume] Continuing from %lld bytes\n", existing);
        dl->total_downloaded = existing;
    }

    // Get file size
    dl->total_size = modx_get_file_size(url);
    if (dl->total_size <= 0) {
        return -1;
    }

    if (existing >= dl->total_size) {
        remove_total_progress(filename);
        remove_all_progress(filename, dl->thread_count);
        return 0;
    }

    // Pre-allocate file
    FILE *fp = fopen(filename, "rb+");
    if (!fp) {
        fp = fopen(filename, "wb");
        if (!fp) return -1;
        fseek(fp, dl->total_size - 1, SEEK_SET);
        fputc(0, fp);
    }
    fclose(fp);

    // Initialize threads
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

        // Load thread progress if resuming
        long long start, end;
        long long thread_done = load_thread_progress(filename, i, &start, &end);
        if (thread_done > 0) {
            td->start_byte = start;
            td->end_byte = end;
            td->downloaded = thread_done;
            dl->total_downloaded += thread_done;
            printf("[Resume] Thread %d: %lld/%lld bytes\n", i, thread_done, end - start + 1);
        }
    }

    dl->should_stop = 0;
    dl->error_flag = 0;

    // Start threads
    for (int i = 0; i < dl->thread_count; i++) {
        pthread_create(&dl->threads[i].thread, NULL, download_thread_func, &dl->threads[i]);
    }

    // Wait for threads
    for (int i = 0; i < dl->thread_count; i++) {
        pthread_join(dl->threads[i].thread, NULL);
    }

    // Check result
    if (dl->error_flag || dl->total_downloaded < dl->total_size) {
        save_total_progress(filename, dl->total_downloaded);
        return -1;
    }

    // Merge thread files
    if (!merge_thread_files(filename, dl->thread_count)) {
        return -1;
    }

    // Cleanup
    remove_total_progress(filename);
    remove_all_progress(filename, dl->thread_count);
    return 0;
}