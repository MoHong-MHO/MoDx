// modx_lib.h - MoDx multi-thread downloader library

#ifndef MODX_LIB_H
#define MODX_LIB_H

#include <stddef.h>

#define MODX_THREAD_COUNT 2
#define MODX_BUFFER_SIZE 8192

typedef struct ModxDownloader *modx_handle;

typedef void (*modx_progress_callback)(long long downloaded, long long total,
                                        double speed, int eta, void *userdata);

modx_handle modx_create(int thread_count);
void modx_destroy(modx_handle handle);

void modx_set_progress_callback(modx_handle handle, modx_progress_callback cb, void *userdata);
void modx_set_user_agent(modx_handle handle, const char *ua);
void modx_set_max_retries(modx_handle handle, int retries);
void modx_set_rate_limit(modx_handle handle, long long bytes_per_second);

long long modx_get_file_size(const char *url);
const char* modx_get_server_ip(const char *url);

int modx_download(modx_handle handle, const char *url, const char *filename);
int modx_can_resume(const char *filename);

long long modx_get_downloaded(modx_handle handle);
long long modx_get_total_size(modx_handle handle);

#endif