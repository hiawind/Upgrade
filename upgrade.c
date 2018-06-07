/******************************************************************************
Copyright Shanghai YG Electronic Technology Co., Ltd.
FileName: upgrade.c
Description:  upgrade
******************************************************************************/

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

#include "utils.h"
#include "upgrade.h"
#include "crc.h"

#define MOUNT_DIR "/mnt"
#define UPGRADE_DIR "upgrade"
#define PART_FILE_BOOT "boot.img"
#define PART_FILE_SYSTEM "squashfs"

#define VERSION_FILE "version"

/* current part file information
{0x000000, "splash/splash.data",},  //splash
{0x040000, "rootfs.jffs2",},        //userdata
{0x0c0000, "squashfs",},            //system
{0xa70000, "boot.img",},            //boot
{0xf70000, ubootenv,},              //ubootenv
{0xf80000, "u-boot.bin",},          //uboot
{0xff0000, "zxboot.bin",},          //bootloader
*/

struct mtd_part_info {
    char mtd[8];
    unsigned long size;
    unsigned long offset;
    unsigned long erasesize;
    char part_name[16];
    char part_file[32];
};

struct upg_part_info {
    char mtd[8];
    bool need_upgrade;
    char md5[33];
    unsigned long file_size;
    unsigned long write_size;
};

static char g_upg_path[MAX_PATH];
struct mtd_part_info g_mtd_part_info[MAX_PART_NUM];
struct upg_part_info g_upg_part_info[MAX_PART_NUM];
char version_img[32];
char version_sys[32];

int g_part_num = 0;
unsigned long g_upgrade_size = 0;
unsigned long g_upgyet_size = 0;

void part_info_init() 
{
    memset(g_mtd_part_info, 0, sizeof(struct mtd_part_info)*MAX_PART_NUM);
    memset(g_upg_part_info, 0, sizeof(struct upg_part_info)*MAX_PART_NUM);
    
    strcpy(g_mtd_part_info[0].part_file, "splash.dat");
    strcpy(g_mtd_part_info[1].part_file, "rootfs.jffs2");
    strcpy(g_mtd_part_info[2].part_file, "squashfs");
    strcpy(g_mtd_part_info[3].part_file, "boot.img");
    strcpy(g_mtd_part_info[4].part_file, "ubootenv");
    strcpy(g_mtd_part_info[5].part_file, "u-boot.bin");
    strcpy(g_mtd_part_info[6].part_file, "zxboot.bin");
}

