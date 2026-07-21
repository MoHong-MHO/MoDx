// main.c - MoDx CLI tool

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <ctype.h>
#include "modx_lib.h"

#define VERSION "1.6.0"

static int g_lang_is_chinese = 0;
static int g_thread_count = MODX_THREAD_COUNT;
static char g_output_filename[256] = {0};
static char g_output_dir[512] = {0};
static char g_user_agent[256] = "MoDx/1.6";
static int g_quiet = 0;
static int g_max_retries = 3;
static long long g_rate_limit = 0;          // 0 = no limit
static char g_batch_file[512] = {0};        // -i 指定文件

static void detect_language(void) {
    char *lang = getenv("LANG");
    if (lang) {
        if (strstr(lang, "zh_CN") || strstr(lang, "zh_TW") ||
            strstr(lang, "zh_HK") || strstr(lang, "zh_SG")) {
            g_lang_is_chinese = 1;
        }
    }
}

static void msg(const char *en_msg, const char *zh_msg, ...) {
    if (g_quiet) return;
    va_list args;
    va_start(args, zh_msg);
    if (g_lang_is_chinese) {
        vprintf(zh_msg, args);
    } else {
        vprintf(en_msg, args);
    }
    va_end(args);
}

static void msg_error(const char *en_msg, const char *zh_msg, ...) {
    va_list args;
    va_start(args, zh_msg);
    if (g_lang_is_chinese) {
        vfprintf(stderr, zh_msg, args);
    } else {
        vfprintf(stderr, en_msg, args);
    }
    va_end(args);
}

static void format_size(long long bytes, char *buf) {
    const char *units[] = {"B", "KB", "MB", "GB", "TB"};
    int idx = 0;
    double val = (double)bytes;
    while (val >= 1024.0 && idx < 4) {
        val /= 1024.0;
        idx++;
    }
    sprintf(buf, "%.2f %s", val, units[idx]);
}

static void format_time(int seconds, char *buf) {
    if (seconds < 60) {
        sprintf(buf, "%ds", seconds);
    } else if (seconds < 3600) {
        sprintf(buf, "%dm%ds", seconds / 60, seconds % 60);
    } else {
        sprintf(buf, "%dh%dm%ds", seconds / 3600, (seconds % 3600) / 60, seconds % 60);
    }
}

static long long parse_rate(const char *str) {
    long long val = 0;
    char unit = 0;
    sscanf(str, "%lld%c", &val, &unit);
    switch (toupper(unit)) {
        case 'K': val *= 1024; break;
        case 'M': val *= 1024 * 1024; break;
        case 'G': val *= 1024 * 1024 * 1024; break;
        default: break;
    }
    return val;
}

static void print_help(void) {
    if (g_lang_is_chinese) {
        printf("用法: modx [选项] <URL>\n");
        printf("      modx [选项] -i <批量文件>\n\n");
        printf("选项:\n");
        printf("  -t <线程数>    指定下载线程数 (默认 2, 最大 16)\n");
        printf("  -o <文件名>    指定输出文件名\n");
        printf("  -d <目录>      指定输出目录 (默认当前目录)\n");
        printf("  -u <UA>        指定 User-Agent (默认 MoDx/1.6)\n");
        printf("  -R <次数>      失败重试次数 (默认 3)\n");
        printf("  -r <速率>      限速下载，如 1M (单位: B/K/M/G)\n");
        printf("  -q             静默模式，不输出进度信息\n");
        printf("  -i <文件>      从文件读取 URL 列表批量下载\n");
        printf("  -h, --help     显示此帮助信息\n");
        printf("  -v, --version  显示版本号\n\n");
        printf("示例:\n");
        printf("  modx https://example.com/file.zip\n");
        printf("  modx -t 4 -o myfile.zip https://example.com/file.zip\n");
        printf("  modx -d ./downloads -r 1M https://example.com/file.zip\n");
        printf("  modx -i urls.txt -d ./downloads -t 2\n");
    } else {
        printf("Usage: modx [options] <URL>\n");
        printf("       modx [options] -i <batch-file>\n\n");
        printf("Options:\n");
        printf("  -t <threads>    Number of download threads (default 2, max 16)\n");
        printf("  -o <filename>   Output filename\n");
        printf("  -d <directory>  Output directory (default current)\n");
        printf("  -u <UA>         Set User-Agent (default MoDx/1.6)\n");
        printf("  -R <retries>    Max retries on failure (default 3)\n");
        printf("  -r <rate>       Rate limit, e.g. 1M (units: B/K/M/G)\n");
        printf("  -q              Quiet mode, no progress output\n");
        printf("  -i <file>       Batch download from URL list file\n");
        printf("  -h, --help      Show this help message\n");
        printf("  -v, --version   Show version information\n\n");
        printf("Examples:\n");
        printf("  modx https://example.com/file.zip\n");
        printf("  modx -t 4 -o myfile.zip https://example.com/file.zip\n");
        printf("  modx -d ./downloads -r 1M https://example.com/file.zip\n");
        printf("  modx -i urls.txt -d ./downloads -t 2\n");
    }
}

