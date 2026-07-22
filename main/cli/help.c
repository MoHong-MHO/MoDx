// main/cli/help.c

#include "cli.h"
#include <stdio.h>

static int g_lang_chinese = 0;

void modx_set_lang(int chinese)
{
    g_lang_chinese = chinese;
}

void modx_print_help(const struct modx_cli_opts *opts)
{
    (void)opts;
    if (g_lang_chinese) {
        printf("用法: modx [选项] <URL>\n");
        printf("      modx [选项] -i <批量文件>\n\n");
        printf("选项:\n");
        printf("  -t <线程数>   下载线程数 (默认 2, 最大 16)\n");
        printf("  -o <文件名>   输出文件名\n");
        printf("  -d <目录>     输出目录 (默认当前目录)\n");
        printf("  -u <UA>       User-Agent (默认 MoDx/1.7)\n");
        printf("  -R <次数>     失败重试次数 (默认 3)\n");
        printf("  -r <速率>     限速, 如 1M (单位: B/K/M/G)\n");
        printf("  -q            静默模式, 不输出进度\n");
        printf("  -i <文件>     从文件读取 URL 列表批量下载\n");
        printf("  -H            显示 HTTP 响应头\n");
        printf("  -V            详细模式, 输出调试信息\n");
        printf("  -y            自动续传 (不询问)\n");
        printf("  -x <代理>     使用代理, 如 http://proxy:8080\n");
        printf("  -m <URL...>   镜像下载, 提供多个备用 URL\n");
        printf("  --dns <IP>    自定义 DNS 服务器\n");
        printf("  --ca-cert <文件> 指定 CA 证书文件\n");
        printf("  -h, --help    显示此帮助\n");
        printf("  -v, --version 显示版本号\n");
    } else {
        printf("Usage: modx [options] <URL>\n");
        printf("       modx [options] -i <batch-file>\n\n");
        printf("Options:\n");
        printf("  -t <threads>   Thread count (default 2, max 16)\n");
        printf("  -o <filename>  Output filename\n");
        printf("  -d <directory> Output directory (default current)\n");
        printf("  -u <UA>        User-Agent (default MoDx/1.7)\n");
        printf("  -R <retries>   Max retries (default 3)\n");
        printf("  -r <rate>      Rate limit, e.g. 1M (units: B/K/M/G)\n");
        printf("  -q             Quiet mode, no progress output\n");
        printf("  -i <file>      Batch download from URL list file\n");
        printf("  -H             Show HTTP response headers\n");
        printf("  -V             Verbose mode, show debug info\n");
        printf("  -y             Auto-resume (don't ask)\n");
        printf("  -x <proxy>     Use proxy, e.g. http://proxy:8080\n");
        printf("  -m <URL...>    Mirror download with fallback URLs\n");
        printf("  --dns <IP>     Custom DNS server\n");
        printf("  --ca-cert <file> CA certificate file for TLS\n");
        printf("  -h, --help     Show this help\n");
        printf("  -v, --version  Show version\n");
    }
}

void modx_print_version(void)
{
    printf("MoDx version 1.7.0\n");
    printf("Supported: Linux x86_64, Linux ARM64\n");
    printf("HTTPS: mbedTLS (static linked)\n");
}