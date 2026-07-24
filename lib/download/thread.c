// lib/download/thread.c

#include "download.h"
#include "../core/core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

struct modx_thread_pool {
    pthread_t *threads;
    int count;
    int running;
    int active;
};

struct download_task {
    char url[512];
    char filename[256];
    long long start_byte;
    long long end_byte;
    long long downloaded;
    int id;
    int max_retries;
    long long rate_limit;
    char user_agent[256];
    volatile int *should_stop;
    volatile long long *total_downloaded;
    pthread_mutex_t *progress_mutex;
    long long total_size;
    int quiet;
    void (*progress_cb)(long long, long long, double, int);
};

static int write_to_file(void *userdata, const char *data, int len)
{
    struct download_task *task = (struct download_task *)userdata;
    FILE *fp;
    int written;

    fp = fopen(task->filename, "r+b");
    if (!fp) {
        fp = fopen(task->filename, "w+b");
        if (!fp) return -1;
    }

    long long offset = task->start_byte + task->downloaded;
    if (fseek(fp, offset, SEEK_SET) != 0) {
        fclose(fp);
        return -1;
    }

    written = fwrite(data, 1, len, fp);
    fclose(fp);

    if (written != len) return -1;

    task->downloaded += written;

    pthread_mutex_lock(task->progress_mutex);
    *task->total_downloaded += written;
    pthread_mutex_unlock(task->progress_mutex);

    modx_rate_limit_wait(written);

    return 0;
}

static void* download_thread_func(void *arg)
{
    struct download_task *task = (struct download_task *)arg;
    long long remaining = task->end_byte - task->start_byte + 1;
    long long offset = task->start_byte + task->downloaded;
    int retries = 0;
    int status_code = 0;  /* 修复：用 int 变量接收状态码 */

    while (remaining > 0 && !(*task->should_stop)) {
        long long chunk_size = (remaining > 8192) ? 8192 : remaining;
        long long end = offset + chunk_size - 1;

        /* 修复：传 &status_code (int*) 而不是 &headers (char**) */
        int downloaded = modx_http_download_range(task->url,
                                                   offset, end,
                                                   task->user_agent,
                                                   write_to_file, task,
                                                   &status_code);

        if (downloaded <= 0) {
            retries++;
            if (retries >= task->max_retries) {
                fprintf(stderr, "[Thread %d] Failed after %d retries\n",
                        task->id, retries);
                return NULL;
            }
            sleep(1 << retries);
            continue;
        }

        retries = 0;
        offset += downloaded;
        remaining -= downloaded;
    }

    return NULL;
}

struct modx_thread_pool* modx_pool_create(int thread_count)
{
    struct modx_thread_pool *pool = calloc(1, sizeof(struct modx_thread_pool));
    if (!pool) return NULL;
    pool->threads = calloc(thread_count, sizeof(pthread_t));
    if (!pool->threads) {
        free(pool);
        return NULL;
    }
    pool->count = thread_count;
    pool->running = 0;
    pool->active = 0;
    return pool;
}

void modx_pool_destroy(struct modx_thread_pool *pool)
{
    if (!pool) return;
    free(pool->threads);
    free(pool);
}

int modx_pool_download(struct modx_thread_pool *pool,
                       const char *url, const char *filename,
                       long long total_size, int thread_count,
                       int max_retries, long long rate_limit,
                       const char *user_agent,
                       volatile int *should_stop,
                       volatile long long *total_downloaded,
                       pthread_mutex_t *progress_mutex,
                       void (*progress_cb)(long long, long long, double, int),
                       int quiet)
{
    if (!pool || !url || !filename || total_size <= 0) return -1;

    long long seg = total_size / thread_count;
    struct download_task *tasks = calloc(thread_count, sizeof(struct download_task));
    if (!tasks) return -1;

    pool->active = thread_count;

    for (int i = 0; i < thread_count; i++) {
        struct download_task *task = &tasks[i];
        task->id = i;
        strncpy(task->url, url, sizeof(task->url) - 1);
        strncpy(task->filename, filename, sizeof(task->filename) - 1);
        task->start_byte = (long long)i * seg;
        task->end_byte = (i == thread_count - 1) ?
            total_size - 1 : (long long)(i + 1) * seg - 1;

        long long start, end;
        long long done = modx_progress_load_thread(filename, i, &start, &end);
        if (done > 0) {
            task->start_byte = start;
            task->end_byte = end;
            task->downloaded = done;
            *total_downloaded += done;
        } else {
            task->downloaded = 0;
        }

        task->max_retries = max_retries;
        task->rate_limit = rate_limit;
        strncpy(task->user_agent, user_agent, sizeof(task->user_agent) - 1);
        task->should_stop = should_stop;
        task->total_downloaded = total_downloaded;
        task->progress_mutex = progress_mutex;
        task->total_size = total_size;
        task->quiet = quiet;
        task->progress_cb = progress_cb;

        pthread_create(&pool->threads[i], NULL, download_thread_func, task);
        pool->running++;
    }

    for (int i = 0; i < pool->running; i++) {
        pthread_join(pool->threads[i], NULL);
        struct download_task *task = &tasks[i];
        if (task->downloaded > 0) {
            modx_progress_save_thread(filename, i,
                                      task->start_byte, task->end_byte,
                                      task->downloaded);
        }
    }

    int success = 1;
    for (int i = 0; i < thread_count; i++) {
        if (tasks[i].downloaded < (tasks[i].end_byte - tasks[i].start_byte + 1)) {
            success = 0;
            break;
        }
    }

    free(tasks);
    pool->running = 0;
    pool->active = 0;
    return success ? 0 : -1;
}