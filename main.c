#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <mtd/mtd-abi.h>
#include <stdint.h>
#include <errno.h>
#include <arpa/inet.h> 
#include <netinet/in.h> 

#include "utils.h"

//#define UPGRADE_MORE

#define MAX_PATH 512
#define MAX_PART_NUM 16
#define MTD_WRITE_LEN 0x10000
#define UEVENT_BUFFER_SIZE 2048

#define MOUNT_DIR "/mnt"
#define UPGRADE_DIR "upgrade"
#define PART_FILE_BOOT "boot.img"
#define PART_FILE_SYSTEM "squashfs"

#define PART_NAME_BOOT "boot"
#define PART_NAME_SYSTEM "system"

#define VERSION_FILE "version"
#define VERSION_NO "0.1"

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
    unsigned long erasesize;
    char part_name[16];
    char part_file[32];
    bool need_upgrade;
};

struct upg_part_info {
    char mtd[8];
    unsigned long file_size;
    unsigned long write_size;
};

static char g_upg_path[MAX_PATH];
struct mtd_part_info mtdPartInfo[MAX_PART_NUM];
struct upg_part_info upgPartInfo[MAX_PART_NUM];
char version_img[32];
char version_sys[32];
bool upg_more = false;
int g_part_num = 0;
unsigned long g_upgrade_size = 0;
unsigned long g_upgyet_size = 0;

/*
static int init_hotplug_sock() 
{
    const int buffer_size = 1024;

    int ret;

    struct sockaddr_nl snl;
    bzero(&snl, sizeof(struct sockaddr_nl));

    return 0;
}
*/

void partInfoInit() 
{
    memset(mtdPartInfo, 0, sizeof(struct mtd_part_info)*MAX_PART_NUM);
    
    strcpy(mtdPartInfo[0].part_file, "splash.dat");
    strcpy(mtdPartInfo[1].part_file, "rootfs.jffs2");
    strcpy(mtdPartInfo[2].part_file, "squashfs");
    mtdPartInfo[2].need_upgrade = true;
    strcpy(mtdPartInfo[3].part_file, "boot.img");
    mtdPartInfo[3].need_upgrade = true;
    strcpy(mtdPartInfo[4].part_file, "ubootenv");
    strcpy(mtdPartInfo[5].part_file, "u-boot.bin");
    strcpy(mtdPartInfo[6].part_file, "zxboot.bin");

    if(upg_more) {
        mtdPartInfo[1].need_upgrade = true; //userdata
        mtdPartInfo[5].need_upgrade = true; //uboot
    }
}

