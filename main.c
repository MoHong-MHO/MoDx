// main.c - MoDx multi-thread downloader
// Cross-platform: Linux x86_64, ARM64, Android ARM64/ARMv7, Alpine x86_64

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <curl/curl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#define THREAD_COUNT 2
#define BUFFER_SIZE 8192

#if defined(__ANDROID__)
    #define PLATFORM_ANDROID 1
#elif defined(__linux__)
    #define PLATFORM_LINUX 1
#else
    #define PLATFORM_UNKNOWN 1
#endif

// Thread data structure
typedef struct {
    int id;
    char url[512];
    char filename[256];
    long long start_byte;
    long long end_byte;
    long long downloaded;
    int complete;
    pthread_t thread;
} ThreadData;

static ThreadData g_threads[THREAD_COUNT];
static long long g_total_size = 0;
static volatile long long g_total_downloaded = 0;
static volatile int g_should_stop = 0;
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

// Print message in current language
static void msg(const char *en_msg, const char *zh_msg) {
    if (g_lang_is_chinese) {
        printf("%s", zh_msg);
    } else {
        printf("%s", en_msg);
    }
}

// Write callback for curl
static size_t write_callback(void *ptr, size_t size, size_t nmemb, void *userdata) {
    ThreadData *td = (ThreadData*)userdata;
    size_t total = size * nmemb;
    size_t written = 0;
    FILE *fp = NULL;
    
    fp = fopen(td->filename, "rb+");
    if (!fp) {
        fp = fopen(td->filename, "wb+");
        if (!fp) {
            return 0;
        }
    }
    
    if (fseek(fp, (long)(td->start_byte + td->downloaded), SEEK_SET) != 0) {
        fclose(fp);
        return 0;
    }
    
    written = fwrite(ptr, 1, total, fp);
    fclose(fp);
    
    if (written == total) {
        td->downloaded += written;
        g_total_downloaded += written;
    }
    
    return written;
}

// Thread function
static void* download_thread(void *arg) {
    ThreadData *td = (ThreadData*)arg;
    CURL *curl = NULL;
    char range[64];
    CURLcode res;
    
    curl = curl_easy_init();
    if (!curl) {
        td->complete = 1;
        return NULL;
    }
    
    snprintf(range, sizeof(range), "%lld-%lld", td->start_byte, td->end_byte);
    
    curl_easy_setopt(curl, CURLOPT_URL, td->url);
    curl_easy_setopt(curl, CURLOPT_RANGE, range);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, td);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    
    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    
    td->complete = 1;
    return NULL;
}

// Get file size from URL
static long long get_file_size(const char *url) {
    CURL *curl = NULL;
    CURLcode res;
    curl_off_t size = 0;
    
    curl = curl_easy_init();
    if (!curl) return -1;
    
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    
    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        curl_easy_cleanup(curl);
        return -1;
    }
    
    curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &size);
    curl_easy_cleanup(curl);
    return (long long)size;
}

// Simple progress display thread
static void* progress_thread(void *arg) {
    (void)arg;
    int last_percent = -1;
    int percent = 0;
    
    while (!g_should_stop) {
        sleep(1);
        if (g_should_stop) break;
        
        if (g_total_size > 0) {
            percent = (int)((double)g_total_downloaded / g_total_size * 100);
            if (percent > 100) percent = 100;
            
            if (percent != last_percent) {
                printf("\r[%03d%%]", percent);
                fflush(stdout);
                last_percent = percent;
                
                if (percent >= 100) {
                    printf("\n");
                    break;
                }
            }
        }
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    char filename[256] = "downloaded_file";
    char *last_slash = NULL;
    long long seg = 0;
    pthread_t prog_thread;
    int success = 1;
    int i = 0;
    
    detect_language();
    
    if (argc < 2) {
        msg("Usage: %s <URL>\n", "用法: %s <URL>\n");
        msg("Example: %s https://example.com/file.zip\n", "示例: %s https://example.com/file.zip\n");
        return 1;
    }
    
    // Extract filename from URL
    last_slash = strrchr(argv[1], '/');
    if (last_slash && *(last_slash + 1) != '\0') {
        strncpy(filename, last_slash + 1, sizeof(filename) - 1);
        filename[sizeof(filename) - 1] = '\0';
    }
    
    msg("Starting download...\n", "开始下载...\n");
    msg("URL: %s\n", "URL: %s\n");
    msg("File: %s\n", "文件: %s\n");
    
    // Get file size
    g_total_size = get_file_size(argv[1]);
    if (g_total_size <= 0) {
        msg("Error: Cannot get file size\n", "错误: 无法获取文件大小\n");
        return 1;
    }
    
    // Calculate thread segments
    seg = g_total_size / THREAD_COUNT;
    for (i = 0; i < THREAD_COUNT; i++) {
        g_threads[i].id = i;
        strncpy(g_threads[i].url, argv[1], sizeof(g_threads[i].url) - 1);
        g_threads[i].url[sizeof(g_threads[i].url) - 1] = '\0';
        strncpy(g_threads[i].filename, filename, sizeof(g_threads[i].filename) - 1);
        g_threads[i].filename[sizeof(g_threads[i].filename) - 1] = '\0';
        g_threads[i].start_byte = (long long)i * seg;
        g_threads[i].end_byte = (i == THREAD_COUNT - 1) ? 
            g_total_size - 1 : (long long)(i + 1) * seg - 1;
        g_threads[i].downloaded = 0;
        g_threads[i].complete = 0;
    }
    
    // Create progress thread
    pthread_create(&prog_thread, NULL, progress_thread, NULL);
    
    // Create download threads
    for (i = 0; i < THREAD_COUNT; i++) {
        pthread_create(&g_threads[i].thread, NULL, download_thread, &g_threads[i]);
    }
    
    // Wait for all download threads to finish
    for (i = 0; i < THREAD_COUNT; i++) {
        pthread_join(g_threads[i].thread, NULL);
    }
    
    g_should_stop = 1;
    pthread_join(prog_thread, NULL);
    
    // Check if all threads completed successfully
    for (i = 0; i < THREAD_COUNT; i++) {
        if (!g_threads[i].complete) {
            success = 0;
            break;
        }
    }
    
    if (success && g_total_downloaded >= g_total_size) {
        msg("Download completed: %s\n", "下载完成: %s\n");
        return 0;
    } else {
        msg("Download failed or incomplete\n", "下载失败或不完整\n");
        msg("Partial data saved as: %s\n", "部分数据已保存为: %s\n");
        return 1;
    }
}