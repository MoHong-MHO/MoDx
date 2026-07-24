// main/batch/reader.c

#include "batch.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int modx_batch_read(const char *filename, char ***urls, int *count)
{
    FILE *fp = fopen(filename, "r");
    if (!fp) return -1;

    char line[1024];
    int cap = 10;
    *count = 0;
    *urls = malloc(cap * sizeof(char *));
    if (!*urls) {
        fclose(fp);
        return -1;
    }

    while (fgets(line, sizeof(line), fp)) {
        char *nl = strchr(line, '\n');
        if (nl) *nl = '\0';
        if (strlen(line) == 0 || line[0] == '#') continue;

        if (*count >= cap) {
            cap *= 2;
            *urls = realloc(*urls, cap * sizeof(char *));
            if (!*urls) break;
        }
        (*urls)[(*count)++] = strdup(line);
    }

    fclose(fp);
    return 0;
}

void modx_batch_free(char **urls, int count)
{
    for (int i = 0; i < count; i++) {
        free(urls[i]);
    }
    free(urls);
}