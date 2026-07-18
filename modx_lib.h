#ifndef MODX_LIB_H
#define MODX_LIB_H

#include <stddef.h>

#define MODX_BUFFER_SIZE 8192
#define MODX_THREAD_COUNT 2

typedef void* modx_handle;

// Progress callback: called with (total_downloaded, total_size, userdata)
typedef void (*modx_progress_callback)(long long downloaded, long long total, void *userdata);

// Create downloader handle
modx_handle modx_create(int thread_count);

// Destroy downloader
void modx_destroy(modx_handle handle);

// Set progress callback
void modx_set_progress_callback(modx_handle handle, modx_progress_callback cb, void *userdata);

// Get file size from URL
long long modx_get_file_size(const char *url);

// Download file
int modx_download(modx_handle handle, const char *url, const char *filename);

// Get current downloaded bytes
long long modx_get_downloaded(modx_handle handle);

// Get total file size
long long modx_get_total_size(modx_handle handle);

#endif