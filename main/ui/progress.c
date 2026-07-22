#include "ui.h"
#include <stdio.h>
#include <string.h>

static int g_quiet = 0;

void modx_progress_init(int quiet)
{
    g_quiet = quiet;
}

void modx_progress_bar(long long downloaded, long long total)
{
    if (g_quiet || total <= 0) return;

    int percent = (int)((double)downloaded / total * 100);
    if (percent > 100) percent = 100;

    int bar_width = 40;
    int filled = (bar_width * percent) / 100;

    printf("\r[");
    for (int i = 0; i < bar_width; i++) {
        if (i < filled) printf("#");
        else if (i == filled) printf(">");
        else printf("-");
    }
    printf("] %3d%%", percent);
    fflush(stdout);
}

void modx_progress_update(long long downloaded, long long total,
                          double speed, int eta)
{
    modx_progress_bar(downloaded, total);
}

void modx_progress_finish(void)
{
    if (!g_quiet) printf("\n");
}