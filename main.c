// main.c - MoDx 主入口

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

#include "lib/core/core.h"
#include "lib/download/download.h"
#include "lib/net/net.h"
#include "lib/checksum/checksum.h"

#include "main/cli/cli.h"
#include "main/ui/ui.h"
#include "main/batch/batch.h"

#define VERSION "1.7.0"

static volatile int g_should_stop = 0;
static volatile long long g_total_downloaded = 0;
static pthread_mutex_t g_progress_mutex = PTHREAD_MUTEX_INITIALIZER;
static long long g_total_size = 0;
static int g_quiet = 0;
static int g_verbose = 0;
static double g_last_speed = 0;
static time_t g_last_time = 0;
static long long g_last_bytes = 0;

static void signal_handler(int sig)
{
    (void)sig;
    g_should_stop = 1;
    modx_output_msg("\n[Interrupted] Stopping...\n");
}

static void progress_wrapper(long long downloaded, long long total,
                              double speed, int eta)
{
    if (g_quiet) return;

    /* 计算速度 */
    time_t now = time(NULL);
    if (g_last_time == 0) {
        g_last_time = now;
        g_last_bytes = downloaded;
        g_last_speed = 0;
    } else {
        double elapsed = difftime(now, g_last_time);
        if (elapsed >= 1.0) {
            g_last_speed = (downloaded - g_last_bytes) / elapsed;
            g_last_time = now;
            g_last_bytes = downloaded;
        }
    }

    modx_progress_update(downloaded, total, g_last_speed, eta);
}

/* 下载单个 URL */
static int download_single(const struct modx_cli_opts *opts,
                           const char *url, const char *filename)
{
    int ret = 0;
    char fullpath[512];
    struct modx_thread_pool *pool = NULL;
    char ip[64];
    char checksum_out[128];
    int status_code = 0;
    const char *headers;
    int use_mirror = 0;
    const char *download_url = url;

    /* 构建完整路径 */
    if (opts->output_dir && strlen(opts->output_dir) > 0) {
        snprintf(fullpath, sizeof(fullpath), "%s/%s",
                 opts->output_dir, filename);
        char cmd[1024];
        snprintf(cmd, sizeof(cmd), "mkdir -p '%s'", opts->output_dir);
        (void)system(cmd);
    } else {
        strcpy(fullpath, filename);
    }

    modx_output_msg_init(opts->quiet, opts->verbose);
    modx_progress_init(opts->quiet);

    /* 显示代理信息 */
    if (opts->proxy_url) {
        modx_output_msg_verbose("Using proxy: %s\n", opts->proxy_url);
    }

    /* 显示镜像信息 */
    if (opts->mirrors && opts->mirror_count > 0) {
        modx_output_msg_verbose("Mirrors: %d available\n", opts->mirror_count);
    }

    /* 获取并显示服务器 IP */
    if (modx_tcp_connect(url, 80, ip, sizeof(ip)) >= 0 ||
        modx_tcp_connect(url, 443, ip, sizeof(ip)) >= 0) {
        if (!opts->quiet) {
            modx_output_msg("[Server IP] %s\n", ip);
        }
    }

    /* 获取文件大小 */
    g_total_size = modx_http_get_size(download_url);
    if (g_total_size <= 0) {
        modx_output_msg_error("Error: Cannot get file size\n");
        return 1;
    }

    if (!opts->quiet) {
        modx_output_msg("URL: %s\n", download_url);
        modx_output_msg("File: %s\n", fullpath);
        modx_output_msg("Size: %lld bytes (%.2f MB)\n",
                    g_total_size, (double)g_total_size / (1024 * 1024));
        modx_output_msg("Threads: %d\n", opts->threads);
        if (opts->rate_limit > 0) {
            modx_output_msg("Rate limit: %lld bytes/s\n", opts->rate_limit);
        }
        if (opts->max_retries != 3) {
            modx_output_msg("Max retries: %d\n", opts->max_retries);
        }
    }

    /* 检查续传 */
    if (modx_progress_exists(fullpath)) {
        if (opts->ask_resume) {
            char answer[8];
            modx_output_msg("Resume previous download? [Y/n]: ");
            fflush(stdout);
            if (fgets(answer, sizeof(answer), stdin)) {
                if (answer[0] == 'n' || answer[0] == 'N') {
                    modx_progress_remove(fullpath);
                    modx_progress_remove_all(fullpath, opts->threads);
                    modx_output_msg("Starting fresh download...\n");
                } else {
                    long long done = modx_progress_load(fullpath);
                    if (done > 0) {
                        modx_output_msg("Resuming from %lld bytes (%.1f%%)\n",
                                    done, (double)done / g_total_size * 100);
                        g_total_downloaded = done;
                    }
                }
            }
        } else {
            long long done = modx_progress_load(fullpath);
            if (done > 0) {
                if (!opts->quiet) {
                    modx_output_msg("Auto-resume from %lld bytes\n", done);
                }
                g_total_downloaded = done;
            }
        }
    }

    /* 预分配文件 */
    FILE *fp = fopen(fullpath, "rb+");
    if (!fp) {
        fp = fopen(fullpath, "wb");
        if (!fp) {
            modx_output_msg_error("Error: Cannot create file %s\n", fullpath);
            return 1;
        }
        fseek(fp, g_total_size - 1, SEEK_SET);
        fputc(0, fp);
    }
    fclose(fp);

    /* 设置限速 */
    if (opts->rate_limit > 0) {
        modx_rate_limit_init(opts->rate_limit);
    }

    /* 显示响应头（-H） */
    if (opts->show_headers) {
        headers = modx_http_get_last_headers();
        if (headers && strlen(headers) > 0) {
            modx_output_msg_headers(headers);
        }
    }

    /* 创建线程池并下载 */
    pool = modx_pool_create(opts->threads);
    if (!pool) {
        modx_output_msg_error("Error: Failed to create thread pool\n");
        return 1;
    }

    g_quiet = opts->quiet;
    g_verbose = opts->verbose;
    g_last_time = 0;
    g_last_bytes = 0;
    g_last_speed = 0;

    ret = modx_pool_download(pool,
                             download_url, fullpath,
                             g_total_size,
                             opts->threads,
                             opts->max_retries,
                             opts->rate_limit,
                             opts->user_agent ? opts->user_agent : "MoDx/1.7",
                             &g_should_stop,
                             &g_total_downloaded,
                             &g_progress_mutex,
                             progress_wrapper,
                             opts->quiet);

    modx_pool_destroy(pool);

    /* 检查是否完成 */
    if (ret == 0 && g_total_downloaded >= g_total_size) {
        /* 合并临时文件 */
        if (modx_merge_files(fullpath, opts->threads) == 0) {
            modx_progress_remove(fullpath);
            modx_progress_remove_all(fullpath, opts->threads);

            modx_output_msg("\n");

            /* 校验和（默认开启） */
            if (modx_file_md5(fullpath, checksum_out, sizeof(checksum_out)) == 0) {
                modx_output_msg("[MD5] %s\n", checksum_out);
            }
            if (modx_file_sha256(fullpath, checksum_out, sizeof(checksum_out)) == 0) {
                modx_output_msg("[SHA256] %s\n", checksum_out);
            }

            modx_output_msg("Download completed: %s\n", fullpath);
            return 0;
        } else {
            modx_output_msg_error("Error: Merge failed\n");
            return 1;
        }
    } else {
        if (g_should_stop) {
            modx_output_msg("\nDownload interrupted. Progress saved.\n");
        } else {
            modx_output_msg_error("\nDownload failed\n");
        }
        modx_progress_save(fullpath, g_total_downloaded);
        return 1;
    }
}

