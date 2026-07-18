// main.c - MoDx CLI tool
// Uses modx_lib for downloading

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "modx_lib.h"

static int g_lang_is_chinese = 0;

// Detect system language
static void detect_language(void) {
    char *lang = getenv("LANG");
    if (lang) {
        if (strstr(lang, "zh_CN") || strstr(lang, "zh_TW") || 
            strstr(lang, "zh_HK") || strstr(lang, "zh_SG")) {
            g_lang_is_chinese = 1;
        }
    }
}

// Print message in current language with format support
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

// Progress callback
static void progress_callback(long long downloaded, long long total, void *userdata) {
    if (total > 0) {
        int percent = (int)((double)downloaded / total * 100);
        if (percent > 100) percent = 100;
        printf("\r[%03d%%]", percent);
        fflush(stdout);
    }
}

int main(int argc, char *argv[]) {
    char filename[256] = "downloaded_file";
    char *last_slash = NULL;
    long long file_size = 0;
    modx_handle downloader = NULL;
    
    detect_language();
    
    if (argc < 2) {
        msg("Usage: %s <URL>\n", "用法: %s <URL>\n", argv[0]);
        msg("Example: %s http://example.com/file.zip\n", "示例: %s http://example.com/file.zip\n", argv[0]);
        return 1;
    }
    
    // Extract filename from URL
    last_slash = strrchr(argv[1], '/');
    if (last_slash && *(last_slash + 1) != '\0') {
        strncpy(filename, last_slash + 1, sizeof(filename) - 1);
        filename[sizeof(filename) - 1] = '\0';
    }
    
    msg("Starting download...\n", "开始下载...\n");
    msg("URL: %s\n", "URL: %s\n", argv[1]);
    msg("File: %s\n", "文件: %s\n", filename);
    
    // Get file size
    file_size = modx_get_file_size(argv[1]);
    if (file_size <= 0) {
        msg("Error: Cannot get file size\n", "错误: 无法获取文件大小\n");
        return 1;
    }
    
    msg("File size: %lld bytes\n", "文件大小: %lld 字节\n", file_size);
    
    // Create downloader
    downloader = modx_create(MODX_THREAD_COUNT);
    if (!downloader) {
        msg("Error: Failed to create downloader\n", "错误: 无法创建下载器\n");
        return 1;
    }
    
    // Set progress callback
    modx_set_progress_callback(downloader, progress_callback, NULL);
    
    // Download
    if (modx_download(downloader, argv[1], filename) == 0) {
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