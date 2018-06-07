/******************************************************************************
Copyright Shanghai YG Electronic Technology Co., Ltd.
FileName: md5.c
Description:  MD5 utility
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <linux/netlink.h>
#include <mtd/mtd-abi.h>
#include <stdint.h>
#include <errno.h>
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <memory.h>
#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#include "md5.h"

/*
 * 32-bit integer manipulation macros (little endian)
 */
#ifndef GET_UINT32_LE
#define GET_UINT32_LE(n,b,i)                            \
{                                                       \
    (n) = ( (uint32_t) (b)[(i)    ]       )             \
        | ( (uint32_t) (b)[(i) + 1] <<  8 )             \
        | ( (uint32_t) (b)[(i) + 2] << 16 )             \
        | ( (uint32_t) (b)[(i) + 3] << 24 );            \
}
#endif

#ifndef PUT_UINT32_LE
#define PUT_UINT32_LE(n,b,i)                                    \
{                                                               \
    (b)[(i)    ] = (unsigned char) ( ( (n)       ) & 0xFF );    \
    (b)[(i) + 1] = (unsigned char) ( ( (n) >>  8 ) & 0xFF );    \
    (b)[(i) + 2] = (unsigned char) ( ( (n) >> 16 ) & 0xFF );    \
    (b)[(i) + 3] = (unsigned char) ( ( (n) >> 24 ) & 0xFF );    \
}
#endif

void md5_init(MD5_CTX* ctx)
{
    memset(ctx, 0, sizeof(MD5_CTX));
}

/*
 * MD5 context setup
 */
void md5_starts(MD5_CTX* ctx)
{
    ctx->total[0] = 0;
    ctx->total[1] = 0;

    ctx->state[0] = 0x67452301;
    ctx->state[1] = 0xEFCDAB89;
    ctx->state[2] = 0x98BADCFE;
    ctx->state[3] = 0x10325476;
}

