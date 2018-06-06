/******************************************************************************
Copyright Shanghai YG Electronic Technology Co., Ltd.
FileName: md5.h
Description:  MD5 utility APIs
******************************************************************************/
#ifndef _MD5_H_
#define _MD5_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    unsigned int total[2];
    unsigned int state[4];
    unsigned char buffer[64];
} MD5_CTX;

extern void md5_init(MD5_CTX* ctx);
extern void md5_starts(MD5_CTX* ctx);
extern void md5_process(MD5_CTX* ctx, const unsigned char data[64]);
extern void md5_update(MD5_CTX* ctx, const unsigned char *input, unsigned int input_len);
extern void md5_finish(MD5_CTX* ctx, unsigned char output[16]);
extern void md5_buffer(const unsigned char *buffer, int buf_len, unsigned char digest[16]);
extern int md5_file(const char *filename, unsigned char digest[16]);
extern int md5_file_plus(const char * filename, unsigned char digest [ 16 ], unsigned int size);
int md5_part(const char *mtdname, unsigned char digest[16]);
extern int md5_buffer_string(const unsigned char *buffer, int buf_len, char* md5_str, int capital);
extern void md5_hex_string(const unsigned char *md5_hex, char* md5_str, int capital);

#ifdef __cplusplus
}
#endif

#endif // _MD5_H_
// E.O.F
