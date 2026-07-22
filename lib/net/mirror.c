// lib/net/mirror.c

#include "net.h"
#include <stdlib.h>
#include <string.h>

#define MAX_MIRRORS 16

static char *g_mirrors[MAX_MIRRORS];
static int g_mirror_count = 0;
static int g_mirror_index = 0;

int modx_mirror_add(const char *url)
{
    if (g_mirror_count >= MAX_MIRRORS) return -1;
    g_mirrors[g_mirror_count] = strdup(url);
    if (!g_mirrors[g_mirror_count]) return -1;
    g_mirror_count++;
    return 0;
}

void modx_mirror_clear(void)
{
    for (int i = 0; i < g_mirror_count; i++) {
        free(g_mirrors[i]);
        g_mirrors[i] = NULL;
    }
    g_mirror_count = 0;
    g_mirror_index = 0;
}

const char* modx_mirror_next(void)
{
    if (g_mirror_count == 0) return NULL;
    const char *url = g_mirrors[g_mirror_index];
    g_mirror_index = (g_mirror_index + 1) % g_mirror_count;
    return url;
}

int modx_mirror_count(void)
{
    return g_mirror_count;
}