void md5_process(MD5_CTX* ctx, const unsigned char data[64])
{
    unsigned int X[16], A, B, C, D;

    GET_UINT32_LE( X[ 0], data,  0 );
    GET_UINT32_LE( X[ 1], data,  4 );
    GET_UINT32_LE( X[ 2], data,  8 );
    GET_UINT32_LE( X[ 3], data, 12 );
    GET_UINT32_LE( X[ 4], data, 16 );
    GET_UINT32_LE( X[ 5], data, 20 );
    GET_UINT32_LE( X[ 6], data, 24 );
    GET_UINT32_LE( X[ 7], data, 28 );
    GET_UINT32_LE( X[ 8], data, 32 );
    GET_UINT32_LE( X[ 9], data, 36 );
    GET_UINT32_LE( X[10], data, 40 );
    GET_UINT32_LE( X[11], data, 44 );
    GET_UINT32_LE( X[12], data, 48 );
    GET_UINT32_LE( X[13], data, 52 );
    GET_UINT32_LE( X[14], data, 56 );
    GET_UINT32_LE( X[15], data, 60 );

#define S(x,n) ((x << n) | ((x & 0xFFFFFFFF) >> (32 - n)))

#define P(a,b,c,d,k,s,t)                                \
{                                                       \
    a += F(b,c,d) + X[k] + t; a = S(a,s) + b;           \
}

    A = ctx->state[0];
    B = ctx->state[1];
    C = ctx->state[2];
    D = ctx->state[3];

#define F(x,y,z) (z ^ (x & (y ^ z)))

    P( A, B, C, D,  0,  7, 0xD76AA478 );
    P( D, A, B, C,  1, 12, 0xE8C7B756 );
    P( C, D, A, B,  2, 17, 0x242070DB );
    P( B, C, D, A,  3, 22, 0xC1BDCEEE );
    P( A, B, C, D,  4,  7, 0xF57C0FAF );
    P( D, A, B, C,  5, 12, 0x4787C62A );
    P( C, D, A, B,  6, 17, 0xA8304613 );
    P( B, C, D, A,  7, 22, 0xFD469501 );
    P( A, B, C, D,  8,  7, 0x698098D8 );
    P( D, A, B, C,  9, 12, 0x8B44F7AF );
    P( C, D, A, B, 10, 17, 0xFFFF5BB1 );
    P( B, C, D, A, 11, 22, 0x895CD7BE );
    P( A, B, C, D, 12,  7, 0x6B901122 );
    P( D, A, B, C, 13, 12, 0xFD987193 );
    P( C, D, A, B, 14, 17, 0xA679438E );
    P( B, C, D, A, 15, 22, 0x49B40821 );

#undef F

#define F(x,y,z) (y ^ (z & (x ^ y)))

    P( A, B, C, D,  1,  5, 0xF61E2562 );
    P( D, A, B, C,  6,  9, 0xC040B340 );
    P( C, D, A, B, 11, 14, 0x265E5A51 );
    P( B, C, D, A,  0, 20, 0xE9B6C7AA );
    P( A, B, C, D,  5,  5, 0xD62F105D );
    P( D, A, B, C, 10,  9, 0x02441453 );
    P( C, D, A, B, 15, 14, 0xD8A1E681 );
    P( B, C, D, A,  4, 20, 0xE7D3FBC8 );
    P( A, B, C, D,  9,  5, 0x21E1CDE6 );
    P( D, A, B, C, 14,  9, 0xC33707D6 );
    P( C, D, A, B,  3, 14, 0xF4D50D87 );
    P( B, C, D, A,  8, 20, 0x455A14ED );
    P( A, B, C, D, 13,  5, 0xA9E3E905 );
    P( D, A, B, C,  2,  9, 0xFCEFA3F8 );
    P( C, D, A, B,  7, 14, 0x676F02D9 );
    P( B, C, D, A, 12, 20, 0x8D2A4C8A );

#undef F

#define F(x,y,z) (x ^ y ^ z)

    P( A, B, C, D,  5,  4, 0xFFFA3942 );
    P( D, A, B, C,  8, 11, 0x8771F681 );
    P( C, D, A, B, 11, 16, 0x6D9D6122 );
    P( B, C, D, A, 14, 23, 0xFDE5380C );
    P( A, B, C, D,  1,  4, 0xA4BEEA44 );
    P( D, A, B, C,  4, 11, 0x4BDECFA9 );
    P( C, D, A, B,  7, 16, 0xF6BB4B60 );
    P( B, C, D, A, 10, 23, 0xBEBFBC70 );
    P( A, B, C, D, 13,  4, 0x289B7EC6 );
    P( D, A, B, C,  0, 11, 0xEAA127FA );
    P( C, D, A, B,  3, 16, 0xD4EF3085 );
    P( B, C, D, A,  6, 23, 0x04881D05 );
    P( A, B, C, D,  9,  4, 0xD9D4D039 );
    P( D, A, B, C, 12, 11, 0xE6DB99E5 );
    P( C, D, A, B, 15, 16, 0x1FA27CF8 );
    P( B, C, D, A,  2, 23, 0xC4AC5665 );

#undef F

#define F(x,y,z) (y ^ (x | ~z))

    P( A, B, C, D,  0,  6, 0xF4292244 );
    P( D, A, B, C,  7, 10, 0x432AFF97 );
    P( C, D, A, B, 14, 15, 0xAB9423A7 );
    P( B, C, D, A,  5, 21, 0xFC93A039 );
    P( A, B, C, D, 12,  6, 0x655B59C3 );
    P( D, A, B, C,  3, 10, 0x8F0CCC92 );
    P( C, D, A, B, 10, 15, 0xFFEFF47D );
    P( B, C, D, A,  1, 21, 0x85845DD1 );
    P( A, B, C, D,  8,  6, 0x6FA87E4F );
    P( D, A, B, C, 15, 10, 0xFE2CE6E0 );
    P( C, D, A, B,  6, 15, 0xA3014314 );
    P( B, C, D, A, 13, 21, 0x4E0811A1 );
    P( A, B, C, D,  4,  6, 0xF7537E82 );
    P( D, A, B, C, 11, 10, 0xBD3AF235 );
    P( C, D, A, B,  2, 15, 0x2AD7D2BB );
    P( B, C, D, A,  9, 21, 0xEB86D391 );

#undef F

    ctx->state[0] += A;
    ctx->state[1] += B;
    ctx->state[2] += C;
    ctx->state[3] += D;
}

/*
 * MD5 process buffer
 */
void md5_update(MD5_CTX* ctx, const unsigned char *input, unsigned int input_len)
{
    unsigned int fill;
    unsigned int left;

    if( input_len == 0 )
        return;

    left = ctx->total[0] & 0x3F;
    fill = 64 - left;

    ctx->total[0] += (uint32_t) input_len;
    ctx->total[0] &= 0xFFFFFFFF;

    if( ctx->total[0] < (uint32_t) input_len )
        ctx->total[1]++;

    if( left && input_len >= fill )
    {
        memcpy( (void *) (ctx->buffer + left), input, fill );
        md5_process( ctx, ctx->buffer );
        input += fill;
        input_len  -= fill;
        left = 0;
    }

    while( input_len >= 64 )
    {
        md5_process( ctx, input );
        input += 64;
        input_len  -= 64;
    }

    if( input_len > 0 )
    {
        memcpy( (void *) (ctx->buffer + left), input, input_len );
    }
}

