#include "cachelab.h"
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<getopt.h>
#include<string.h>

int s, S, E, b;
char t[1000];

int hit_count, miss_count, eviction_count; // cache statistics
typedef struct {
    int valid_bit;
    int tag;
    int stamp;
} cache_line, *cache_set, **cache;

cache my_cache = NULL;


// init the cache simulator
void init_cache() {
    my_cache = (cache) malloc(sizeof(cache_set) * S);
    int i, j;
    for (i = 0; i < S; i++) {
        my_cache[i] = (cache_set) malloc(sizeof(cache_line) * E);
        for (j = 0; j < E; j++) {
            my_cache[i][j].valid_bit = 0;
            my_cache[i][j].tag = -1;
            my_cache[i][j].stamp = -1;
        }
    }
}

// process single operation on cache
void update_cache(unsigned addr) {
    int set_idx = (addr >> b) & ((-1U) >> (64 -s));
    int tag = addr >> (b + s);
    int i;
    // try to hit
    for (i = 0; i < E; i++) {
        if (my_cache[set_idx][i].tag == tag) { // hit
            my_cache[set_idx][i].stamp = 0; // reset LRU timestamp
            hit_count++;
            return;
        }
    }
    // miss
    // try to store in an empty line
    for (i = 0; i < E; i++) {
        if (my_cache[set_idx][i].valid_bit == 0) { // empty line
            my_cache[set_idx][i].valid_bit = 1;
            my_cache[set_idx][i].tag = tag;
            my_cache[set_idx][i].stamp = 0;
            miss_count++;
            return;
        }
    }

    // either hit or have empty line
    // need eviction
    int max_stamp = -1;
    int max_stamp_idx = -1;
    for (i = 0; i < E; i++) {
        if (my_cache[set_idx][i].stamp > max_stamp) {
            max_stamp = my_cache[set_idx][i].stamp;
            max_stamp_idx = i;
        }
    }
    my_cache[set_idx][max_stamp_idx].tag = tag;
    my_cache[set_idx][max_stamp_idx].stamp = 0;
    miss_count++;
    eviction_count++;
    return;

}

// increment the timestamp for record
void update_stamp() {
    int i, j;
    for (i = 0; i < S; i++) {
        for (j = 0; j < E; j++) {
            if (my_cache[i][j].valid_bit == 1) my_cache[i][j].stamp++;
        }  
    }
}


// process trace file
void process_trace() {
    FILE* fp = fopen(t, "r");
    if (fp == NULL) {
        printf("Error while opening the trace file...\n");
        exit(-1);
    }
    
    char operation; // I or L or M or S
    unsigned int addr; // address
    int size;

    // process the trace file line by line
    while (fscanf(fp, " %c %xu,%d\n", &operation, &addr, &size) > 0) {
        switch (operation) {
            case 'L':
                update_cache(addr);
                break;
            case 'M':
                update_cache(addr);
            case 'S':
                update_cache(addr);
        }
        update_stamp();
    }
}

// free space allocated by malloc
void free_space() {
    for (int i = 0; i < S; i++) {
        free(my_cache[i]);
    }
    free(my_cache);
}

int main(int argc, char* argv[])
{   
    // parse command line args
    int opt;
    while (-1 != (opt = getopt(argc, argv, "s:E:b:t:"))) {
        switch (opt) {
            case 's':
                s = atoi(optarg);
                break;
            case 'E':
                E = atoi(optarg);
                break;
            case 'b':
                b = atoi(optarg);
                break;
            case 't':
                strcpy(t, optarg);
                break;
            default:
                break;
        }
    }

    // check arguments
    if (s <= 0 || E <= 0 || b <= 0 || t == NULL) {
        printf("Invalid arguments...\n");
        return -1;
    }

    // number of cache sets is 2 ^ s
    S = 1 << s;
    init_cache();
    process_trace();
    printSummary(hit_count, miss_count, eviction_count);
    free_space();
    return 0;
}


