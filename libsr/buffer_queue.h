/**
 * @file buffer_queue.h
 * @author yangwei-ewe (jaychiang0127@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-05-27
 *
 * @copyright @楊偉 Copyright 2024
 */

#ifndef __BUFFERQUEUE_H_
#define __BUFFERQUEUE_H_
#include <stdlib.h>
#include <string.h>
#include "defines.h"
#include <pthread.h>
#include "linkedList.h"


typedef struct buf_node buf_node;
typedef struct sr_pkg sr_pkg;
typedef struct sr_obj sr_obj;

struct buf_node {
    sr_pkg* pkg;
    buf_node* next;
};

typedef struct buffer {
    pthread_mutex_t buffer_lock;
    ll_t* wnd;
    ll_t* buf;
    sr_obj* obj;
    size_t curr_wnd_len;
    size_t curr_buf_len;
    size_t def_wnd_len;
    size_t def_buf_len; // typically is 2 times of def_wnd_len
    int is_EOF; // 0 stands for EOF
} buffer_t;

void buffer_init(buffer_t** buffer, sr_obj* obj);
void free_buffer(buffer_t** buffer);
/**
 * @brief
 *
 * @param buffer pointer to buffer
 * @return current base seqence number
 */
int buffer_update(buffer_t* buffer);

/**
 * @brief mark pkg to acked, for sender use
 *
 * @param buffer
 * @param seq_num
 */
void wnd_ack(buffer_t* buffer, size_t seq_num);

/**
 * @brief peek the head of buffer(wnd)
 *
 * @param buffer buffer
 * @return sr_header* the header of the head pkg in buffer
 */
sr_header* buffer_peek(buffer_t* buffer);

#endif