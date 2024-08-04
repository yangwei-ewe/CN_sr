/**
 * @file receiver.c
 * @author yangwei-ewe (jaychiang0127@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-05-04
 *
 * @copyright @楊偉 Copyright 2024
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "defines.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "linkedList.h"
#include "bst.h"

void* receiver_send_daemon(void* args) {
    sr_obj* obj = (sr_obj*)args;
    int sock_fd;
    int rtval;
    sr_pkg* pkg;
    struct sockaddr_in serv_info = { AF_INET,htons(1620),inet_addr("127.0.0.1"),"\0" };
    TEST((sock_fd = socket(AF_INET, SOCK_DGRAM/*udp*/ | SOCK_NONBLOCK, 0/*auto*/)) == -1, "socket create faild.");
    while (obj->is_exit) {
        while (obj->is_exit && ll_peek(obj->sending_lst)) {
            //fputs("nothing to send.\n", stderr);
            usleep(ms_to_us(0.5));
        }
        if (!obj->is_exit) {
            break;
        }
        TEST((pkg = ll_pop(obj->sending_lst)) == NULL, "ll_pop() failed.");
        //printf("seq. num: %u ...sending\n", pkg->seq_num);
        fflush(stdout);
        while (rtval = sendto(sock_fd, pkg, sizeof(sr_header), 0, &serv_info, sizeof(struct sockaddr_in)) == -1) {
            fputs("header send failed, retrying...", stderr);
            usleep(ms_to_us(5));
        }
        if (pkg->len) {
            while (rtval = sendto(sock_fd, pkg->data, pkg->len, 0, &serv_info, sizeof(struct sockaddr_in)) == -1) {
                fputs("body send failed, retrying...", stderr);
                usleep(ms_to_us(5));
            }
        }
        //printf("seq. num: %u ...done.\n", pkg->seq_num);
        free(pkg);
    }
    pthread_exit(NULL);
}

#include "xorshift.h"
void* receiver_recv_daemon(void* args) {
    xorshift32_t xor;
    xorshift32_init(& xor);
    sr_obj* obj = (sr_obj*)args;
    int sock_fd, rtval;
    sr_pkg* pkg, * ack_pkg;
    sr_pkg buf = { UINT32_MAX, 0, pkg_init, PTHREAD_MUTEX_INITIALIZER, {0, 0}, "\0" };
    struct sockaddr_in serv_info = { AF_INET, htons(1630), inet_addr("127.0.0.1"), "\0" };
    TEST((sock_fd = socket(AF_INET, SOCK_DGRAM/*udp*/ | SOCK_NONBLOCK, 0/*auto*/)) == -1, "socket create faild.");
    TEST((rtval = bind(sock_fd, &serv_info, sizeof(struct sockaddr_in)) == -1), "bind(): fialed.");
    while (obj->is_exit) {
        while (obj->is_exit && (rtval = recvfrom(sock_fd, &buf, sizeof(sr_header), NULL, NULL, NULL) == -1)) {
            //fputs("recv failed, retrying...\n", stderr);
            usleep(ms_to_us(0.5));
        }
        if (buf.len) {
            //fputs("data recved.\n", stderr);
            while (obj->is_exit && (rtval = recvfrom(sock_fd, &buf.data, buf.len, NULL, NULL, NULL) == -1)) {
                //fputs("recv body failed, retrying...\n", stderr);
                usleep(ms_to_us(0.5));
            }
        }
        if (xorshift32_next(& xor) / (UINT32_MAX * 1.0) < 0.2) {
            printf("seq. %u lost.\n", buf.seq_num);
            fflush(stdout);
            continue;
        }
        if (!obj->is_exit) {
            pthread_exit(EXIT_SUCCESS);
        }
        //fprintf(stderr, "recved. seq: %u\tlen: %lu\n", buf.seq_num, buf.len);
        pthread_mutex_lock(&obj->base_seq_num_lock);
        if (obj->base_seq_num > buf.seq_num) {
            pthread_mutex_unlock(&obj->base_seq_num_lock);
            fprintf(stderr, "old packet, ignore.\n");
            TEST((ack_pkg = (sr_pkg*)malloc(sizeof(sr_pkg))) == NULL, "recviver_recv_daemon(): malloc failed");
            *ack_pkg = (sr_pkg){ buf.seq_num, 0, pkg_init, PTHREAD_MUTEX_INITIALIZER, { 0, 0}, "\0" };
            ll_push(obj->sending_lst, ack_pkg);
            continue;
        }
        if (buf.seq_num > obj->base_seq_num + obj->wnd_size) {
            pthread_mutex_unlock(&obj->base_seq_num_lock);
            fprintf(stderr, "out of range, ignore.\n");
            continue;
        }
        pthread_mutex_unlock(&obj->base_seq_num_lock);
        if (buf.seq_num == 0) {
            if (buf.len) {
                pthread_mutex_lock(&obj->tmp->pkg_lock);
                memcpy(obj->tmp->data, &buf.data, buf.len);
                obj->tmp->stat = pkg_acked;
                pthread_mutex_unlock(&obj->tmp->pkg_lock);
            }
            continue;
        }
        if (bst_find(obj->bst, buf.seq_num)) {
            fprintf(stderr, "seq: %u repeated. ignored\n", buf.seq_num);
            continue;
        }
        while (!bst_is_pushable(obj->bst)) {
            fputs("reach bst limit\n", stderr);
            usleep(ms_to_us(5));
        }
        TEST((pkg = (sr_pkg*)malloc(sizeof(sr_pkg))) == NULL, "recviver_recv_daemon(): malloc failed");
        memcpy(pkg, &buf, sizeof(sr_pkg));
        bst_push(obj->bst, pkg);
        TEST((ack_pkg = (sr_pkg*)malloc(sizeof(sr_pkg))) == NULL, "recviver_recv_daemon(): malloc failed");
        *ack_pkg = (sr_pkg){ buf.seq_num, 0, buf.flags, pkg_init, PTHREAD_MUTEX_INITIALIZER, { 0, 0}, "\0" };
        ll_push(obj->sending_lst, ack_pkg);
        //fprintf(stderr, "recv_recv_daemon(): bst_push completed\n");
    }
    pthread_exit(NULL);
}

