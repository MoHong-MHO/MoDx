#include "download.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_SIZE 8192

int modx_merge_files(const char *filename, int thread_count)
{
    FILE *final = fopen(filename, "wb");
    if (!final) return -1;

    for (int i = 0; i < thread_count; i++) {
        char tmp_path[512];
        FILE *tmp;
        char buf[BUFFER_SIZE];
        size_t n;

        snprintf(tmp_path, sizeof(tmp_path), "%s.tmp.%d", filename, i);
        tmp = fopen(tmp_path, "rb");
        if (!tmp) {
            fclose(final);
            return -1;
        }

        while ((n = fread(buf, 1, sizeof(buf), tmp)) > 0) {
            if (fwrite(buf, 1, n, final) != n) {
                fclose(tmp);
                fclose(final);
                return -1;
            }
        }

        fclose(tmp);
        remove(tmp_path);
    }

    fclose(final);
    return 0;
}