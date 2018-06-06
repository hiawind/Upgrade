/******************************************************************************
Copyright Shanghai YG Electronic Technology Co., Ltd.
FileName: upgrade.h
Description:  upgrade APIs
******************************************************************************/
#ifndef _UPGRADE_H_
#define _UPGRADE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <strings.h>

#include "md5.h"

#define VERSION_NO "0.1"

#define UBOOT_UPG_SUPPORT

#define MAX_PATH 512
#define MAX_PART_NUM 16

struct upg_info_uboot {
    bool flag;
    int update_cnt;
    char part_idx[8];
    int usb_port; 
};

struct env_image_single {
	uint32_t	crc;	/* CRC32 over data bytes    */
	char		data[];
};

void part_info_init();
bool check_file();
bool mtd_parse_info();
bool check_ver_md5(char *path);
int verify_image_file(const char* img_file_name, const char* md5, int flag);
int get_md5_plus(const char *img_file_name, char *md5, unsigned int len);
int upgrade_image_progress(int one_part, char* name);

#ifdef UBOOT_UPG_SUPPORT
int upgrade_to_uboot_progress(int one_part, char* name);
#endif

#ifdef __cplusplus
}
#endif


#endif

