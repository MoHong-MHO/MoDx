// main/cli/args.c

#include "cli.h"
#include "../ui/ui.h"
#include "../../lib/net/net.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define VERSION "1.7.0"

static long long parse_rate(const char *str)
{
    long long val = 0;
    char unit = 0;
    if (sscanf(str, "%lld%c", &val, &unit) < 1) return 0;
    switch (toupper(unit)) {
        case 'K': val *= 1024; break;
        case 'M': val *= 1024 * 1024; break;
        case 'G': val *= 1024 * 1024 * 1024; break;
        default: break;
    }
    return val;
}

static int parse_mirrors(int *i, int argc, char **argv, struct modx_cli_opts *opts)
{
    int count = 0;
    opts->mirrors = &argv[(*i) + 1];
    while ((*i) + 1 < argc && argv[(*i) + 1][0] != '-') {
        (*i)++;
        count++;
        if (count >= 16) break;
    }
    opts->mirror_count = count;
    return 0;
}

int modx_parse_args(int argc, char **argv, struct modx_cli_opts *opts)
{
    memset(opts, 0, sizeof(struct modx_cli_opts));
    opts->threads = 2;
    opts->max_retries = 3;
    opts->user_agent = "MoDx/1.7";
    opts->ask_resume = 1;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            return 1;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            return 2;
        } else if (strcmp(argv[i], "-q") == 0) {
            opts->quiet = 1;
        } else if (strcmp(argv[i], "-H") == 0) {
            opts->show_headers = 1;
        } else if (strcmp(argv[i], "-V") == 0) {
            opts->verbose = 1;
        } else if (strcmp(argv[i], "-y") == 0) {
            opts->ask_resume = 0;
        } else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
            opts->threads = atoi(argv[++i]);
            if (opts->threads < 1) opts->threads = 1;
            if (opts->threads > 16) opts->threads = 16;
        } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            opts->output_file = argv[++i];
        } else if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
            opts->output_dir = argv[++i];
        } else if (strcmp(argv[i], "-u") == 0 && i + 1 < argc) {
            opts->user_agent = argv[++i];
        } else if (strcmp(argv[i], "-R") == 0 && i + 1 < argc) {
            opts->max_retries = atoi(argv[++i]);
            if (opts->max_retries < 0) opts->max_retries = 0;
        } else if (strcmp(argv[i], "-r") == 0 && i + 1 < argc) {
            opts->rate_limit = parse_rate(argv[++i]);
        } else if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
            opts->batch_file = argv[++i];
        } else if (strcmp(argv[i], "-x") == 0 && i + 1 < argc) {
            opts->proxy_url = argv[++i];
            modx_proxy_set_http(opts->proxy_url);  /* 现在有声明了 */
        } else if (strcmp(argv[i], "-m") == 0) {
            parse_mirrors(&i, argc, argv, opts);
        } else if (strcmp(argv[i], "--dns") == 0 && i + 1 < argc) {
            opts->dns_server = argv[++i];
            modx_output_verbose("Custom DNS: %s\n", opts->dns_server);
        } else if (strcmp(argv[i], "--ca-cert") == 0 && i + 1 < argc) {
            opts->ca_cert_file = argv[++i];
            modx_output_verbose("CA cert: %s\n", opts->ca_cert_file);
        } else if (argv[i][0] != '-') {
            /* 可能是 URL，由 main.c 处理 */
        }
    }

    return 0;
}