bool checkFile()
{
    printf("%s, enter\n", __FUNCTION__);

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

        sprintf(name, "%s/%s", name, UPGRADE_DIR);
        printf("upgrade dir: %s\n", name);

        sprintf(filePath, "%s/%s", name, PART_FILE_BOOT);
        printf("boot file path: %s\n", filePath);
        if(stat(filePath, &stbuf) == -1)
        {
            printf("error, boot file is not exist!\n");
            continue;
        }

        sprintf(filePath, "%s/%s", name, PART_FILE_SYSTEM);
        printf("squashfs file path: %s\n", filePath);
        if(stat(filePath, &stbuf) == -1)
        {
            printf("error, squashfs file is not exist!\n");
            continue;
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

bool parsePartTable()
{
    char buff[MAX_PATH];
    char buffBak[MAX_PATH];
    FILE *fp = NULL;
    char str[16];
    int len = 0;
    int i = 0, j = 0;
    
    memset(buff, 0, MAX_PATH);
    memset(str, 0, 16);

    fp = fopen("/proc/mtd", "r");
    if(fp == NULL) {
        printf("open /proc/mtd error\n");
        return false;
    }

    //fscanf(fp, "%s", buff);
    //printf("1: %s\n", buff );
                 
    fgets(buff, 255, fp);
    printf("%s\n", buff );
    char *tmp = NULL;
    char *tmpBuf = NULL;

    while(!feof(fp)) {
                         
        fgets(buff, 255, fp);
        printf("\n%s", buff );
        
        len = strlen(buff);
        //printf("len: %d\n", len);

        if(0 == strcmp(buff, buffBak)) {
            printf("str is same, buff: %s\n", buff);
            continue;
        }

        strcpy(buffBak, buff);

        tmp = str;
        tmpBuf = buff;
        i = 0;

        while(1) {
            //printf("buff: %c ", *tmpBuf);
            if( (*tmpBuf != '\n') && (*tmpBuf != ' ') ) {
                *tmp = *tmpBuf;
                tmp++;
                tmpBuf++;
            } else {
                *tmp = '\0';
                tmp = str;

                //printf("i: %d, str: %s\n", i, str);
                if(i == 0) {
                    removeChar((char*)(str), ':');
                    strcpy(mtdPartInfo[j].mtd, str);
                }
                else if(i == 1)
                    mtdPartInfo[j].size = StrToHex(str, 4);
                else if(i == 2)
                    mtdPartInfo[j].erasesize = StrToHex(str, 4);
                else if(i == 3) {
                    removeChar((char*)(str), '\"');
                    strcpy(mtdPartInfo[j].part_name, str);
                }
                
                if(*tmpBuf == '\n') {
                    printf("parse line end\n");
                    j++;
                    break;
                }

                i++;
                //printf("i: %d, j: %d\n", i, j);
                memset(str, 0, 16);
                tmpBuf++;
            }

        }

    }

    printf("\ntotal part num: %d\n", j);
    g_part_num = j;
    for(i = 0; i < g_part_num; i++) {
        printf("part i: %d, %s, size: 0x%08lx, erasesize: 0x%08lx, part name: %s, \tfile: %s\n", \
                i, mtdPartInfo[i].mtd, mtdPartInfo[i].size, mtdPartInfo[i].erasesize, mtdPartInfo[i].part_name, mtdPartInfo[i].part_file);
    }

    fclose(fp);
    
    return true;
}

bool checkPartition()
{
    printf("%s, enter\n", __FUNCTION__);

    if(parsePartTable() == false) {
        printf("error, cannot get part table, return");
        return false;
    }

    return true;
}

int getVerMd5() 
{
    char file[MAX_PATH];
    char buff[MAX_PATH];
    char buffBak[MAX_PATH];
    FILE *fp = NULL;
    char *tmp = NULL;
    char str[16];
    int len = 0;
    int i = 0, j = 0;
    
    memset(buff, 0, MAX_PATH);
    memset(str, 0, 16);
    // get system information
    sprintf(file, "%s/%s", "/system", VERSION_FILE);
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

    return 0;
}

int parseShErr() {

    char file[MAX_PATH];
    char buff[MAX_PATH];
    FILE *fp = NULL;
    char str[16];
    int len = 0;
    
    memset(buff, 0, MAX_PATH);
    memset(str, 0, 16);
    // get err file
    sprintf(file, "%s/%s", g_upg_path, "err");
    fp = fopen(file, "r");
    if(fp == NULL) {
        printf("open %s error\n", file);
        return 0;
    }

    while(!feof(fp)) {

        fgets(buff, 255, fp);
        len = strlen(buff);
        buff[len-1] = '\0';

        if(0 == strcmp(buff, "No such file or directory")) {
            printf("error: %s\n", buff);
            return -1;
        }
    }

    return 0;
}

bool upgradeIsAvailable() 
{
    int cnt = 0;

    // get system and image version info
    if(getVerMd5() < 0) {
        printf("cannot get version info, return\n");
        return false;
    }

    if(strcmp(version_img, version_sys) <= 0) {
        printf("Img version (%s) <= system version (%s), no need upgrade\n", version_img, version_sys);
        return false;
    }

    char cmd[64];
    char res[MAX_PART_NUM][MAX_PATH];

    sprintf(cmd, "%s %s && %s 2> err", "cd", g_upg_path, "md5sum -c md5");
    
    cnt = shexec(cmd, res, MAX_PART_NUM);
    int i = 0;
    printf("cnt: %d, res0: %s\n", cnt, res[0]);
    for(i = 0; i < cnt; i++) {
        if(NULL != strstr(res[i], "FAILED")) {
            for(int j = 0; j < MAX_PART_NUM; j++) {
                if(mtdPartInfo[j].need_upgrade && (NULL != strstr(res[i], mtdPartInfo[j].part_file))) {
                    printf("cannot update part: %s, md5 not match\n", mtdPartInfo[j].part_file);
                    return false;
                }
            }
        }
    }

    return true;
}

int non_region_erase(int Fd, int start, int count, int unlock) 
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

        printf("\n-----------------Erase Done----------------\n");
    }

    return 0;
}

bool mtdUpgradePart(int index, const char *mtd) 
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

    memset(mtdDev, 0, 16);

    printf("%s, part%d: %s\n", __FUNCTION__, index, mtd);

    sprintf(mtdDev, "/dev/%s", mtd);

    memset(&info, 0, sizeof(struct mtd_info_user));
    if((fd = open(mtdDev, O_RDWR)) < 0) {
        printf("%s open error\n", mtdDev);
        reto = false;
        goto END;
    } else {
        printf("%s open, fd: %d, error: %d\n",mtdDev, fd, errno);
        ret = ioctl(fd, MEMGETINFO, &info);
        printf("ret: %d, error: %d\n", ret, errno);
        printf("size: 0x%08x, erasesize: 0x%08x, writesize: 0x%08x, oobsize: 0x%08x\n", \
                info.size, info.erasesize, info.writesize, info.oobsize);
    }

    if(ioctl(fd, MEMGETREGIONCOUNT, &regcount) == 0) {
        printf("regcount: %d\n", regcount);
    }

    readlen = info.erasesize;
    write_buf = (char*)malloc(readlen);
    if(write_buf == NULL) {
        printf("error, cannot malloc for write buf");
        reto = false;
        goto END;
    }

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

    sprintf(file_path, "%s/%s", g_upg_path, mtdPartInfo[index].part_file);
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

    ret = read(ifd, write_buf, readlen);
    if(ret != readlen) {
        printf("read error, ret: %d, readlen: %d\n", ret, readlen);
    }

    //hexdump(write_buf, dump_len);

