#include "checksum.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 使用系统工具，避免引入额外依赖 */
int modx_file_md5(const char *filename, char *out, size_t out_size)
{
    char cmd[1024];
    FILE *fp;

    snprintf(cmd, sizeof(cmd), "md5sum %s 2>/dev/null", filename);
    fp = popen(cmd, "r");
    if (!fp) return -1;

    if (fscanf(fp, "%s", out) != 1) {
        pclose(fp);
        return -1;
    }

    pclose(fp);
    return 0;
}

int modx_file_sha256(const char *filename, char *out, size_t out_size)
{
    char cmd[1024];
    FILE *fp;

    snprintf(cmd, sizeof(cmd), "sha256sum %s 2>/dev/null", filename);
    fp = popen(cmd, "r");
    if (!fp) return -1;

    if (fscanf(fp, "%s", out) != 1) {
        pclose(fp);
        return -1;
    }

    pclose(fp);
    return 0;
}