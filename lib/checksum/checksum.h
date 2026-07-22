#ifndef MODX_CHECKSUM_H
#define MODX_CHECKSUM_H

#include <stddef.h>

/* MD5 */
void modx_md5(const unsigned char *data, size_t len, unsigned char out[16]);
char* modx_md5_str(const unsigned char *data, size_t len, char *out, size_t out_size);

/* SHA256 */
void modx_sha256(const unsigned char *data, size_t len, unsigned char out[32]);
char* modx_sha256_str(const unsigned char *data, size_t len, char *out, size_t out_size);

/* File checksum */
int modx_file_md5(const char *filename, char *out, size_t out_size);
int modx_file_sha256(const char *filename, char *out, size_t out_size);

#endif