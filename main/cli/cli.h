#ifndef MODX_CLI_H
#define MODX_CLI_H

struct modx_cli_opts {
    int threads;
    char *output_file;
    char *output_dir;
    char *user_agent;
    int max_retries;
    long long rate_limit;
    int quiet;
    char *batch_file;
    int show_headers;
    int verbose;
    int ask_resume;
    char *proxy_url;
    char **mirrors;
    int mirror_count;
    char *dns_server;
    char *ca_cert_file;
};

int modx_parse_args(int argc, char **argv, struct modx_cli_opts *opts);
void modx_print_help(const struct modx_cli_opts *opts);
void modx_print_version(void);

/* 从 main.c 导出 */
int modx_download_single(const struct modx_cli_opts *opts,
                         const char *url, const char *filename);

#endif