#include <sys/stat.h>
#include <fcntl.h>
int recv_file(sr_obj* obj) {
    struct stat st;
    const char* basedir = "file-downloads/";
    char* filename_with_path[128];
    sr_pkg* pkg;
    strcpy(filename_with_path, basedir);
    strcat(filename_with_path, obj->metadata->filename);
    if (stat(basedir, &st) == -1/*if basedir doesn't exit*/) {
        mkdir(basedir, 0755); // create directory
    }
    TEST((obj->_fd = open(filename_with_path, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU | S_IRGRP | S_IROTH)) == -1, "cannot open file");
    /*obj->_fd = open(filename_with_path, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU | S_IRGRP | S_IROTH);
    if (obj->_fd == -1) {
        perror("error: ");
        return EXIT_FAILURE;
    }*/
    while (obj->is_exit || bst_is_not_empty(obj->bst)) {
        pthread_mutex_lock(&obj->base_seq_num_lock);
        while (((pkg = bst_find(obj->bst, obj->base_seq_num)) == NULL)) {
            pthread_mutex_unlock(&obj->base_seq_num_lock);
            //fprintf(stderr, "cannot found seq %u in bst\n", obj->base_seq_num);
            usleep(ms_to_us(10));
            pthread_mutex_lock(&obj->base_seq_num_lock);
        }
        pthread_mutex_unlock(&obj->base_seq_num_lock);
        pkg = bst_pop(obj->bst);
        if (pkg->flags == flg_close) {
            obj->is_exit = 0;
            printf("all recv, exiting..\n");
            pthread_mutex_lock(&obj->base_seq_num_lock);
            obj->base_seq_num++;
            pthread_mutex_unlock(&obj->base_seq_num_lock);
            break;
        }
        //fprintf(stderr, "pkg found: %u\n", pkg->seq_num);
        write(obj->_fd, pkg->data, pkg->len);
        pthread_mutex_lock(&obj->base_seq_num_lock);
        obj->base_seq_num++;
        pthread_mutex_unlock(&obj->base_seq_num_lock);
    }
    free_bst(&obj->bst);
    close(obj->_fd);
    return EXIT_SUCCESS;
}
