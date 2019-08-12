#include "lib/hw_malloc.h"
#include "hw4_mm_test.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[])
{
    // printf("start\n");
    init();

    char buf[50];
    char tmp[50];
    char num[10];
    char *delim = " ";
    char *delim_1 = "[";
    char *delim_2 = "]";
    void *ans;

    while(fgets(buf, 50, stdin)) {
        // fgets(buf, 50, stdin);
        // printf("input = %s\n", buf);
        strtok(buf, delim);
        strcpy(tmp, strtok(NULL, delim));
        // printf("%s\n", tmp);

        if(strncmp(buf, "alloc", 5) == 0) { //alloc
            ans = hw_malloc(atoi(tmp));
            if(ans == NULL)
                printf("ans is NULL\n");
            else
                printf("%014p\n", ans);

        } else if(strncmp(buf, "free", 4) == 0) { //free mem
            // printf("tmp mem = %s\n", tmp);
            // printf("hex mem = %lx\n", strtol(tmp, NULL, 16));
            int a = hw_free(strtol(tmp, NULL, 16));
            if(a == 1)
                printf("success\n");
            else
                printf("%d\n", a);

        } else if(strncmp(buf, "print", 5) == 0) {
            if(strncmp(tmp, "bin", 3) == 0) { //print bin
                strtok(tmp, delim_1);
                strcpy(tmp, strtok(NULL, delim_1));
                strcpy(num, strtok(tmp, delim_2));
                print_bin(atoi(num));
            } else { //print mmap
                print_mmap();
            }

        }
    }
    return 0;
}