static void print_version(void) {
    printf("MoDx version %s\n", VERSION);
    printf("Supported architectures: Linux x86_64, Linux ARM64\n");
    printf("HTTPS support: mbedTLS (static linked into libmodx.so)\n");
}

static void progress_callback(long long downloaded, long long total, double speed, int eta, void *userdata) {
    if (g_quiet) return;
    if (total <= 0) return;

    int percent = (int)((double)downloaded / total * 100);
    if (percent > 100) percent = 100;

    char speed_str[32], eta_str[32], total_str[32], done_str[32];
    format_size((long long)speed, speed_str);
    format_time(eta, eta_str);
    format_size(total, total_str);
    format_size(downloaded, done_str);

    printf("\r[%03d%%] %s / %s  %s/s  ETA: %s  ",
           percent, done_str, total_str, speed_str, eta_str);
    fflush(stdout);
}

static int download_url(const char *url, const char *output_dir, const char *output_name) {
    char filename[512];
    char fullpath[512];
    char *last_slash = NULL;
    modx_handle downloader = NULL;
    int ret = 1;

    if (output_name && strlen(output_name) > 0) {
        strncpy(filename, output_name, sizeof(filename) - 1);
        filename[sizeof(filename) - 1] = '\0';
    } else {
        last_slash = strrchr(url, '/');
        if (last_slash && *(last_slash + 1) != '\0') {
            strncpy(filename, last_slash + 1, sizeof(filename) - 1);
            filename[sizeof(filename) - 1] = '\0';
        } else {
            strcpy(filename, "downloaded_file");
        }
    }

    if (output_dir && strlen(output_dir) > 0) {
        snprintf(fullpath, sizeof(fullpath), "%s/%s", output_dir, filename);
    } else {
        strcpy(fullpath, filename);
    }

    // Ensure directory exists
    if (output_dir && strlen(output_dir) > 0) {
        char cmd[1024];
        snprintf(cmd, sizeof(cmd), "mkdir -p '%s'", output_dir);
        system(cmd);
    }

    if (!g_quiet) {
        msg("Starting download...\n", "开始下载...\n");
        msg("URL: %s\n", "URL: %s\n", url);
        msg("File: %s\n", "文件: %s\n", fullpath);
        msg("Threads: %d\n", "线程数: %d\n", g_thread_count);
        msg("User-Agent: %s\n", "User-Agent: %s\n", g_user_agent);
        if (g_rate_limit > 0) {
            char rate_str[32];
            format_size(g_rate_limit, rate_str);
            msg("Rate limit: %s/s\n", "限速: %s/s\n", rate_str);
        }
        if (g_max_retries != 3) {
            msg("Max retries: %d\n", "最大重试: %d\n", g_max_retries);
        }
    }

    if (modx_can_resume(fullpath)) {
        msg("Resuming previous download...\n", "检测到未完成的下载，继续中...\n");
    }

    downloader = modx_create(g_thread_count);
    if (!downloader) {
        msg_error("Error: Failed to create downloader\n", "错误: 无法创建下载器\n");
        return 1;
    }

    modx_set_user_agent(downloader, g_user_agent);
    modx_set_max_retries(downloader, g_max_retries);
    modx_set_rate_limit(downloader, g_rate_limit);
    modx_set_progress_callback(downloader, progress_callback, NULL);

    if (modx_download(downloader, url, fullpath) == 0) {
        if (!g_quiet) printf("\n");
        msg("Download completed: %s\n", "下载完成: %s\n", fullpath);
        ret = 0;
    } else {
        if (!g_quiet) printf("\n");
        msg_error("Download failed: %s\n", "下载失败: %s\n", fullpath);
        ret = 1;
    }

    modx_destroy(downloader);
    return ret;
}