bool check_file()
{
    char name[MAX_PATH];
    char filePath[MAX_PATH];
    struct dirent *dp;
    DIR *dfd = NULL;
    struct stat stbuf;
    bool file_exist = false;

    if((dfd = opendir(MOUNT_DIR)) == NULL)
    {
        printf("cannot open %s\n", MOUNT_DIR);
        return false;
    }

    memset(g_upg_path, 0, MAX_PATH);

    while((dp = readdir(dfd)) != NULL)
    {
        if(strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
        {
            continue;
        }

        sprintf(name, "%s/%s", MOUNT_DIR, dp->d_name);
        printf("mount dir : %s\n", name);

        //sprintf(name, "%s/%s", name, UPGRADE_DIR);
        //printf("upgrade dir: %s\n", name);

        sprintf(filePath, "%s/%s", name, PART_FILE_BOOT);
        printf("boot file path: %s\n", filePath);
        if(stat(filePath, &stbuf) == -1)
        {
            printf("error, boot file is not exist!\n");
            //continue;
        }

        sprintf(filePath, "%s/%s", name, PART_FILE_SYSTEM);
        printf("squashfs file path: %s\n", filePath);
        if(stat(filePath, &stbuf) == -1)
        {
            printf("error, squashfs file is not exist!\n");
            //continue;
        }
        
        sprintf(filePath, "%s/%s", name, "md5");
        printf("md5 file path: %s\n", filePath);
        if(stat(filePath, &stbuf) == -1)
        {
            printf("error, md5 file is not exist!\n");
            continue;
        }

        strcpy(g_upg_path, name);
        file_exist = true;
        break;
    }

    return file_exist;

}

int calc_usb_device_count()
{
    int cnt = 0;
    struct dirent *dp;
    DIR *dfd = NULL;

    if((dfd = opendir(MOUNT_DIR)) == NULL)
    {
        printf("cannot open %s\n", MOUNT_DIR);
        return false;
    }

    while((dp = readdir(dfd)) != NULL)
    {
        if(strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
        {
            continue;
        }

        cnt++;
    }

    return cnt;
}

bool mtd_parse_info()
{
    char buff[MAX_PATH];
    FILE *fp = NULL;
    char str[16];
    int i = 0;
    
    memset(buff, 0, MAX_PATH);
    memset(str, 0, 16);

    fp = fopen("/proc/mtd", "r");
    if(fp == NULL) {
        printf("open /proc/mtd error\n");
        return false;
    }

    fgets(buff, 255, fp);
    //printf("%s\n", buff );
    int index = 0;
    unsigned long offset = 0;

    while(fgets(buff, 255, fp)) {
        /*char temp[128];
        if(strSplit(buff, ':', 0, temp, sizeof(temp)-1) == NULL) {
            printf("read mtd error, return\n");
            return false;
        }*/

        //printf("temp: %s\n", temp);
        int no, size, erasesize;
        char name[32];
    
        if(sscanf(buff, "mtd%d: %08x %08x \"%s\"", &no, &size, &erasesize, name) != 4 || no != index) {
            printf("cannot get part%d info, return\n", index);
            return false;
        }

        char* ptr;
        for(ptr = name; *ptr && *ptr != '\"'; ptr++);
        *ptr = 0;

        sprintf(g_mtd_part_info[index].mtd, "mtd%d", no);
        g_mtd_part_info[index].size = size;
        g_mtd_part_info[index].offset = offset;
        g_mtd_part_info[index].erasesize = erasesize;
        strcpy(g_mtd_part_info[index].part_name, name);

        index++;
        offset += size;
    }

    printf("\ntotal part num: %d\n", index);
    g_part_num = index;
    for(i = 0; i < g_part_num; i++) {
        printf("%s, size: 0x%08lx, offset: 0x%08lx, erasesize: 0x%08lx, part name: %s, \tfile: %s\n", \
                g_mtd_part_info[i].mtd, g_mtd_part_info[i].size, g_mtd_part_info[i].offset, g_mtd_part_info[i].erasesize, g_mtd_part_info[i].part_name, g_mtd_part_info[i].part_file);
    }

    fclose(fp);
    
    return true;
}

int get_ver_md5(char *version_path) 
{
    char file[MAX_PATH];
    char buff[MAX_PATH];
    FILE *fp = NULL;
    char *tmp = NULL;
    char str[16];
    int len = 0;
    
    memset(buff, 0, MAX_PATH);
    memset(str, 0, 16);
    
    // get system information
    if(strlen(version_path) == 0)
        sprintf(file, "%s/%s", "/system", VERSION_FILE);
    else 
        sprintf(file, "%s/%s", version_path, VERSION_FILE);
    
    fp = fopen(file, "r");
    if(fp == NULL) {
        printf("open %s error\n", file);
        return -1;
    }

    fgets(buff, 255, fp);
    len = strlen(buff);
    buff[len-1] = '\0';
    tmp = strchr(buff, '=');
    strcpy(version_sys, ++tmp);
    printf("%s, tmp: %s, version_sys: %s\n", __FUNCTION__, tmp, version_sys);

    if(fp != NULL) {
        fclose(fp);
        fp = NULL;
    }

    // get img information    
    sprintf(file, "%s/%s", g_upg_path, VERSION_FILE);
    
    fp = fopen(file, "r");
    if(fp == NULL) {
        printf("open %s error\n", file);
        return -1;
    }

    fgets(buff, 255, fp);
    len = strlen(buff);
    buff[len-1] = '\0';
    tmp = strchr(buff, '=');
    strcpy(version_img, ++tmp);
    printf("%s, tmp: %s, version_img: %s\n", __FUNCTION__, tmp, version_img);

    if(fp != NULL) {
        fclose(fp);
        fp = NULL;
    }

    // get md5 file information
    sprintf(file, "%s/%s", g_upg_path, "md5");
    fp = fopen(file, "r");
    if(fp == NULL) {
        printf("open %s error\n", file);
        return -1;
    }

    while(fgets(buff, 255, fp)) {
        char md5[33];
        char part_file[32];
        sscanf(buff, "%s %s", md5, part_file);

        for(int i = 0; i < g_part_num; i++) {
            if(strcmp(g_mtd_part_info[i].part_file, part_file) == 0) {
                g_upg_part_info[i].need_upgrade = true;
                strcpy(g_upg_part_info[i].md5, md5);
                break;
            }
        }
    }

    fclose(fp);

    return 0;
}

bool check_ver_md5(char *path) 
{
    struct stat stbuf;
    
    // get system and image version info
    if(get_ver_md5(path) < 0) {
        printf("cannot get version info, return\n");
        return false;
    }

    if(strcmp(version_img, version_sys) <= 0) {
        printf("Img version (%s) <= system version (%s), no need upgrade\n", version_img, version_sys);
        return false;
    }

    for(int i = 0; i < g_part_num; i++) {
        if(g_upg_part_info[i].need_upgrade) {
            char file[MAX_PATH];
            sprintf(file, "%s/%s", g_upg_path, g_mtd_part_info[i].part_file);

            if(stat(file, &stbuf) == -1) {
                printf("cannot find part%d file: %s\n", i, g_mtd_part_info[i].part_file);
                g_upg_part_info[i].need_upgrade = false;
                continue;
            } else {
                g_upg_part_info[i].file_size = stbuf.st_size;
                g_upg_part_info[i].write_size = ALIGN(g_upg_part_info[i].file_size, g_mtd_part_info[i].erasesize);
                g_upgrade_size += g_upg_part_info[i].write_size;
            }
            
            if(verify_image_file(file, g_upg_part_info[i].md5, 0, 0) != 0) {
                printf("part%d md5 error!\n", i);
                return false;
            }

            char tmp_md5[33];
            if(get_md5_plus(file, tmp_md5, g_mtd_part_info[i].size)) {
                printf("get part%d tmp md5 failed!\n", i);
                return false;
            }

            printf("file: %s, size: %d, tmp_md5: %s\n", file, g_mtd_part_info[i].size, tmp_md5);

            sprintf(file, "/dev/%s", g_mtd_part_info[i].mtd);
            if(verify_image_file(file, tmp_md5, 1, g_mtd_part_info[i].size) == 0) {
                printf("%s md5 of flash and image are the same, not upgrade!\n\n", file);
                g_upg_part_info[i].need_upgrade = false;
            } else {
                printf("%s md5 of flash and image are different, need to upgrade!\n\n", file);
            }
        }
    }

    return true;
}

int mtd_erase_part(int Fd, int start, int count, int unlock) 
{
    mtd_info_user meminfo;

    memset(&meminfo, 0, sizeof(mtd_info_user));

    if(ioctl(Fd, MEMGETINFO, &meminfo) == 0) {
        erase_info_user erase;
        erase.start = start;
        erase.length = meminfo.erasesize;
        
        printf("----------------Erase Start----------------\n");
        for(; count > 0; count--) {
            //printf("Performing Flash Erase of length %u at offset 0x%x\n", erase.length, erase.start);
            printf("*");
            fflush(stdout);

            if(unlock != 0) {
                //Unlock the sector first
                printf("unlock\n");
                if(ioctl(Fd, MEMUNLOCK, &erase) != 0) {
                    perror("MTD Unlock fail\n");
                    close(Fd);
                    return -1;
                }
            }

            if(ioctl(Fd, MEMERASE, &erase) != 0) {
                perror("MTD Erase Failure\n");
                close(Fd);
                return -1;
            }

            erase.start += meminfo.erasesize;
        }

        printf("\n----------------Erase Done-----------------\n");
    }

    return 0;
}

bool mtd_write_part(int index, const char *mtd) 
{
    bool reto = true;
    int ret = 0;
    //i = 0;
    int fd = -1, ifd = -1;
    struct mtd_info_user info;
    char mtdDev[16];
    char file_path[MAX_PATH];
    int regcount;
    int res = 0, write_cnt = 0, read_cnt = 0;
    int imglen, imglen1;
    int readlen = 0;
    char *write_buf = NULL;
    char *read_buf = NULL;
    //int dump_len = 256;
    int progressl = 0, progresst = 0;
    unsigned long upgyet_size = 0;
    struct timeval tv1;
    struct timeval tv2;

    memset(mtdDev, 0, 16);

    printf("%s, part%d: %s\n", __FUNCTION__, index, mtd);

    sprintf(mtdDev, "/dev/%s", mtd);

    memset(&info, 0, sizeof(struct mtd_info_user));
    if((fd = open(mtdDev, O_RDWR)) < 0) {
        printf("%s open error: %d\n", mtdDev, errno);
        reto = false;
        goto END;
    } else {
        printf("%s open, fd: %d\n",mtdDev, fd);
        ret = ioctl(fd, MEMGETINFO, &info);
        printf("ret: %d\n", ret);
        printf("size: 0x%08x, erasesize: 0x%08x, writesize: 0x%08x, oobsize: 0x%08x\n", \
                info.size, info.erasesize, info.writesize, info.oobsize);
    }

    if(ioctl(fd, MEMGETREGIONCOUNT, &regcount) == 0) {
        printf("regcount: %d\n", regcount);
    }

    readlen = g_upg_part_info[index].file_size;//info.erasesize*16;
    write_buf = (char*)malloc(readlen);
    if(write_buf == NULL) {
        printf("error, cannot malloc for write buf");
        reto = false;
        goto END;
    }
/*
    read_buf = (char*)malloc(readlen);
    if(read_buf == NULL) {
        printf("error, cannot malloc for read buf");
        reto = false;
        goto END;
    }

    lseek(fd, 0, SEEK_SET);
    ret = read(fd, write_buf, readlen);
    if(ret != readlen) {
        printf("read mtddevice error\n");
        reto = false;
        goto END;
    } else {
        //hexdump(write_buf, dump_len);
    }
*/
    sprintf(file_path, "%s/%s", g_upg_path, g_mtd_part_info[index].part_file);
    printf("file_path: %s\n", file_path);
    if((ifd = open(file_path, O_RDONLY)) == -1) {
        printf("open input file error\n");
        reto = false;
        goto END;
    } else {
        imglen = lseek(ifd, 0, SEEK_END);
        lseek(ifd, 0, SEEK_SET);
        printf("The input file image len is: %d\n", imglen);
    }

/*    
    ret = read(ifd, write_buf, readlen);
    if(ret != readlen) {
        printf("read error, ret: %d, readlen: %d\n", ret, readlen);
    }

    //hexdump(write_buf, dump_len);
*/

// erase part
    if(regcount == 0) {
        res = mtd_erase_part(fd, 0, (info.size/info.erasesize), 0);
        printf("erase, res: %d\n", res);
    }

// write part
    printf("part size: 0x%08x\n", info.size);
    imglen1 = imglen;
    upgyet_size = 0;
    lseek(ifd, 0, SEEK_SET);
    lseek(fd, 0, SEEK_SET);
    gettimeofday(&tv1, NULL);
    printf("tv1: %ld\n", tv1.tv_sec);
    
    while(imglen) {
        memset(write_buf, 0xFF, readlen);
        if((read_cnt = read(ifd, write_buf, readlen)) != readlen) {
            if(read_cnt < readlen) {
                printf("The file is end\n");
            } else {
                printf("File I/O error on input file\n");
                reto = false;
                goto END;
            }
        }
  //      hexdump(write_buf, 256);
        if((write_cnt = write(fd, write_buf, readlen)) != readlen) {
            printf("write %s error! addr: 0x%08x, write cnt: %d, errno: %d\n", mtdDev, (imglen1 - imglen), write_cnt, errno);
            reto = false;
            goto END;
        } else {
            g_upgyet_size += write_cnt;
            upgyet_size += write_cnt;
            progressl = upgyet_size*100/g_upg_part_info[index].write_size;
            progresst = g_upgyet_size*100/g_upgrade_size;
            
            printf("write %s ok！addr: 0x%08x, readlen: %d, write cnt: %d, progress: %d%%/%d%%\n", mtdDev, (imglen1 - imglen), readlen, write_cnt, progressl, progresst);
          
        }
        //fflush(stdout);
        //fsync(fd);
        /*
        if( (read_cnt = read(fd, read_buf, readlen)) != readlen ) {
            printf("read error, readcnt: %d\n", read_cnt);
            reto = false;
            goto END;
        } else {
            hexdump(read_buf, 256);
            //break;
        }
*/

        if(imglen >= read_cnt) {
            imglen -= read_cnt;
        } else {
            printf("error, image len: %d < read cnt: %d\n", imglen, read_cnt);
        }

    }

    gettimeofday(&tv2, NULL);
    printf("tv2: %ld, elapsed time: %lds\n", tv2.tv_sec, tv2.tv_sec-tv1.tv_sec);

    print_size(readlen/(tv2.tv_sec-tv1.tv_sec), "/s");

    printf("\nwrite done\n");

END:
    //fflush(stdout);
    //fsync(fd);
    ret = close(fd);
    if(ret == -1) {
        printf("close fd %d error(%d)\n", fd, errno);
        reto = false;
    }

    ret = close(ifd);
    if(ret == -1) {
        printf("close fd %d error(%d)\n", ifd, errno);
        reto = false;
    }

    if(write_buf != NULL) {
        free(write_buf);
        write_buf = NULL;
    }

    if(read_buf != NULL) {
        free(read_buf);
        read_buf = NULL;
    }

    return reto;
}



int verify_image_file(const char* img_file_name, const char* md5, int flag, unsigned long size)
{
    unsigned char digest[16];

    if(flag == 0) {
        if(md5_file(img_file_name, digest) != 0)
            return -1;
    } else if(flag == 1) {
        if(md5_part(img_file_name, digest, size) != 0)
            return -1;
    }

    char md5_buf[33];
    int i;
    char* ptr = md5_buf;
    for(i = 0; i < 16; i++)
    {
        sprintf(ptr, "%02x", digest[i]);
        ptr += 2;
    }
    *ptr = 0;

    if(flag == 0) {
        printf("file_md5=%s verify_md5=%s\n", md5_buf, md5);
    } else if(flag == 1) {
        printf("flash part md5=%s verify_md5=%s\n", md5_buf, md5);
    }
    
    if(strcasecmp(md5_buf, md5) == 0)
        return 0;
    return -1;
}

int get_md5_plus(const char *img_file_name, char *md5, unsigned int len) 
{
    unsigned char digest[16];
    int i;
    
    if(md5_file_plus(img_file_name, digest, len) != 0)
        return -1;

    char* ptr = md5;
    for(i = 0; i < 16; i++)
    {
        sprintf(ptr, "%02x", digest[i]);
        ptr += 2;
    }
    *ptr = 0;

    return 0;
}

int upgrade_image_progress(int one_part, char *name) 
{
    int i, cnt = 0;
    
    for(i = 0; i < g_part_num; i++) {
        strcpy(g_upg_part_info[i].mtd, g_mtd_part_info[i].mtd);

        if(one_part == true) {
            if((0 == strcmp(g_mtd_part_info[i].mtd, name)) || (0 == strcmp(g_mtd_part_info[i].part_name, name))) {
                g_upg_part_info[i].need_upgrade = true;
            } else {
                g_upg_part_info[i].need_upgrade = false;
            }
        }

        if(g_upg_part_info[i].need_upgrade) 
            printf("upgPartInfo, mtd%d, filesize: 0x%08lx, writesize: 0x%08lx, g_upgrade_size: 0x%08lx, need_upgrade: %d, file: %s\n", \
                    i, g_upg_part_info[i].file_size, g_upg_part_info[i].write_size, g_upgrade_size, g_upg_part_info[i].need_upgrade, g_mtd_part_info[i].part_file);
    }

    for(i = 0; i < g_part_num; i++) {
        if(g_upg_part_info[i].need_upgrade == true) {
            printf("part%d: %s start to upgrade!\n", i, g_mtd_part_info[i].part_name);
            if(false == mtd_write_part(i, g_mtd_part_info[i].mtd)) {
                printf("upgrade %s part fail, exit\n", g_mtd_part_info[i].part_name);
                return -3;
            } 

            cnt++;
        }
    }

    return cnt;
}

int find_usb_device_port(char *path) 
{
    char mount_name[4];
    char cmd[64];
    char res[1][MAX_PATH];
    char *ptr = NULL;
    int cnt = 0, port = -1;

    if(calc_usb_device_count() == 1) {
        port = 0;
        return port;
    }

    // /mnt/sda1
    strncpy(mount_name, path+5, 3);

    memset(cmd, 0, 64);
    sprintf(cmd, "cd /sys/devices/soc && find -name %s", mount_name);
    cnt = shexec(cmd, res, 1);
    if(cnt <= 0) {
        printf("execute shell cmd error, cnt: %d\n", cnt);
        return -1;
    }

    // ./d9092000.ehci/usb1/1-1/1-1:1.0/host2/target2:0:0/2:0:0:0/block/sdc
    printf("res: %s\n", res[0]);

    ptr = strstr(res[0], "usb1");
    if(ptr == NULL) {
        printf("cannot find usb1 string, return!\n");
        return -1;
    }

    port = ptr[7]-'0';

    printf("ptr: %s, port: %d\n", ptr, port);
    
    return port;
}

#ifdef UBOOT_UPG_SUPPORT
int upgrade_to_uboot_progress(int one_part, char *name) 
{
    int i, cnt = 0;
    char mtd[16], mtddev[16];
    int fd = -1;
    unsigned long size = 0, erasesize = 0;
    char *buf = NULL;
    //char *ptr = NULL;
    struct upg_info_uboot *upg_info = NULL;
    struct env_image_single *uboot_env = NULL;
    int ret = 0;
    unsigned int crc = 0;

    memset(mtd, 0, 16);
    memset(mtddev, 0, 16);
    
    for(i = 0; i < g_part_num; i++) {
        strcpy(g_upg_part_info[i].mtd, g_mtd_part_info[i].mtd);

        if(one_part == true) {
            if((0 == strcmp(g_mtd_part_info[i].mtd, name)) || (0 == strcmp(g_mtd_part_info[i].part_name, name))) {
                g_upg_part_info[i].need_upgrade = true;
            } else {
                g_upg_part_info[i].need_upgrade = false;
            }
        }

        if(strcmp(g_mtd_part_info[i].part_name, "ubootenv") == 0) {
            strcpy(mtd, g_mtd_part_info[i].mtd);
            size = g_mtd_part_info[i].size;
            erasesize = g_mtd_part_info[i].erasesize;
        }

        if(g_upg_part_info[i].need_upgrade) { 

            cnt++;
            printf("mtd%d, filesize: 0x%08lx, writesize: 0x%08lx, g_upgrade_size: 0x%08lx, need_upgrade: %d, file: %s\n", \
                    i, g_upg_part_info[i].file_size, g_upg_part_info[i].write_size, g_upgrade_size, g_upg_part_info[i].need_upgrade, g_mtd_part_info[i].part_file);
        }
    }

    if(cnt == 0) {
        return 0;
    }

    buf = (char*)malloc(size);
    if(buf == NULL) {
        printf("malloc buf error!\n");
        return -1;
    }
    
    // 1. read bootargs
    sprintf(mtddev, "/dev/%s", mtd);
    if((fd = open(mtddev, O_RDWR)) < 0) {
        printf("%s open error: %d\n", mtddev, errno);
        return -1;
    }

    lseek(fd, 0, SEEK_SET);
    ret = read(fd, buf, size);
    if(ret != size) {
        printf("read %s error\n", mtddev);
        return -1;
    } else {
        //hexdump(buf, 256);
    }

    uboot_env = (struct env_image_single*)buf;
    crc = crc32(0, (const unsigned char*)uboot_env->data, size - 4); // uboot_env->crc;
    upg_info = (struct upg_info_uboot*)(buf + 0xFF00);
    printf("buf: 0x%08x, crc: 0x%08x, cal crc: 0x%08x, upg_info: 0x%08x\n", buf, uboot_env->crc, crc, upg_info);

    if(crc != uboot_env->crc) {
        printf("error crc: 0x%08x : 0x%08x\n", crc, uboot_env->crc);
    }

    // 2. modify upg params
    memset(upg_info, 0, 0x100);
    upg_info->upg_flag = 0x01;
    for(i = 0; i < g_part_num; i++) {
        if(g_upg_part_info[i].need_upgrade == true) {
            upg_info->part[i].part_flag = 0x01;
            upg_info->part[i].size = g_mtd_part_info[i].size;
        }
    }

    upg_info->update_cnt = cnt;
    upg_info->usb_port = find_usb_device_port(g_upg_path);
    if(upg_info->usb_port < 0) {
        printf("cannot get %s device port, return!\n");
        return -1;
    }

    uboot_env->crc = crc32(0, (const unsigned char*)uboot_env->data, size - 4);

    printf("new crc: 0x%08x\n", uboot_env->crc);

    //hexdump((const char*)upg_info, 256);

#if 1  
    // 3. write bootargs
    ret = mtd_erase_part(fd, 0, (size/erasesize), 0);
    if(ret < 0) {
        printf("Erase %s error!\n", mtddev);
        return -1;
    }

    lseek(fd, 0, SEEK_SET);
    ret = write(fd, buf, size);
    if(ret != size) {
        printf("write error, ret: %d, size: %d, errorno: %d\n", ret, size, errno);
        return -1;
    }
#endif

    if(buf != NULL) {
        free(buf); buf = NULL;
    }

    ret = close(fd);
    if(ret == -1) {
        printf("close %s error(%d)\n", mtddev, errno);
        return -1;
    }

    return cnt;
}

#endif

// E.O.F