/* 导出给 batch 使用 */
int modx_download_single(const struct modx_cli_opts *opts,
                         const char *url, const char *filename)
{
    return download_single(opts, url, filename);
}

/* 检查 URL 是否有效 */
static int is_http_url(const char *str)
{
    return strncmp(str, "http://", 7) == 0 || strncmp(str, "https://", 8) == 0;
}

int main(int argc, char *argv[])
{
    struct modx_cli_opts opts;
    int ret;
    char default_ua[64] = "MoDx/1.7";

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    /* 设置默认值 */
    opts.user_agent = default_ua;
    opts.ask_resume = 1;  /* 默认询问 */

    ret = modx_parse_args(argc, argv, &opts);
    if (ret == 1) {
        modx_print_help(&opts);
        return 0;
    }
    if (ret == 2) {
        modx_print_version();
        return 0;
    }

    modx_output_msg_init(opts.quiet, opts.verbose);

    /* 批量下载模式 */
    if (opts.batch_file) {
        char **urls = NULL;
        int count = 0;
        if (modx_batch_read(opts.batch_file, &urls, &count) == 0) {
            int success = 0, failed = 0;
            for (int i = 0; i < count; i++) {
                if (!opts.quiet) {
                    modx_output_msg("\n[Batch %d/%d] %s\n", i + 1, count, urls[i]);
                }
                char *name = strrchr(urls[i], '/');
                if (modx_download_single(&opts, urls[i],
                                         name ? name + 1 : "downloaded_file") == 0) {
                    success++;
                } else {
                    failed++;
                }
            }
            modx_batch_free(urls, count);
            modx_output_msg("\nBatch done: %d success, %d failed\n", success, failed);
            return failed > 0 ? 1 : 0;
        }
        modx_output_msg_error("Error: Cannot read batch file %s\n", opts.batch_file);
        return 1;
    }

    /* 单 URL 模式 */
    if (argc < 2) {
        modx_print_help(&opts);
        return 1;
    }

    /* 获取 URL（最后一个非选项参数） */
    const char *url = NULL;
    for (int i = argc - 1; i >= 1; i--) {
        if (argv[i][0] != '-' && is_http_url(argv[i])) {
            url = argv[i];
            break;
        }
    }

    if (!url) {
        modx_output_msg_error("Error: No URL provided\n");
        return 1;
    }

    /* 确定输出文件名 */
    char filename[256];
    const char *name = strrchr(url, '/');
    if (opts.output_file) {
        strcpy(filename, opts.output_file);
    } else if (name && *(name + 1) != '\0') {
        strcpy(filename, name + 1);
    } else {
        strcpy(filename, "downloaded_file");
    }

    return modx_download_single(&opts, url, filename);
}