static int batch_download(const char *batch_file, const char *output_dir) {
    FILE *fp = fopen(batch_file, "r");
    if (!fp) {
        msg_error("Error: Cannot open batch file %s\n", "错误: 无法打开批量文件 %s\n", batch_file);
        return 1;
    }

    char line[1024];
    int line_num = 0;
    int success_count = 0;
    int fail_count = 0;

    while (fgets(line, sizeof(line), fp)) {
        line_num++;
        // Remove trailing newline
        char *nl = strchr(line, '\n');
        if (nl) *nl = '\0';
        // Skip empty lines and comments
        if (strlen(line) == 0 || line[0] == '#') continue;

        if (!g_quiet) {
            msg("\n[Batch %d] Processing: %s\n", "[批量 %d] 处理: %s\n", line_num, line);
        }

        if (download_url(line, output_dir, NULL) == 0) {
            success_count++;
        } else {
            fail_count++;
            if (!g_quiet) {
                msg_error("Batch: skipping to next URL\n", "批量: 跳过到下一个 URL\n");
            }
        }
    }

    fclose(fp);

    if (!g_quiet) {
        msg("Batch completed: %d succeeded, %d failed\n",
            "批量完成: %d 成功, %d 失败\n", success_count, fail_count);
    }
    return (fail_count > 0) ? 1 : 0;
}

int main(int argc, char *argv[]) {
    char url[512] = {0};
    char *last_slash = NULL;
    int has_url = 0;
    int has_batch = 0;
    int ret = 0;

    detect_language();

    if (argc < 2) {
        msg("Usage: %s [options] <URL>\n", "用法: %s [选项] <URL>\n", argv[0]);
        msg("Try '%s -h' for more information.\n", "尝试 '%s -h' 获取更多信息。\n", argv[0]);
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_help();
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            print_version();
            return 0;
        } else if (strcmp(argv[i], "-q") == 0) {
            g_quiet = 1;
        } else if (strcmp(argv[i], "-t") == 0) {
            if (i + 1 < argc) {
                g_thread_count = atoi(argv[++i]);
                if (g_thread_count < 1) g_thread_count = 1;
                if (g_thread_count > 16) g_thread_count = 16;
            } else {
                msg_error("Error: -t requires a number\n", "错误: -t 需要指定数字\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 < argc) {
                strncpy(g_output_filename, argv[++i], sizeof(g_output_filename) - 1);
                g_output_filename[sizeof(g_output_filename) - 1] = '\0';
            } else {
                msg_error("Error: -o requires a filename\n", "错误: -o 需要指定文件名\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-d") == 0) {
            if (i + 1 < argc) {
                strncpy(g_output_dir, argv[++i], sizeof(g_output_dir) - 1);
                g_output_dir[sizeof(g_output_dir) - 1] = '\0';
            } else {
                msg_error("Error: -d requires a directory\n", "错误: -d 需要指定目录\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-u") == 0) {
            if (i + 1 < argc) {
                strncpy(g_user_agent, argv[++i], sizeof(g_user_agent) - 1);
                g_user_agent[sizeof(g_user_agent) - 1] = '\0';
            } else {
                msg_error("Error: -u requires a string\n", "错误: -u 需要指定字符串\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-R") == 0) {
            if (i + 1 < argc) {
                g_max_retries = atoi(argv[++i]);
                if (g_max_retries < 0) g_max_retries = 0;
            } else {
                msg_error("Error: -R requires a number\n", "错误: -R 需要指定数字\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-r") == 0) {
            if (i + 1 < argc) {
                g_rate_limit = parse_rate(argv[++i]);
                if (g_rate_limit < 0) g_rate_limit = 0;
            } else {
                msg_error("Error: -r requires a rate\n", "错误: -r 需要指定速率\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-i") == 0) {
            if (i + 1 < argc) {
                strncpy(g_batch_file, argv[++i], sizeof(g_batch_file) - 1);
                g_batch_file[sizeof(g_batch_file) - 1] = '\0';
                has_batch = 1;
            } else {
                msg_error("Error: -i requires a file\n", "错误: -i 需要指定文件\n");
                return 1;
            }
        } else {
            // 最后一个非选项参数当作 URL
            strncpy(url, argv[i], sizeof(url) - 1);
            url[sizeof(url) - 1] = '\0';
            has_url = 1;
        }
    }

    // 批量模式
    if (has_batch) {
        if (strlen(g_batch_file) == 0) {
            msg_error("Error: Batch file not specified\n", "错误: 未指定批量文件\n");
            return 1;
        }
        return batch_download(g_batch_file, g_output_dir);
    }

    // 普通 URL 模式
    if (!has_url || strlen(url) == 0) {
        msg_error("Error: No URL provided\n", "错误: 未提供 URL\n");
        msg_error("Try '%s -h' for more information.\n", "尝试 '%s -h' 获取更多信息。\n", argv[0]);
        return 1;
    }

    return download_url(url, g_output_dir, g_output_filename);
}