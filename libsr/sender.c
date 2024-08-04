/**
 * @file sender.c
 * @author yangwei-ewe (jaychiang0127@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-04-28
 *
 * @copyright @楊偉 Copyright 2024
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "defines.h"
#include "sender.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include "linkedList.h"
#include "buffer_queue.h"

void* sender_send_daemon(void* args) {
    sr_obj* obj = (sr_obj*)args;
    int sock_fd, rtval;
    sr_pkg* pkg;
    struct sockaddr_in serv_info = { AF_INET,htons(1630),inet_addr("127.0.0.1"),"\0" };
    TEST((sock_fd = socket(AF_INET, SOCK_DGRAM/*udp*/ | SOCK_NONBLOCK, 0/*auto*/)) == -1, "socket create faild.");
    while (obj->is_exit) {
        while (obj->is_exit && ll_peek(obj->sending_lst)) {
            //fputs("nothing to send.\n", stderr);
            usleep(ms_to_us(0.5));
        }
        if (!obj->is_exit) {
            pthread_exit(EXIT_SUCCESS);
        }
        TEST((pkg = ll_pop(obj->sending_lst)) == NULL, "ll_pop() failed.");
        if (pkg->stat == pkg_acked) {
            fputs("already acked, skip.\n", stderr);
            pthread_mutex_lock(&pkg->pkg_lock);
            pkg->stat = pkg_exit;
            pthread_mutex_unlock(&pkg->pkg_lock);
            continue;
        }
        //printf("seq. num: %u ...sending\n", pkg->seq_num);
        fflush(stdout);
        while (rtval = sendto(sock_fd, pkg, sizeof(sr_header), 0, &serv_info, sizeof(struct sockaddr_in)) == -1) {
            fputs(stderr, "header send failed, retrying...");
            usleep(ms_to_us(5));
        }
        if (pkg->len) {
            while (rtval = sendto(sock_fd, pkg->data, pkg->len, 0, &serv_info, sizeof(struct sockaddr_in)) == -1) {
                fputs(stderr, "body send failed, retrying...");
                usleep(ms_to_us(5));
            }
        }
        if (pkg->stat < pkg_acked) {
            pthread_mutex_lock(&pkg->pkg_lock);
            pkg->stat = pkg_sending;
            pthread_mutex_unlock(&pkg->pkg_lock);
        }
        //printf("seq. num: %u ...done.\n", pkg->seq_num);
        gettimeofday(&pkg->brust_time, NULL);
        pkg->brust_time.tv_usec += ms_to_us(20);
        ll_push(obj->waiting_lst, pkg);
    }
    pthread_exit(NULL);
}

void* sender_recv_daemon(void* args) {
    sr_obj* obj = (sr_obj*)args;
    int sock_fd, rtval;
    sr_pkg buf = { 0, 0, flg_close, pkg_init,PTHREAD_MUTEX_INITIALIZER, {0, 0}, "\0" };
    struct sockaddr_in serv_info = { AF_INET,htons(1620),inet_addr("127.0.0.1"),"\0" };
    //TEST((buf = (sr_pkg_t*)malloc(sizeof(sr_pkg_t))) == NULL, "malloc() failed");
    TEST((sock_fd = socket(AF_INET, SOCK_DGRAM/*udp*/ | SOCK_NONBLOCK, 0/*auto*/)) == -1, "socket create faild.");
    TEST((rtval = bind(sock_fd, &serv_info, sizeof(struct sockaddr_in)) == -1), "bind(): fialed.");
    while (1) {
        while ((rtval = recvfrom(sock_fd, &buf, sizeof(sr_header), NULL, NULL, NULL) == -1)) {
            //fputs("recv failed, retrying...\n", stderr);
            usleep(ms_to_us(0.5));
        }
        if (buf.len) {
            //fputs("data recved.\n", stderr);
            while ((rtval = recvfrom(sock_fd, &buf.data, buf.len, NULL, NULL, NULL) == -1)) {
                fputs("recv body failed, retrying...\n", stderr);
                usleep(ms_to_us(0.5));
            }
        }
        if (buf.flags == flg_close) {
            obj->is_exit = 0;
            fprintf(stderr, "all recved exit\n");
            pthread_exit(EXIT_SUCCESS);
        }
        //printf("recved. seq. num: %u\n", buf.seq_num);
        if (buf.seq_num == 0) { // for sync metadata use
            pthread_mutex_lock(&obj->tmp->pkg_lock);
            if (buf.len) {
                memcpy(obj->tmp->data, &buf.data, buf.len);
            }
            obj->tmp->stat = pkg_acked;
            pthread_mutex_unlock(&obj->tmp->pkg_lock);
            continue;
        }
        pthread_mutex_lock(&obj->base_seq_num_lock);
        pthread_mutex_unlock(&obj->base_seq_num_lock);
        wnd_ack(obj->buffer, buf.seq_num);
        fprintf(stderr, "seq. %d ack complete\n", buf.seq_num);
    }
    pthread_exit(NULL);
}

void* sender_waiting_daemon(void* args) {
    sr_obj* obj = (sr_obj*)args;
    int sock_fd, rtval;
    sr_pkg* pkg;
    struct timeval tv;
    while (obj->is_exit) {
        while (obj->is_exit && ll_peek(obj->waiting_lst)) {
            usleep(ms_to_us(3));
        }
        if (!obj->is_exit) {
            pthread_exit(EXIT_SUCCESS);
        }
        TEST((pkg = ll_pop(obj->waiting_lst)) == NULL, "sender_waiting_daemon(): ll_pop failed.");
        pthread_mutex_lock(&pkg->pkg_lock);
        if (pkg->stat == pkg_acked) {
            pkg->stat = pkg_exit;
            pthread_mutex_unlock(&pkg->pkg_lock);
            continue;
        }
        gettimeofday(&tv, NULL);
        long diff = (pkg->brust_time.tv_sec - tv.tv_sec) * 1e6 + (pkg->brust_time.tv_usec - tv.tv_usec) - ms_to_us(0.5);
        fprintf(stderr, "seq. num: %u\ttime to sleep: %lf ms\n", pkg->seq_num, diff / 1e3);
        if (diff > 0) {
            usleep(diff);
        }
        pkg->stat = pkg_waiting;
        pthread_mutex_unlock(&pkg->pkg_lock);
        ll_push(obj->sending_lst, pkg);
    }
    pthread_exit(NULL);
}
int send_file(sr_obj* obj) {
    buffer_init(&obj->buffer, obj);
    while (buffer_update(obj->buffer)) {
        usleep(ms_to_us(10));
    }
    free_buffer(&obj->buffer);
    sr_pkg* close_pkg;
    TEST((close_pkg = (sr_pkg*)malloc(sizeof(sr_pkg))) == NULL, "send_file(): malloc() failed");
    *close_pkg = (sr_pkg){ obj->base_seq_num, 0, flg_close, pkg_init, PTHREAD_MUTEX_INITIALIZER, {0, 0}, "\0" };
    ll_push(obj->sending_lst, close_pkg);
    while (obj->is_exit) {
        fprintf(stderr, "wait for exit\n");
        usleep(ms_to_us(10));
    }
    free(close_pkg);
    return EXIT_SUCCESS;
}
