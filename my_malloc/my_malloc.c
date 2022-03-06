
#include "my_malloc.h"

void * ts_malloc_lock(size_t size) {
  pthread_mutex_lock(&lock);
  void * p = bf_malloc(size, &free_head_lock, &tail_lock, 0);
  pthread_mutex_unlock(&lock);
  return p;
}

void ts_free_lock(void * ptr) {
  pthread_mutex_lock(&lock);
  bf_free(ptr, &free_head_lock, &tail_lock);
  pthread_mutex_unlock(&lock);
}

void * ts_malloc_nolock(size_t size) {
  void * p = bf_malloc(size, &free_head_unlock, &tail_unlock, 1);
  return p;
}

void ts_free_nolock(void * ptr) {
  bf_free(ptr, &free_head_unlock, &tail_unlock);
}

void split_blk(block curr_blk,size_t size, block *free_head, block *tail){
    block new_blk = NULL;
    new_blk = (void *)curr_blk + sizeof(struct m_block) + size;
    new_blk ->prev = curr_blk -> prev;
    new_blk ->next = curr_blk -> next;
    new_blk ->size = curr_blk->size - sizeof(struct m_block)-size;
    if(curr_blk->next != NULL){
        curr_blk->next->prev = new_blk;
    }
    else{
        *tail = new_blk;
    }
    if(curr_blk->prev !=NULL){
        curr_blk->prev->next = new_blk;
    }
    else{
        *free_head = new_blk;
    }
    curr_blk->size = size;
}

void remove_front(block curr_blk,size_t size, block *free_head, block *tail){
    if((int)curr_blk->size-(int)size-(int)sizeof(struct m_block)>0){
        split_blk(curr_blk, size, free_head, tail);
    }
    else{
        *free_head = curr_blk->next;
        if(curr_blk->next!=NULL){
            curr_blk->next->prev=NULL;
        }
        else{
            *tail = NULL;
        }
    }
    curr_blk->next = NULL;
    curr_blk->prev = NULL;
}

void remove_last(block curr_blk, size_t size, block *free_head, block *tail){
    if((int)curr_blk->size-(int)size-(int)sizeof(struct m_block)>0){
        split_blk(curr_blk, size, free_head, tail);
    }
    else{
        *tail = curr_blk->prev;
        if(curr_blk->prev!=NULL){
            curr_blk->prev->next=NULL;
        }
        else{
            *free_head = NULL;
        }
    }
    curr_blk->next = NULL;
    curr_blk->prev = NULL;
}

void remove_blk(block curr_blk, size_t size, block *free_head, block *tail){
    if((int)curr_blk->size-(int)size-(int)sizeof(struct m_block)>0){
        split_blk(curr_blk, size, free_head, tail);
    }
    else{
        curr_blk->next->prev = curr_blk->prev;
        curr_blk->prev->next = curr_blk->next;
    }
    curr_blk->next = NULL;
    curr_blk->prev = NULL;
}

void judge_remove(block curr_blk, size_t len, block *free_head, block *tail){
    if(curr_blk->size>=len){
        //remove the curr_blk from the free_list
        if(curr_blk->prev == NULL){
            remove_front(curr_blk, len, free_head, tail);
        }
        else if(curr_blk->next == NULL){
            remove_last(curr_blk, len, free_head, tail);
        }
        else{
            remove_blk(curr_blk, len, free_head, tail);
        }
    }
}

block ff_extend(size_t len, int sbrk_lock){
    block curr_meta = NULL;
    //allocate required memory
    //curr_meta = sbrk(len + sizeof(struct m_block));
    //fail to allocate memory
    if (sbrk_lock == 0) {
        curr_meta = sbrk(len + sizeof(struct m_block));
    }

    else {
        pthread_mutex_lock(&lock);
        curr_meta = sbrk(len + sizeof(struct m_block));
        pthread_mutex_unlock(&lock);
    }

    if(curr_meta==(void *)-1){
        return NULL;
    }

    else{
        curr_meta->prev = NULL;
        curr_meta->next = NULL;
        curr_meta->flag = 0;
        curr_meta->size = len;
    }
    return curr_meta;
}

void prev_merge(block curr_blk, block *tail){
    curr_blk->prev->size +=sizeof(struct m_block)+curr_blk->size;
    curr_blk->prev->next = curr_blk->next;
    if(curr_blk->next!=NULL){
        curr_blk->next->prev=curr_blk->prev;
    }
    else{
        *tail = curr_blk->prev;
    }
    curr_blk->prev = NULL;
    curr_blk->next = NULL;
}