static const unsigned char md5_padding[64] =
{
 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/*
 * MD5 final digest
 */
void md5_finish(MD5_CTX* ctx, unsigned char output[16])
{
    unsigned int last, padn;
    unsigned int high, low;
    unsigned char msglen[8];

    high = ( ctx->total[0] >> 29 )
         | ( ctx->total[1] <<  3 );
    low  = ( ctx->total[0] <<  3 );

    PUT_UINT32_LE( low,  msglen, 0 );
    PUT_UINT32_LE( high, msglen, 4 );

    last = ctx->total[0] & 0x3F;
    padn = ( last < 56 ) ? ( 56 - last ) : ( 120 - last );

    md5_update( ctx, md5_padding, padn );
    md5_update( ctx, msglen, 8 );

    PUT_UINT32_LE( ctx->state[0], output,  0 );
    PUT_UINT32_LE( ctx->state[1], output,  4 );
    PUT_UINT32_LE( ctx->state[2], output,  8 );
    PUT_UINT32_LE( ctx->state[3], output, 12 );
}

void md5_buffer(const unsigned char *buffer, int buf_len, unsigned char digest[16])
{
	MD5_CTX context;

	md5_init(&context);
	md5_starts(&context);
	md5_update(&context, (unsigned char *)buffer, buf_len);
	md5_finish(&context, digest);
}

/* Digests a file and prints the result.
 */
int md5_file(const char *filename, unsigned char digest[16])
{
	FILE *file;
	MD5_CTX context;
	int len;
	unsigned char buffer[256];

	if ((file = fopen (filename, "rb")) == NULL)
	{
        printf("Open file error: %s\n", filename);
		return -1;
	}
	else
	{
		md5_init(&context);
		md5_starts(&context);
		while ((len = fread (buffer, 1, sizeof(buffer), file)) > 0)
			md5_update(&context, buffer, len);
		md5_finish(&context, digest);
		fclose (file);
	}
	return 0;
}

/* Digests a file and prints the result.
 */
int md5_file_plus(const char *filename, unsigned char digest[16], unsigned int size)
{
	FILE *file;
	MD5_CTX context;
	int len, total_len = 0;
	unsigned char buffer[256];

	if ((file = fopen (filename, "rb")) == NULL)
	{
        printf("Open file error: %s\n", filename);
		return -1;
	}
	else
	{
		md5_init(&context);
		md5_starts(&context);

        while(size) {
            if((len = fread (buffer, 1, sizeof(buffer), file)) > 0) {
                if(size >= len)
                    size -= len;
            } else {
                len = sizeof(buffer);
                if(size >=len)
                    size -= len;
                else {
                    len = size;
                    size = 0;
                }
                memset(buffer, 0xFF, len);    
            }

            md5_update(&context, buffer, len);
        }
        
		md5_finish(&context, digest);
		fclose (file);
	}
	return 0;
}


/* Digests a part and prints the result.
 */
int md5_part(const char *mtdname, unsigned char digest[16], unsigned long size)
{
	int fd = -1;
	MD5_CTX context;
	int len = 0;
	unsigned char buffer[256];
    unsigned char *ptr = NULL;
    unsigned char *tmp = NULL;

	if ((fd = open (mtdname, O_RDONLY)) < 0)
	{
        printf("Open file error: %s\n", mtdname);
		return -1;
	}
	else
	{
		md5_init(&context);
		md5_starts(&context);

        ptr = (unsigned char*)malloc(size);
        if(ptr == NULL) {
            printf("cannot malloc size: %d\n");
            return -1;
        }

        if(read(fd, ptr, size) != size) {
            printf("read %s error, len: %d\n", mtdname);
            return -1;
        }

        len = sizeof(buffer);
        tmp = ptr;
		do {
            memcpy(buffer, tmp, len);
		    md5_update(&context, buffer, len);

            tmp += len;
            if(size >= len) {
                size -= len;
            } else {
                len = size;
            }
            
        } while(size > 0);
        
		md5_finish(&context, digest);
		close (fd);

        free(ptr); ptr = NULL;
	}
	return 0;
}


int md5_buffer_string(const unsigned char *buffer, int buf_len, char* md5_str, int capital)
{
    unsigned char digest[16];
    md5_buffer(buffer, buf_len, digest);
    md5_hex_string(digest, md5_str, capital);
    return 0;
}

void md5_hex_string(const unsigned char *md5_hex, char* md5_str, int capital)
{
    int i;
    char* ptr = md5_str;
    for(i = 0; i < 16; i++)
    {
        if(capital)
            sprintf(ptr, "%02X", md5_hex[i]);
        else
            sprintf(ptr, "%02x", md5_hex[i]);
        ptr += 2;
    }
    *ptr = 0;
}

// E.O.F
