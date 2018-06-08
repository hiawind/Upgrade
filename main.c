#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "upgrade.h"

int main(int argc, char * argv[])
{   
    int ret = 0;
    
    if(argc > 1 && strcmp(argv[1], "--version") == 0) {
        printf("\nVersion: %s\n", VERSION_NO);
        return 0;
    }

    int c;
    bool force = false;
    bool one_part = false;
    bool upgrade_in_uboot = true;
    char part_name[16];
    char version_path[MAX_PATH];
    while(1)
    {
        c = getopt(argc, argv, "p:b:fau");
        if(c == EOF)
            break;

            switch(c)
            {
                case 'f':
                    force = true;
                    break;

                case 'u':
                    upgrade_in_uboot = true;
                    break;

                case 'a':
                    upgrade_in_uboot = false;
                    break;
                
                case 'p':
                    one_part = true;
                    strcpy(part_name, optarg);
                    break;

                case 'b':
                    strcpy(version_path, optarg);
                    break;

                default:
                    break;
            }
    }

    bool no_part_upgrade = true;
    int i = 0;

    printf("\n-------------Wifi NVR USB Upgrade Start---------------\n\n");

    part_info_init();

    if(check_file() == false) {
        printf("error, file is not exist, exit!\n");
        return -1;
    }

    if(mtd_parse_info() == false) {
        printf("cannot get part table info, return \n");
        return -1;
    }
  
    if( (force == false) && (check_ver_md5(version_path) == false)) {
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

    if(upgrade_in_uboot) {
        ret = upgrade_to_uboot_progress(one_part, part_name);
        if(ret > 0) {
            printf("reboot to uboot to upgrade!\n");
            system("reboot");
        } else if (ret == 0){
            printf("No Need to Upgrade Any Partition!\n");
        } else {
            printf("***********ERROR!***********\n");
        }
    } else {
        ret = upgrade_image_progress(one_part, part_name);
        
        if(ret > 0) {
            printf("**********Upgrade success!************\n");
            printf("reboot!\n");
            system("reboot");
        } else if (ret == 0){
            printf("No Need to Upgrade Any Partition!\n");
        } else {
            printf("***********ERROR!***********\n");
        }
    }
    return 0;
}

