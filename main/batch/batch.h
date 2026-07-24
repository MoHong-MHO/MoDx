// main/batch/batch.h

#ifndef MODX_BATCH_H
#define MODX_BATCH_H

#include "../cli/cli.h"

int modx_batch_read(const char *filename, char ***urls, int *count);
void modx_batch_free(char **urls, int count);

/* 队列管理 */
int modx_queue_submit(const char *url);
int modx_queue_run(int thread_count, const struct modx_cli_opts *opts);

#endif