// main/ui/output.c

#include "ui.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

static int g_verbose = 0;
static int g_quiet = 0;

void modx_output_init(int quiet, int verbose)
{
    g_quiet = quiet;
    g_verbose = verbose;
}

void modx_output(const char *msg)
{
    if (g_quiet) return;
    printf("%s", msg);
}

void modx_output_verbose(const char *msg, ...)
{
    if (g_quiet || !g_verbose) return;
    va_list args;
    va_start(args, msg);
    printf("[VERBOSE] ");
    vprintf(msg, args);
    va_end(args);
}

void modx_output_error(const char *msg, ...)
{
    va_list args;
    va_start(args, msg);
    vfprintf(stderr, msg, args);
    va_end(args);
}

void modx_output_headers(const char *headers)
{
    if (!headers) return;
    printf("\n[HEADERS]\n");
    printf("---\n%s---\n", headers);
}