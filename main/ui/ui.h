// main/ui/ui.h

#ifndef MODX_UI_H
#define MODX_UI_H

#include <stdarg.h>

void modx_output_init(int quiet, int verbose);

/* 简单输出（无格式化） */
void modx_output(const char *msg);

/* 格式化输出（支持 printf 风格） */
void modx_output_msg(const char *fmt, ...);

/* 详细模式输出 */
void modx_output_verbose(const char *fmt, ...);

/* 错误输出 */
void modx_output_error(const char *fmt, ...);

/* 响应头输出 */
void modx_output_headers(const char *headers);

/* 进度相关 */
void modx_progress_init(int quiet);
void modx_progress_update(long long downloaded, long long total,
                          double speed, int eta);
void modx_progress_finish(void);
void modx_progress_bar(long long downloaded, long long total);

#endif