/**
 * @file linkedList.h
 * @author yangwei-ewe (jaychiang0127@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-04-29
 *
 * @copyright @楊偉 Copyright 2024
 */

#ifndef __LINKEDLIST_H_
#define __LINKEDLIST_H_
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "sr.h"

typedef struct ll_node ll_node;
typedef struct sr_pkg sr_pkg;

/**
 * @brief basic unit in ll
 */
struct ll_node {
    sr_pkg* pkg;
    ll_node* next;
};

/**
 * @brief
 */
typedef struct ll {
    pthread_mutex_t ll_lock;
    ll_node* ll_head;
    ll_node* ll_tail;
} ll_t;

/**
 * @brief A linked list initalizer, please make sure to call free_ll() before terminal.
 * @param ll pointer of ll handle
 */
void ll_init(ll_t** ll);

/**
 * @brief free linked list
 *
 * @param ll pointer of ll handle
 */
void free_ll(ll_t** ll);
/**
 * @brief pop from linked list
 *
 * @param ll linked list handle
 * @return sr_pkg* the first element in ll , null if the ll is empty.
 */
sr_pkg* ll_pop(ll_t* ll);

/**
 * @brief push sr_pkg* into ll
 *
 * @param ll linked list handle
 * @param pkg pkg to push into ll
 */
void ll_push(ll_t* ll, sr_pkg* pkg);

/**
 * @brief check is the ll popable
 *
 * @param ll
 * @return int 0 if the ll is popable, 1 otherwise.
 */
int ll_peek(ll_t* ll);

#endif