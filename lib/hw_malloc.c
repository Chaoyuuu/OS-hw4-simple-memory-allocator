#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include "hw_malloc.h"

#define mmap_threshold 32768
typedef unsigned long long ull;

void * program_end;
void * program_ptr;

struct s_chunk_info {
    unsigned current_size : 31;
    unsigned allco_flag : 1;
    unsigned prev_size : 31;
    unsigned mmap_flag : 1;
};

struct s_chunk {
    struct s_chunk_info chunk_info;
    struct s_chunk * chunk_prev;
    struct s_chunk * chunk_next;
};

struct s_chunk* bin[15];
struct s_chunk* p_bin[15][20];
struct s_chunk* mmap_list;

void *get_start_sbrk(void)
{
    return sbrk(64*1024);
}

void init()
{

    // printf("in init_malloc\n");
    program_ptr = get_start_sbrk();
    // printf("0x000000000000\n");
    // printf("program_ptr = %p, %p\n", program_ptr, (struct s_chunk*)((ull)program_ptr-(ull)program_ptr));
    program_end = sbrk(0);

    //create bin
    for(int i=0; i<=10; i++) {
        bin[i] = malloc(sizeof(struct s_chunk));
        bin[i]->chunk_prev = bin[i];
        bin[i]->chunk_next = bin[i];

        // print_bin[i] = malloc(sizeof(struct s_chunk));
        // print_bin[i]->chunk_prev = print_bin[i];
        // print_bin[i]->chunk_next = print_bin[i];
    }

    //create mmap_list
    mmap_list = malloc(sizeof(struct s_chunk));
    mmap_list->chunk_prev = mmap_list;
    mmap_list->chunk_next = mmap_list;
    mmap_list->chunk_info.current_size = 10;

    // split 64K->32k
    // printf("size = %ld\n", sizeof(tmp_c));
    // printf("size = %ld\n", sizeof(tmp3));

    struct s_chunk * tmp = program_ptr;  //0x00
    bin[10]->chunk_next = tmp;
    bin[10]->chunk_prev = tmp;

    tmp->chunk_prev = bin[10];
    tmp->chunk_next = bin[10];
    tmp->chunk_info.current_size = 32*1024;
    tmp->chunk_info.allco_flag = 0;
    tmp->chunk_info.prev_size = 0;///////The Last One (OK)
    tmp->chunk_info.mmap_flag = 0;

    struct s_chunk * tmp1 = program_ptr + 0x8000; //+32K
    tmp1->chunk_prev = tmp;
    tmp1->chunk_next = bin[10];
    tmp1->chunk_info.current_size = 32*1024;
    tmp1->chunk_info.allco_flag = 0;
    tmp1->chunk_info.prev_size = tmp->chunk_info.current_size;
    tmp1->chunk_info.mmap_flag = 0;

    tmp1->chunk_prev->chunk_next = tmp1;
    bin[10]->chunk_prev = tmp1;

    //put in print_bin
    p_bin[10][0] = tmp;
    p_bin[10][1] = tmp1;

    // printf("tmp = %d\n", p_bin[10][0]->chunk_info.current_size);
    // printf("tmp1 = %d\n", p_bin[10][1]->chunk_info.current_size);
    /*
    for(int i=10; i>=0; i--) {
        printf("bin[i] = %p\n", bin[i]);
    }
    */
    return;
}
void *hw_malloc(size_t bytes)
{
    // printf("in hw_malloc\n");

    bytes = bytes + 24;//data + header
    if(bytes >= mmap_threshold) {
        //use mmap(), put in mmap_alloc_list
        return malloc_mmap(bytes);

    } else {           //bin[x], spilt mem
        //find bytes_size
        size_t size_tmp = 32;
        int i;
        for(i=0; i<=10; i++) {
            if(bytes <= size_tmp) {  //if bin[i] == NULL
                // printf("bin num = %ld\n", size_tmp);
                break;
            }
            size_tmp = size_tmp*2;
        }

        //do bin
        if(bin[i]->chunk_next == bin[i]) {
            // printf("split the bin\n");
            int flag = 0;
            int s = i;
            while(s<=10) {
                if(bin[s]->chunk_next != bin[s]) {
                    // printf("s= %d, i=%d\n", s, i);
                    split(s, i);
                    flag = 1;
                    break;
                }
                s++;
            }

            if(flag == 0) {
                printf("no memory\n");
                return NULL;
            } else {
                void* ans_tmp = malloc_bin(i);
                return (struct s_chunk*)((ull)ans_tmp - (ull)program_ptr + (ull)0x18);
            }

        } else {
            // printf("malloc mem\n");
            void* ans_tmp = malloc_bin(i);
            return (struct s_chunk*)((ull)ans_tmp - (ull)program_ptr + (ull)0x18);
        }
    }
    return NULL;
}

