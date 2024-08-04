/**
 * @file buffer.c
 * @author yangwei-ewe (jaychiang0127@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-05-26
 *
 * @copyright @楊偉 Copyright 2024
 */

#include "buffer_queue.h"
#include "defines.h"
#include "sr.h"

#include <fcntl.h>
void buffer_init(buffer_t** buf, sr_obj* obj) {
    int rtnval;
    TEST((*buf = (buffer_t*)malloc(sizeof(buffer_t))) == NULL, "buffer_init(): failed to malloc buffer_t.");
    TEST(obj == NULL, "buffer_init(): sending lst is not initialized.");
    **buf = (buffer_t){ PTHREAD_MUTEX_INITIALIZER, NULL,NULL,obj, 0, 0, obj->wnd_size, obj->wnd_size * 2, 1 };
    ll_init(&(*buf)->wnd);
    ll_init(&(*buf)->buf);
    TEST(((*buf)->obj->_fd = open(obj->metadata->filename, O_RDONLY)) == -1, "cannot open file!");
    return;
}

void free_buffer(buffer_t** buf) {
    close((*buf)->obj->_fd);
    free_ll(&(*buf)->wnd);
    free_ll(&(*buf)->buf);
    free(*buf);
    *buf = NULL;
    return;
}

void buffer_pop(buffer_t* buf) {
    if (buf->curr_wnd_len == 0) {
        return;
    }
    sr_pkg* pkg;
    pthread_mutex_lock(&buf->buffer_lock);
    TEST((pkg = ll_pop(buf->wnd)) == NULL, "buffer_pop(): ll_pop() invoked but it should not be null");
    buf->curr_wnd_len--;
    free(pkg);
    pthread_mutex_unlock(&buf->buffer_lock);
    return;
}

void buffer_push(buffer_t* buf, sr_pkg* pkg) {
    pthread_mutex_lock(&buf->buffer_lock);
    if (buf->curr_buf_len + buf->curr_wnd_len == buf->def_buf_len) {
        pthread_mutex_unlock(&buf->buffer_lock);
        fprintf(stderr, "buffer full\n");
        return;
    }
    ll_push(buf->buf, pkg);
    buf->curr_buf_len++;
    //fprintf(stderr, "curr buffer len: %d\n", buf->curr_buf_len);
    pthread_mutex_unlock(&buf->buffer_lock);
    return;
}

sr_header* buffer_peek(buffer_t* buf) {
    sr_pkg* rtn;
    pthread_mutex_lock(&buf->buffer_lock);
    rtn = buf->wnd->ll_head;
    pthread_mutex_unlock(&buf->buffer_lock);
    return rtn;
}

void wnd_ack(buffer_t* buf, size_t seq_num) {
    ll_node* ptr = buf->wnd->ll_head;
    pthread_mutex_lock(&buf->buffer_lock);
    if ((ptr == NULL) || (seq_num < buf->wnd->ll_head->pkg->seq_num)) {
        pthread_mutex_unlock(&buf->buffer_lock);
        fputs("old packet, ignore.\n", stderr);
        return;
    }
    while ((ptr != NULL) && (ptr->pkg->seq_num != seq_num)) {
        //fprintf(stderr, "wnd_ack(): starget seq: %u seq: %u len: %d\n", seq_num, ptr->pkg->seq_num, ptr->pkg->len);
        ptr = ptr->next;
    }
    if (ptr == NULL) {
        pthread_mutex_unlock(&buf->buffer_lock);
        fputs("unfound in wnd\n", stderr);
        return;
    }
    pthread_mutex_lock(&ptr->pkg->pkg_lock);
    ptr->pkg->stat = MAX(pkg_acked, ptr->pkg->stat);
    pthread_mutex_unlock(&ptr->pkg->pkg_lock);
    pthread_mutex_unlock(&buf->buffer_lock);
    return;
}

int buffer_is_pushable(buffer_t* buf) {// 1 if pushable, 0 otherwise
    pthread_mutex_lock(&buf->buffer_lock);
    if (buf->is_EOF && ((buf->curr_wnd_len + buf->curr_buf_len) < buf->def_buf_len)) {
        pthread_mutex_unlock(&buf->buffer_lock);
        return 1;
    }
    else {
        pthread_mutex_unlock(&buf->buffer_lock);
        return 0;
    }
}

int wnd_update(buffer_t* buf) {
    if (buf->curr_wnd_len >= buf->def_wnd_len) {
        return EXIT_FAILURE;
    }
    pthread_mutex_lock(&buf->buffer_lock);
    sr_pkg* pkg;
    while (buf->curr_buf_len && buf->curr_wnd_len < buf->def_wnd_len) {
        ll_push(buf->wnd, (pkg = ll_pop(buf->buf)));
        //fprintf(stderr, "seq: %u is pushed to sending lst\n", pkg->seq_num);
        ll_push(buf->obj->sending_lst, pkg);
        buf->curr_wnd_len++;
        buf->curr_buf_len--;
    }
    pthread_mutex_unlock(&buf->buffer_lock);
    return EXIT_SUCCESS;
}

int buffer_update(buffer_t* buf) {
    buf_node* tmp;
    ssize_t len; // len for read();
    sr_pkg* new_pkg;
    while (((tmp = buffer_peek(buf)) != NULL) && tmp->pkg->stat == pkg_exit) {
        pthread_mutex_lock(&buf->obj->base_seq_num_lock);
        buf->obj->base_seq_num = tmp->pkg->seq_num + 1;
        pthread_mutex_unlock(&buf->obj->base_seq_num_lock);
        buffer_pop(buf);
    }
    while (buffer_is_pushable(buf)) {
        TEST((new_pkg = (sr_pkg*)malloc(sizeof(sr_pkg))) == NULL, "buffer_update(): failed to allocate new sr_pkg");
        *new_pkg = (sr_pkg){ 0, 0, flg_init, pkg_init, PTHREAD_MUTEX_INITIALIZER, {0, 0}, "\0" };
        TEST((len = read(buf->obj->_fd, new_pkg->data, PKG_SIZE)) == -1, "buffer_update(): read failed");
        if (len == 0) {
            //fprintf(stderr, "EOF detect\n");
            pthread_mutex_lock(&buf->buffer_lock);
            buf->is_EOF = 0;
            pthread_mutex_unlock(&buf->buffer_lock);
            free(new_pkg);
            break;
        }
        else if (buf->buf->ll_tail == NULL) {
            new_pkg->seq_num = 1;
            new_pkg->len = len;
        }
        else {
            new_pkg->seq_num = buf->buf->ll_tail->pkg->seq_num + 1;
            new_pkg->len = len;
        }
        buffer_push(buf, new_pkg);
        usleep(ms_to_us(1));
    }
    wnd_update(buf);
    fprintf(stderr, "base_sqe_num is now set to %lu\n", buf->obj->base_seq_num);
    return buf->is_EOF || buf->curr_wnd_len;
}
