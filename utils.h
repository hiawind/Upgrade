#ifndef _UTILS_H_
#define _UTILS_H_

#include <linux/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/un.h>
#include <sys/types.h>
#include <ctype.h>

#define ALIGN(size, mod) ((size%mod)?(((size/mod)+1)*mod):size)

unsigned int StrToHex(char *pszSrc, int nLen);
void removeChar(char *str, char c);
void hexdump(char *buf, int len);
int shexec(const char *cmd, char res[][512], int count);

#endif

