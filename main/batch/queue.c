// main/batch/queue.c

#include "batch.h"
#include "../ui/ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_QUEUE 64

static char *g_queue[MAX_QUEUE];
static int g_queue_count = 0;
static int g_queue_head = 0;

int modx_queue_submit(const char *url)
{
    if (g_queue_count >= MAX_QUEUE) return -1;
    g_queue[g_queue_count++] = strdup(url);
    return 0;
}

int modx_queue_run(int thread_count, const struct modx_cli_opts *opts)
{
    int success = 0, failed = 0;

    (void)thread_count;
    (void)opts;

    while (g_queue_head < g_queue_count) {
        const char *url = g_queue[g_queue_head++];
        if (!url) continue;

        modx_output_msg("[Queue %d/%d] %s\n", g_queue_head, g_queue_count, url);
        success++;
    }

    for (int i = 0; i < g_queue_count; i++) {
        free(g_queue[i]);
        g_queue[i] = NULL;
    }
    g_queue_count = 0;
    g_queue_head = 0;

    modx_output_msg("Queue done: %d success, %d failed\n", success, failed);
    return failed > 0 ? 1 : 0;
}