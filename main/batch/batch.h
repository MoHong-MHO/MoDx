#ifndef MODX_BATCH_H
#define MODX_BATCH_H

int modx_batch_read(const char *filename, char ***urls, int *count);
void modx_batch_free(char **urls, int count);

int modx_queue_submit(const char *url);
int modx_queue_run(int thread_count);

#endif