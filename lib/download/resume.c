#include "download.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PROGRESS_SUFFIX ".progress"

static void make_path(const char *filename, char *out, size_t size, const char *suffix)
{
    snprintf(out, size, "%s.%s", filename, suffix);
}

int modx_progress_save(const char *filename, long long total)
{
    char path[512];
    FILE *fp;

    make_path(filename, path, sizeof(path), "progress");
    fp = fopen(path, "w");
    if (!fp) return -1;
    fprintf(fp, "%lld", total);
    fclose(fp);
    return 0;
}

long long modx_progress_load(const char *filename)
{
    char path[512];
    FILE *fp;
    long long total = -1;

    make_path(filename, path, sizeof(path), "progress");
    fp = fopen(path, "r");
    if (!fp) return -1;
    if (fscanf(fp, "%lld", &total) != 1) total = -1;
    fclose(fp);
    return total;
}

void modx_progress_remove(const char *filename)
{
    char path[512];
    make_path(filename, path, sizeof(path), "progress");
    remove(path);
}

int modx_progress_exists(const char *filename)
{
    return modx_progress_load(filename) >= 0;
}

int modx_progress_save_thread(const char *filename, int tid,
                              long long start, long long end, long long downloaded)
{
    char path[512];
    FILE *fp;

    snprintf(path, sizeof(path), "%s.progress.%d", filename, tid);
    fp = fopen(path, "w");
    if (!fp) return -1;
    fprintf(fp, "%lld %lld %lld", start, end, downloaded);
    fclose(fp);
    return 0;
}

long long modx_progress_load_thread(const char *filename, int tid,
                                    long long *start, long long *end)
{
    char path[512];
    FILE *fp;
    long long downloaded = -1;

    snprintf(path, sizeof(path), "%s.progress.%d", filename, tid);
    fp = fopen(path, "r");
    if (!fp) return -1;
    if (fscanf(fp, "%lld %lld %lld", start, end, &downloaded) != 3) {
        downloaded = -1;
    }
    fclose(fp);
    return downloaded;
}

void modx_progress_remove_thread(const char *filename, int tid)
{
    char path[512];
    snprintf(path, sizeof(path), "%s.progress.%d", filename, tid);
    remove(path);
}

void modx_progress_remove_all(const char *filename, int thread_count)
{
    for (int i = 0; i < thread_count; i++) {
        modx_progress_remove_thread(filename, i);
    }
}