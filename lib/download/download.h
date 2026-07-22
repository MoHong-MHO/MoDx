#ifndef MODX_DOWNLOAD_H
#define MODX_DOWNLOAD_H

#include <stddef.h>
#include <pthread.h>

/* Thread pool */
struct modx_thread_pool;
struct modx_thread_pool* modx_pool_create(int thread_count);
void modx_pool_destroy(struct modx_thread_pool *pool);

int modx_pool_download(struct modx_thread_pool *pool,
                       const char *url, const char *filename,
                       long long total_size, int thread_count,
                       int max_retries, long long rate_limit,
                       const char *user_agent,
                       volatile int *should_stop,
                       volatile long long *total_downloaded,
                       pthread_mutex_t *progress_mutex,
                       void (*progress_cb)(long long, long long, double, int),
                       int quiet);

/* Progress */
int modx_progress_save(const char *filename, long long total);
long long modx_progress_load(const char *filename);
void modx_progress_remove(const char *filename);
int modx_progress_exists(const char *filename);

int modx_progress_save_thread(const char *filename, int tid,
                              long long start, long long end, long long downloaded);
long long modx_progress_load_thread(const char *filename, int tid,
                                    long long *start, long long *end);
void modx_progress_remove_thread(const char *filename, int tid);
void modx_progress_remove_all(const char *filename, int thread_count);

/* Merge */
int modx_merge_files(const char *filename, int thread_count);

/* Rate limit */
void modx_rate_limit_init(long long bytes_per_sec);
void modx_rate_limit_wait(long long bytes_written);

#endif