// erase part
    if(regcount == 0) {
        res = non_region_erase(fd, 0, (info.size/info.erasesize), 0);
        printf("erase, res: %d\n", res);
    }

// write part
    printf("part size: 0x%08x\n", info.size);
    imglen1 = imglen;
    upgyet_size = 0;
    lseek(ifd, 0, SEEK_SET);
    lseek(fd, 0, SEEK_SET);
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
            progressl = upgyet_size*100/upgPartInfo[index].write_size;
            progresst = g_upgyet_size*100/g_upgrade_size;
            //printf("upgyet_size: %lu, info.size: %lu, g_upgyet_size: %lu, g_upgrade_size: %lu\n", \
            //        upgyet_size, info.size, g_upgyet_size, g_upgrade_size);
            printf("write %s ok！addr: 0x%08x, readlen: %d, write cnt: %d, progress: %d%%/%d%%\n", mtdDev, (imglen1 - imglen), readlen, write_cnt, progressl, progresst);
            //printf("*");
        }
        fflush(stdout);
        fsync(fd);
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

    printf("write done\n");

END:
    fflush(stdout);
    fsync(fd);
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

bool checkUsbPlugin()
{

    return true;
}

int main(int argc, char * argv[])
{   
    bool force = false;
    bool one_part = false;
    bool no_part_upgrade = true;
    int arg_index = 0;
    int i = 0;

    for(i = 1; i < argc; i++) {
        if(0 == strcmp("-m", argv[i])) { // more part upgrade
            printf("\nneed to upgrade userdata and uboot part\n");
            upg_more = true;
            printf("----More----\n");
        } else if(0 == strcmp("-f", argv[i])) {
            force = true;
            printf("\nForce upgrade, no need to compare version and md5\n");
        } else if(0 == strcmp("-p", argv[i])) {
            one_part = true;
            arg_index = i+1;
            printf("\nonly upgrade one part: %s\n", argv[arg_index]);
        }else if(0 == strcmp("--version", argv[i])) {
            printf("\nVersion: %s\n", VERSION_NO);
            return 0;
        }
    }
    printf("\n-------------Wifi NVR USB Upgrade Start---------------\n\n");
    char file_path[MAX_PATH];
    struct stat stbuf;

    partInfoInit();

    if(checkFile() == false) {
        printf("error, file is not exist, exit!\n");
        return -1;
    }

    checkPartition();
  
    if( (force == false) && (upgradeIsAvailable() == false)) {
        printf("cannot upgrade, return\n");
        return -2;
    }
    
    printf("------------------start to upgrade---------------\n");

    if(argc > 1) {
        //just for test to check some parse function
        if(0 == strcmp("step1", argv[1])) {
            printf("just test 1st step, not erase and write data\n");
            return 0;
        }
    }

    memset(upgPartInfo, 0, sizeof(struct upg_part_info)*MAX_PART_NUM);
    for(i = 0; i < g_part_num; i++) {
        strcpy(upgPartInfo[i].mtd, mtdPartInfo[i].mtd);

        if(one_part == true) {
            if((0 == strcmp(mtdPartInfo[i].mtd, argv[arg_index])) || (0 == strcmp(mtdPartInfo[i].part_name, argv[arg_index]))) {
                mtdPartInfo[i].need_upgrade = true;
            } else {
                mtdPartInfo[i].need_upgrade = false;
            }
        }

        if(mtdPartInfo[i].need_upgrade == true) {
            sprintf(file_path, "%s/%s", g_upg_path, mtdPartInfo[i].part_file);
            if(stat(file_path, &stbuf) == -1) {
                printf("cannot find part%d file: %s\n", i, mtdPartInfo[i].part_file);
                return -4;
            } else {
                upgPartInfo[i].file_size = stbuf.st_size;
                upgPartInfo[i].write_size = ALIGN(upgPartInfo[i].file_size, mtdPartInfo[i].erasesize);
                g_upgrade_size += upgPartInfo[i].write_size;
            }
        }

        printf("upgPartInfo, i: %d, filesize: 0x%08lx, writesize: 0x%08lx, g_upgrade_size: 0x%08lx, file: %s\n", \
                i, upgPartInfo[i].file_size, upgPartInfo[i].write_size, g_upgrade_size, mtdPartInfo[i].part_file);
    }

    for(i = 0; i < g_part_num; i++) {
        if(mtdPartInfo[i].need_upgrade == true) {
            no_part_upgrade = false;
            printf("part%d: %s start to upgrade!", i, mtdPartInfo[i].part_name);
            if(false == mtdUpgradePart(i, mtdPartInfo[i].mtd)) {
                printf("upgrade %s part fail, exit\n", mtdPartInfo[i].part_name);
                return -3;
            } 
        }
    }

    if(no_part_upgrade == false) {
        printf("**********Upgrade success!************\n");
        printf("reboot!\n");
        system("reboot");
    } else {
        printf("No Need to Upgrade Any Partition!\n");
    }

    return 0;
}

