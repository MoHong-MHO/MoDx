// main.c - MoDx CLI tool

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include "modx_lib.h"

#define VERSION "1.0.0"

static int g_lang_is_chinese = 0;
static int g_thread_count = MODX_THREAD_COUNT;  // 默认 2
static char g_output_filename[256] = {0};

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
    va_list args;
    va_start(args, zh_msg);
    if (g_lang_is_chinese) {
        vprintf(zh_msg, args);
    } else {
        vprintf(en_msg, args);
    }
    va_end(args);
}

static void print_help(void) {
    if (g_lang_is_chinese) {
        printf("用法: modx [选项] <URL>\n\n");
        printf("选项:\n");
        printf("  -t <线程数>    指定下载线程数 (默认 2, 最大 16)\n");
        printf("  -o <文件名>    指定输出文件名\n");
        printf("  -h, --help     显示此帮助信息\n");
        printf("  -v, --version  显示版本号\n\n");
        printf("示例:\n");
        printf("  modx http://example.com/file.zip\n");
        printf("  modx -t 4 http://example.com/file.zip\n");
        printf("  modx -o myfile.zip http://example.com/file.zip\n");
        printf("  modx -t 8 -o ubuntu.iso http://releases.ubuntu.com/.../ubuntu.iso\n");
    } else {
        printf("Usage: modx [options] <URL>\n\n");
        printf("Options:\n");
        printf("  -t <threads>    Number of download threads (default 2, max 16)\n");
        printf("  -o <filename>   Output filename\n");
        printf("  -h, --help      Show this help message\n");
        printf("  -v, --version   Show version information\n\n");
        printf("Examples:\n");
        printf("  modx http://example.com/file.zip\n");
        printf("  modx -t 4 http://example.com/file.zip\n");
        printf("  modx -o myfile.zip http://example.com/file.zip\n");
        printf("  modx -t 8 -o ubuntu.iso http://releases.ubuntu.com/.../ubuntu.iso\n");
    }
}

static void print_version(void) {
    printf("MoDx version %s\n", VERSION);
    printf("Supported architectures: Linux x86_64, Linux ARM64\n");
    printf("Built with: HTTP range request, multi-thread download\n");
}

static void progress_callback(long long downloaded, long long total, void *userdata) {
    if (total > 0) {
        int percent = (int)((double)downloaded / total * 100);
        if (percent > 100) percent = 100;
        printf("\r[%03d%%]", percent);
        fflush(stdout);
    }
}

int main(int argc, char *argv[]) {
    char url[512] = {0};
    char *last_slash = NULL;
    modx_handle downloader = NULL;
    int opt;

    detect_language();

    // Simple argument parsing (no getopt dependency)
    if (argc < 2) {
        msg("Usage: %s [options] <URL>\n", "用法: %s [选项] <URL>\n", argv[0]);
        msg("Try '%s -h' for more information.\n", "尝试 '%s -h' 获取更多信息。\n", argv[0]);
        return 1;
    }

    // Parse arguments manually
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_help();
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            print_version();
            return 0;
        } else if (strcmp(argv[i], "-t") == 0) {
            if (i + 1 < argc) {
                g_thread_count = atoi(argv[++i]);
                if (g_thread_count < 1) g_thread_count = 1;
                if (g_thread_count > 16) g_thread_count = 16;
            } else {
                msg("Error: -t requires a number\n", "错误: -t 需要指定数字\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 < argc) {
                strncpy(g_output_filename, argv[++i], sizeof(g_output_filename) - 1);
                g_output_filename[sizeof(g_output_filename) - 1] = '\0';
            } else {
                msg("Error: -o requires a filename\n", "错误: -o 需要指定文件名\n");
                return 1;
            }
        } else {
            // Assume it's the URL
            strncpy(url, argv[i], sizeof(url) - 1);
            url[sizeof(url) - 1] = '\0';
        }
    }

    // Check if URL is provided
    if (strlen(url) == 0) {
        msg("Error: No URL provided\n", "错误: 未提供 URL\n");
        msg("Try '%s -h' for more information.\n", "尝试 '%s -h' 获取更多信息。\n", argv[0]);
        return 1;
    }

    // Determine output filename
    char filename[256];
    if (strlen(g_output_filename) > 0) {
        strncpy(filename, g_output_filename, sizeof(filename) - 1);
        filename[sizeof(filename) - 1] = '\0';
    } else {
        // Extract from URL
        last_slash = strrchr(url, '/');
        if (last_slash && *(last_slash + 1) != '\0') {
            strncpy(filename, last_slash + 1, sizeof(filename) - 1);
            filename[sizeof(filename) - 1] = '\0';
        } else {
            strcpy(filename, "downloaded_file");
        }
    }

    msg("Starting download...\n", "开始下载...\n");
    msg("URL: %s\n", "URL: %s\n", url);
    msg("File: %s\n", "文件: %s\n", filename);
    msg("Threads: %d\n", "线程数: %d\n", g_thread_count);

    // Create downloader with user-specified thread count
    downloader = modx_create(g_thread_count);
    if (!downloader) {
        msg("Error: Failed to create downloader\n", "错误: 无法创建下载器\n");
        return 1;
    }

    modx_set_progress_callback(downloader, progress_callback, NULL);

    if (modx_download(downloader, url, filename) == 0) {
        printf("\n");
        msg("Download completed: %s\n", "下载完成: %s\n", filename);
        modx_destroy(downloader);
        return 0;
    } else {
        printf("\n");
        msg("Download failed\n", "下载失败\n");
        modx_destroy(downloader);
        return 1;
    }
}