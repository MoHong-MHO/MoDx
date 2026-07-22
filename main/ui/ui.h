#ifndef MODX_UI_H
#define MODX_UI_H

void modx_output_init(int quiet, int verbose);
void modx_progress_init(int quiet);
void modx_progress_update(long long downloaded, long long total,
                          double speed, int eta);
void modx_progress_finish(void);
void modx_progress_bar(long long downloaded, long long total);

void modx_output(const char *msg);
void modx_output_verbose(const char *msg, ...);
void modx_output_error(const char *msg, ...);
void modx_output_headers(const char *headers);

#endif