#include "download.h"
#include <time.h>

static long long g_rate_limit = 0;
static long long g_bucket = 0;
static struct timespec g_last_time = {0, 0};

void modx_rate_limit_init(long long bytes_per_sec)
{
    g_rate_limit = bytes_per_sec;
    g_bucket = 0;
    clock_gettime(CLOCK_MONOTONIC, &g_last_time);
}

void modx_rate_limit_wait(long long bytes_written)
{
    if (g_rate_limit <= 0) return;

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    long long elapsed_ns = (now.tv_sec - g_last_time.tv_sec) * 1000000000LL +
                           (now.tv_nsec - g_last_time.tv_nsec);

    if (elapsed_ns > 0) {
        long long tokens = (elapsed_ns * g_rate_limit) / 1000000000LL;
        g_bucket += tokens;
        if (g_bucket > g_rate_limit * 2) g_bucket = g_rate_limit * 2;
        g_last_time = now;
    }

    g_bucket -= bytes_written;

    if (g_bucket < 0) {
        long long deficit_ns = (-g_bucket * 1000000000LL) / g_rate_limit;
        struct timespec req = {deficit_ns / 1000000000LL, deficit_ns % 1000000000LL};
        nanosleep(&req, NULL);
        g_bucket = 0;
        clock_gettime(CLOCK_MONOTONIC, &g_last_time);
    }
}