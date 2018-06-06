#include "utils.h"

/*
 *  * print sizes as "xxx KiB", "xxx.y KiB", "xxx MiB", "xxx.y MiB",
 *   * xxx GiB, xxx.y GiB, etc as needed; allow for optional trailing string
 *    * (like "\n")
 *     */
void print_size(unsigned long long size, const char *s)
{
    unsigned long m = 0, n;
    unsigned long long f;
    static const char names[] = {'E', 'P', 'T', 'G', 'M', 'K'};
    unsigned long d = 10 * ARRAY_SIZE(names);
    char c = 0;
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(names); i++, d -= 10) {
        if (size >> d) {
            c = names[i];
            break;
                                             
        }                                
    }

    if (!c) {
        printf("%llu Bytes%s", size, s);
        return;
                                                    
    }

    n = size >> d;
    f = size & ((1ULL << d) - 1);

   /* If there's a remainder, deal with it */
    if (f) {
        m = (10ULL * f + (1ULL << (d - 1))) >> d;

        if (m >= 10) {
            m -= 10;
            n += 1;
                                                                        
        }
                                                    
    }

    printf ("%lu", n);
    if (m) {
        printf (".%ld", m);
                                                        
    }
    
    printf (" %ciB%s", c, s);

}


void removeChar(char *str, char c) 
{
    char *s = str;
    int j, k;

    for(j=k=0; s[j]!='\0'; j++) {
        if(s[j]!=c)
            s[k++]=s[j];
    }

    s[k]= '\0';
    //printf("\n%s\n",s);
}

unsigned int StrToHex(char *pszSrc, int nLen)  
{  
    char h1, h2;  
    char s1, s2; 
    unsigned char pbDest;
    unsigned int ret = 0;

    //printf("%s, src: %s\n", __FUNCTION__, pszSrc);

    for (int i = 0; i < nLen; i++)  
    {  
            h1 = pszSrc[2 * i];  
            h2 = pszSrc[2 * i + 1];  
      
            s1 = toupper(h1) - 0x30;  
            if (s1 > 9)  
                s1 -= 7;  
      
            s2 = toupper(h2) - 0x30;  
            if (s2 > 9)  
                s2 -= 7;  
            pbDest = s1 * 16 + s2;  
            //printf("h1: 0x%02x, h2: 0x%02x, s1: 0x%02x, s2: 0x%02x, dst: 0x%x\n", h1, h2, s1, s2, pbDest);  

            ret = (ret << 8) + pbDest;
            //printf("ret: 0x%x\n", ret);
    }  

    return ret;
}  

void hexdump(const char *buf, int len) 
{
    int i = 0; 

    printf("\n----------------------hexdump------------------------\n");
    for(i = 0; i < len; i++) {
        printf("%02x ", buf[i]);
        if( (i+1) % 16 == 0) {
            printf("\n");
        }
    }

    if(i%16 != 0) {
        printf("\n");
    }

    printf("---------------------hexdump-------------------------\n\n");
}

//execute shell command
int shexec(const char *cmd, char res[][512], int count)
{
    printf("shexec, cmd: %s\n", cmd);
    
    FILE *pp = popen(cmd, "r");
    if(!pp) {
        printf("error, cannot popen cmd: %s\n", cmd);
        return -1;
    }
    int i = 0;
    char tmp[512];
    while(fgets(tmp, sizeof(tmp), pp) != NULL) {
        if(tmp[strlen(tmp)-1] == '\n') {
            tmp[strlen(tmp)-1] = '\0';
        }
        printf("%d.get return results: %s\n", i, tmp);
        strcpy(res[i], tmp);
        i++;
        if(i >= count) {
            printf("get enough results, return\n");
            break;
        }
    }

    pclose(pp);

    return i;
}