void split(int s, int i)
{
    // printf("in split\n");
    //spilt bin[s], until s==i
    struct s_chunk * ptr = bin[s]->chunk_next;
    size_t tmp_size = ptr->chunk_info.prev_size;

    //delete ptr in bin[s]
    ptr->chunk_prev->chunk_next = ptr->chunk_next; //bin[s]->chunk_next = ptr->chunk_next;
    ptr->chunk_next->chunk_prev = ptr->chunk_prev; //ptr->chunk_next->chunk_prev= bin[s];

    //spilt in 2 chunk
    bin[s-1]->chunk_prev = ptr;
    bin[s-1]->chunk_next = ptr;

    ptr->chunk_prev = bin[s-1];
    ptr->chunk_next = bin[s-1];
    ptr->chunk_info.prev_size = tmp_size;

    // printf("size = %lld\n", ptr->chunk_info.current_size);
    // printf("size/2 = %lld\n", (ptr->chunk_info.current_size)/2);

    ptr->chunk_info.current_size = (ptr->chunk_info.current_size)/2;
    ptr->chunk_info.allco_flag = 0;//////already have??????
    ptr->chunk_info.mmap_flag = 0;//////already have??????

    struct s_chunk * ptr2 = (struct s_chunk*)((ull)ptr + (ull)ptr->chunk_info.current_size); ///////////

    ptr->chunk_next = ptr2;
    bin[s-1]->chunk_prev = ptr2;

    ptr2->chunk_prev = ptr;
    ptr2->chunk_next = bin[s-1];
    ptr2->chunk_info.prev_size = ptr->chunk_info.current_size;
    ptr2->chunk_info.current_size = (ptr->chunk_info.current_size);
    ptr2->chunk_info.allco_flag = 0;//////already have??????
    ptr2->chunk_info.mmap_flag = 0;//////already have??????

    //compare address, and order(don't need)
    /*
    for(int k=10; k>=0; k--){
    	if(bin[k]->chunk_prev == bin[k])
    		printf("bin[%d] is empty\n", k);
    }
    printf("bin[%d]\n", s-1);
    printf("size = %d\n", ptr->chunk_info.current_size);
    printf("ptr  = %p\n", (struct s_chunk*)((ull)ptr - (ull)program_ptr));
    printf("ptr2 = %p\n", (struct s_chunk*)((ull)ptr2 - (ull)program_ptr));
    */

    if(s-1 != i) {
        // printf("split again\n");
        // printf("--------------\n");
        split(s-1, i);
    }

    return;
}

void* malloc_bin(int i)
{
    // printf("in malloc_bin\n");
    //find lowest address
    struct s_chunk* p = bin[i]->chunk_next;
    //delete p in bin[i] and change chunk_info
    bin[i]->chunk_next = p->chunk_next;
    p->chunk_next->chunk_prev = p->chunk_prev;
    p->chunk_next = NULL;
    p->chunk_info.allco_flag = 1;
    p->chunk_info.mmap_flag = 0;

    // printf("malloc_bin = %p\n", (struct s_chunk*)((ull)p - (ull)program_ptr));
    return p;
}

int hw_free(unsigned long mem)
{
    // printf("in free mem\n");
    // printf("mem = %lx\n", mem);

    //free mmap
    if((ull)mem >= (ull)program_end) {
        // printf("free mmap_list\n");
        struct s_chunk* p_mmap = (struct s_chunk*)(ull)mem;
        p_mmap->chunk_prev->chunk_next = p_mmap->chunk_next;
        p_mmap->chunk_next->chunk_prev = p_mmap->chunk_prev;

        if(munmap(p_mmap, p_mmap->chunk_info.current_size) == 0)
            return 1;
        else
            return 0;
    }

    //free bin
    struct s_chunk* p = (struct s_chunk*)((ull)mem + (ull)program_ptr - (ull)0x18);

    //find the bin[x] and change chunk info
    // printf("size to free = %d\n", p->chunk_info.current_size);
    size_t free_size = p->chunk_info.current_size;
    size_t tmp = 32;
    int i = 0;
    while(tmp != free_size) {
        // printf("+\n");
        tmp = tmp*2;
        i++;
    }
    // printf("tmp = %d, i = %d\n", tmp, i);
    struct s_chunk * bin_ptr = bin[i];
    p->chunk_info.allco_flag = 0;

    //put in bin[x] with address order
    struct s_chunk * p_tmp = bin_ptr->chunk_next;
    if(p_tmp == bin_ptr) {  //bin is empty
        // printf("bin is empty\n");
        p->chunk_prev = bin_ptr;
        p->chunk_next = bin_ptr;
        bin_ptr->chunk_prev = p;
        bin_ptr->chunk_next = p;
    } else if((ull)bin_ptr->chunk_prev < (ull)p) {
        // printf("in free the biggest\n");
        p->chunk_prev = bin_ptr->chunk_prev;
        p->chunk_next = bin_ptr;
        bin_ptr->chunk_prev->chunk_next = p;
        bin_ptr->chunk_prev = p;
    } else {
        while(p_tmp != bin_ptr) {       //bin is not empty
            if((ull)p_tmp >= (ull)p) {	//insert p above p_tmp
                // printf("in insert\n");
                p->chunk_next = p_tmp;
                p->chunk_prev = p_tmp->chunk_prev;
                p_tmp->chunk_prev->chunk_next = p;
                p_tmp->chunk_prev = p;
                break;
            }
            p_tmp = p_tmp->chunk_next;
        }
    }

    merge_bin(i);
    return 1;   //if no this mem?????????????
}

