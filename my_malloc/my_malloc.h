#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <limits.h>
#include <pthread.h>

struct m_block{
    int flag;  //if flag=1, √ use; if flag=0, × use
    //using linked list with two pointers-> helpful for deleting and merge after free
    struct m_block* next;
    struct m_block* prev;
    size_t size;
};

typedef struct m_block*  block;

block head = NULL;
block free_head_lock = NULL;
block tail_lock = NULL;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
__thread block free_head_unlock = NULL;
__thread block tail_unlock = NULL;

void split_blk(block curr_blk,size_t size, block *free_head, block *tail);
void remove_front(block curr_blk,size_t size, block *free_head, block *tail);
void remove_last(block curr_blk, size_t size, block *free_head, block *tail);
void remove_blk(block curr_blk, size_t size, block *free_head, block *tail);
void prev_merge(block curr_blk, block *tail);
void next_merge(block curr_blk, block *tail);
void add_blk(block curr_blk, block *free_head, block *tail);
block ff_find(size_t len);
void judge_remove(block curr_blk, size_t len, block *free_head, block *tail);
void ff_free(void *ptr, block *free_head, block *tail);
block ff_extend(size_t len, int sbrk_lock);
void *ff_malloc(size_t size);
void ff_free(void *ptr, block *free_head, block *tail);
void *bf_malloc(size_t size, block *free_head, block *tail, int sbrk_lock);
void bf_free(void *ptr, block *free_head, block *tail);
block bf_find(size_t size, block *free_head);
void * ts_malloc_lock(size_t size);
void ts_free_lock(void * ptr);
void * ts_malloc_nolock(size_t size);
void ts_free_nolock(void * ptr);