void next_merge(block curr_blk, block *tail){
    curr_blk->size+=sizeof(struct m_block)+curr_blk->next->size;
    if(curr_blk->next->next!=NULL){
        block temp = curr_blk->next;
        curr_blk->next = curr_blk->next->next;
        curr_blk->next->prev = curr_blk;
        temp->next = NULL;
        temp->prev = NULL;
    }
    else{
        *tail = curr_blk;
        curr_blk->next->prev = NULL;
        curr_blk->next = NULL;
    }
}

void add_blk(block curr_blk, block *free_head, block *tail){
    block f_curr_blk = *free_head;
    //if current free list is NULL
    if(*free_head == NULL){
        *free_head = (void *)curr_blk;
        *tail = *free_head;
        return;
    }
    // add the block in the front of the free list
    if((void *)(*free_head) >= (void *)curr_blk+sizeof(struct m_block)+curr_blk->size){
        (*free_head)->prev = curr_blk;
        curr_blk->next = *free_head;
        // judge merge or not
        if((void *)curr_blk+sizeof(struct m_block)+curr_blk->size==(void *)(*free_head)){
            next_merge(curr_blk, tail);
        }
        *free_head=curr_blk;
        return;
    }
    //add the block in the middle of the free list
    while(f_curr_blk->next!=NULL){
        if((void *)curr_blk>=(void *)f_curr_blk+sizeof(struct m_block)+f_curr_blk->size && (void *)curr_blk+sizeof(struct m_block)+curr_blk->size <= (void *)f_curr_blk->next){
            curr_blk->prev = f_curr_blk;
            curr_blk->next = f_curr_blk->next;
            f_curr_blk->next = curr_blk;
            curr_blk->next->prev = curr_blk;
            //judge prev_merge or not
            if((void *)curr_blk->prev+sizeof(struct m_block)+curr_blk->prev->size==(void *)curr_blk){
                prev_merge(curr_blk, tail);
            }
            //judge next_merge of not
            if((void *)curr_blk+sizeof(struct m_block)+curr_blk->size==(void *)curr_blk->next){
                next_merge(curr_blk, tail);
            }
            return;
        }
        f_curr_blk = f_curr_blk->next;
    }

    //add the block at the tail of the free list
    if((void *)curr_blk >= (void *)(*tail)+sizeof(struct m_block)+(*tail)->size){
    (*tail)->next =  curr_blk;
    curr_blk->prev = *tail;
        if((void *)(*tail)+sizeof(struct m_block)+(*tail)->size == (void *)curr_blk){
        //tail->size += sizeof(struct m_block)+curr_blk->size;
        prev_merge(curr_blk, tail);

        }
    else{
        //tail->next = curr_blk;
        //curr_blk->prev = tail;
        *tail = curr_blk;
    }
        return;
    }
}

void ff_free(void *ptr, block *free_head, block *tail){
    block curr_blk = NULL;
    curr_blk=ptr - sizeof(struct m_block);
    //call the add block function
    add_blk(curr_blk, free_head, tail);
}

block bf_find(size_t size, block *free_head){
    block curr = *free_head;
    int flag=0;
    block rig = NULL;
    size_t min=ULONG_MAX;
    while(curr!=NULL){
        if(curr->size==size) return curr;
        if((int)curr->size-(int)size>0){
            if(flag==0){
                rig=curr;
                flag=1;
                min=curr->size;
            }else{
                if(curr->size<min){
                    rig=curr;
                    min=curr->size;
                }
            }
        }
        curr = curr->next;
    }
    return rig;
}

void *bf_malloc(size_t size, block *free_head, block *tail, int sbrk_lock){
    if(size==0) return NULL;
    void * curr = NULL;
    void * last = NULL;
    void * extend_blk = NULL;
    void * first_blk = NULL;
    curr=sbrk(0);
    if(head==NULL){
        head=sbrk(0);
    }
    if(head!=curr){
        last = bf_find(size, free_head);
        if(last!=NULL){
            judge_remove(last, size, free_head, tail);
            return last+sizeof(struct m_block);
        }
        else{
            extend_blk = ff_extend(size, sbrk_lock);
            return extend_blk+sizeof(struct m_block);
        }

    }
    else{
        first_blk=ff_extend(size, sbrk_lock);
        return first_blk+sizeof(struct m_block);
    }
}


void bf_free(void *ptr, block *free_head, block *tail){
    return ff_free(ptr, free_head, tail);
}

