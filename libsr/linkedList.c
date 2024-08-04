/**
 * @file linkedList.c
 * @author yangwei-ewe (jaychiang0127@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-05-01
 *
 * @copyright @楊偉 Copyright 2024
 */

#include "linkedList.h"
#include "defines.h"

void ll_init(ll_t** _ll) {
    if (*_ll) {
        free(*_ll);
    }
    TEST((*_ll = (ll_t*)malloc(sizeof(ll_t))) == NULL, "ll_init(): failed to malloc ll_t.");
    (*_ll)->ll_head = (*_ll)->ll_tail = NULL;
    pthread_mutex_init(&(*_ll)->ll_lock, NULL);
}

void free_ll(ll_t** _ll) {
    ll_node* ptr = (*_ll)->ll_head;
    while (ptr != NULL) {
        ll_node* tmp = ptr->next;
        free(ptr);
        ptr = tmp;
    }
    pthread_mutex_destroy(&(*_ll)->ll_lock);
    free(*_ll);
    *_ll = NULL;
    return;
}

sr_pkg* ll_pop(ll_t* ll_t) {
    if (ll_t->ll_head == NULL) {
        return NULL;
    }
    sr_pkg* rtn = ll_t->ll_head->pkg;
    ll_node* temp = ll_t->ll_head;
    pthread_mutex_lock(&ll_t->ll_lock);
    ll_t->ll_head = ll_t->ll_head->next;
    if (ll_t->ll_head == ll_t->ll_tail->next) { // ll is blank
        ll_t->ll_tail = NULL;
    }
    pthread_mutex_unlock(&ll_t->ll_lock);
    free(temp);
    return rtn;
}

void ll_push(ll_t* ll_t, sr_pkg* data) {
    ll_node* new_node;
    TEST((new_node = (ll_node*)malloc(sizeof(ll_node))) == NULL, "ll_push(): failed to malloc new node.");
    new_node->pkg = data;
    new_node->next = NULL;
    pthread_mutex_lock(&ll_t->ll_lock);
    if (ll_t->ll_tail == NULL) {
        ll_t->ll_head = ll_t->ll_tail = new_node;
    }
    else {
        ll_t->ll_tail->next = new_node;
        ll_t->ll_tail = ll_t->ll_tail->next;
    }
    pthread_mutex_unlock(&ll_t->ll_lock);
    return 0;
}

int ll_peek(ll_t* ll_t) {
    if (ll_t->ll_head == NULL) {
        return 1;
    }
    return 0;
}

sr_pkg* ll_find(ll_t* ll, size_t index) {
    if (ll->ll_head == NULL) {
        return NULL;
    }
    ll_node* curr = ll->ll_head;
    size_t count = 0;
    pthread_mutex_lock(&ll->ll_lock);
    while (count < index) {
        curr = curr->next;
        count++;
    }
    pthread_mutex_unlock(&ll->ll_lock);
    return curr->pkg;
}