void merge_bin(int i)
{
    // printf("in merge_bin\n");
    struct s_chunk * ptr = bin[i]->chunk_next;
    while(ptr != bin[i] && ptr->chunk_next != bin[i] && i!= 10) {
        if((ull)ptr + (ull)ptr->chunk_info.current_size == (ull)ptr->chunk_next) { //merge
            //delete ptr & ptr->chunk_next
            ptr->chunk_prev->chunk_next = ptr->chunk_next->chunk_next;
            ptr->chunk_next->chunk_next->chunk_prev = ptr->chunk_prev;
            ptr->chunk_info.current_size = (ptr->chunk_info.current_size)*2;

            //put in bin[i+1]
            struct s_chunk * tmp = bin[i+1]->chunk_next;

            if(tmp == bin[i+1]) {  //bin is empty
                // printf("bin is empty in next bin\n");
                ptr->chunk_prev = bin[i+1];
                ptr->chunk_next = bin[i+1];
                bin[i+1]->chunk_prev = ptr;
                bin[i+1]->chunk_next = ptr;
            } else if(bin[i+1]->chunk_prev < ptr) { //address bigger than last one
                // printf("the biggest\n");
                ptr->chunk_prev = bin[i+1]->chunk_prev;
                ptr->chunk_next = bin[i+1];
                bin[i+1]->chunk_prev->chunk_next = ptr;
                bin[i+1]->chunk_prev = ptr;
            } else {
                while(tmp != bin[i+1]) {  //bin is not empty
                    if((ull)tmp >= (ull)ptr) {
                        //insert p above p_tmp
                        // printf("in insert in next bin\n");
                        ptr->chunk_next = tmp;
                        ptr->chunk_prev = tmp->chunk_prev;
                        tmp->chunk_prev->chunk_next = ptr;
                        tmp->chunk_prev = ptr;
                        break;
                    }
                    tmp = tmp->chunk_next;
                }
            }
            merge_bin(i+1);
            break;
        }
        ptr = ptr->chunk_next;
        // printf("%p\n", ptr);
    }

    return;
}

void *malloc_mmap(size_t bytes)
{
    // printf("in malloc_mmap\n");
    /* void *mmap(void *start, size_t length, int prot, int flags, int fd, off_t offsize);*/
    struct s_chunk* start =(struct s_chunk*)mmap(NULL, bytes, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
    if(start == MAP_FAILED) { //if failed mmap()
        return NULL;
    }

    //change chunk info
    start->chunk_info.current_size = bytes;
    start->chunk_info.prev_size = 0;
    start->chunk_info.allco_flag = 0;
    start->chunk_info.mmap_flag = 1;

    if(mmap_list->chunk_next == mmap_list) { //mmap_list is empty
        // printf("mmap_list is empty\n");
        start->chunk_prev = mmap_list;
        start->chunk_next = mmap_list;
        mmap_list->chunk_prev = start;
        mmap_list->chunk_next = start;
    } else if(mmap_list->chunk_prev->chunk_info.current_size <= start->chunk_info.current_size) { //the biggest size
        //add in bottom
        start->chunk_next = mmap_list;
        start->chunk_prev = mmap_list->chunk_prev;
        mmap_list->chunk_prev->chunk_next = start;
        mmap_list->chunk_prev = start;

    } else { //order the mmap size
        struct s_chunk * p = mmap_list->chunk_next;
        while(p != mmap_list) {
            if(p->chunk_info.current_size > start->chunk_info.current_size) {
                //add in mmap_list
                // printf("mmap_list is not empty\n");
                start->chunk_prev = p->chunk_prev;
                start->chunk_next = p;
                p->chunk_prev->chunk_next = start;
                p->chunk_prev = start;
                break;
            }
            p = p->chunk_next;
        }
    }

    // printf("success mmap\n");
    // printf("start = %p\n", start);
    return start;
}

void print_bin(int i)
{
    // printf("in print_bin\n");
    // printf("bin[%d]\n", i);

    /* for(int i=10; i>=0; i--){ */

    struct s_chunk* p = bin[i]->chunk_next;

    if(p == bin[i]) {
        printf("bin is empty\n");
    } else {
        // printf("somethin in bin\n");
        while(p != bin[i]) {
            if(((ull)p - (ull)program_ptr) == 0) {
                printf("0x000000000000--------%d\n", p->chunk_info.current_size);
            } else {
                printf("%014p--------%d\n", (struct s_chunk*)((ull)p - (ull)program_ptr), p->chunk_info.current_size);
            }
            p = p->chunk_next;
        }
    }

    /* } */
    return;
}

void print_mmap()
{
    // printf("in print_mmap\n");
    struct s_chunk* p = mmap_list->chunk_next;
    while(p != mmap_list) {
        printf("%014p--------%d\n", p, p->chunk_info.current_size);
        p = p->chunk_next;
    }
    return;
}