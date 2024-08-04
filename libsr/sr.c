/**
 * @file sr.c
 * @author yangwei-ewe (jaychiang0127@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-04-29
 *
 * @copyright @楊偉 Copyright 2024
 */

#include "defines.h"
#include "linkedList.h"
#include "sr.h"
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#define AWAIT(cond, penalty) {\
    while (cond) {\
        usleep(ms_to_us(penalty));\
    }}

#define SR_STATIC_INITIALIZER (sr_obj){ NULL, 0, NULL, NULL, PTHREAD_MUTEX_INITIALIZER, 0, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 1, 0 }
inline int sr_init(sr_obj* obj) {
    *obj = SR_STATIC_INITIALIZER;
    return EXIT_SUCCESS;
}

#include "sender.h"
int sr_send(metadata* _metadata, size_t wnd_size, struct sockaddr_in* serv_info) {
    sr_obj sr;
    sr_init(&sr);
    sr.serv_info = serv_info;
    sr.metadata = _metadata;
    sr.metadata->_wnd_size = wnd_size;
    ll_init(&sr.waiting_lst);
    ll_init(&sr.sending_lst);
    TEST((pthread_create(&sr.waiting_daemon, NULL, sender_waiting_daemon, &sr)) != 0, "sr_init(): pthread_create failed.\n");
    TEST((pthread_create(&sr.sender_daemon, NULL, sender_send_daemon, &sr)) != 0, "sr_init(): pthread_create failed.\n");
    TEST((pthread_create(&sr.receiver_daemon, NULL, sender_recv_daemon, &sr)) != 0, "sr_init(): pthread_create failed.\n");
    fprintf(stderr, "WND size now set to: %llu\nsyncing...", wnd_size);
    sender_sync_wnd(&sr);
    send_file(&sr);
    TEST((sr.tmp = malloc(sizeof(sr_pkg))) == NULL, "sr_send(): malloc failed");
    *sr.tmp = (sr_pkg){ UINT32_MAX, 0, pkg_init, PTHREAD_MUTEX_INITIALIZER, {0, 0}, "\0" };
    ll_push(sr.sending_lst, sr.tmp);
    AWAIT(sr.is_exit, 50);
    free(sr.tmp);
    sr.tmp = NULL;
    free_sr(&sr);
    return EXIT_SUCCESS;
}

#include "receiver.h"
int sr_recv(size_t bufsize, struct sockaddr_in* serv_info) {
    sr_obj sr;
    sr_init(&sr);
    sr.serv_info = serv_info;
    ll_init(&sr.sending_lst);
    bst_init(&sr.bst, &sr);
    TEST((pthread_create(&sr.sender_daemon, NULL, receiver_send_daemon, &sr)) != 0, "sr_init(): pthread_create failed.\n");
    TEST((pthread_create(&sr.receiver_daemon, NULL, receiver_recv_daemon, &sr)) != 0, "sr_init(): pthread_create failed.\n");
    receiver_sync_wnd(&sr, bufsize);
    bst_update_wnd_size(sr.bst, sr.wnd_size);
    recv_file(&sr);
    char fake_md5[MD5_DIGEST_LENGTH];
    char filename_with_path[128] = "file-downloads/";
    strcat(filename_with_path, sr.metadata->filename);
    fprintf(stderr, "path name: %s\n", filename_with_path);
    _mkmd5(filename_with_path, fake_md5);
    fputs("recv file md5:\t", stdout);
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        printf("%02X", (uint8_t)fake_md5[i]);
    }
    fputc('\n', stdout);
    fputs("target md5: \t", stdout);
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        printf("%02X", (uint8_t)sr.metadata->checksum[i]);
    }
    fputc('\n', stdout);
    fflush(stdout);
    if (!strncmp(fake_md5, sr.metadata->checksum, MD5_DIGEST_LENGTH)) {
        fputs("they're same file\n", stderr);
    }
    else {
        fputs("they're different file\n", stderr);
    }
    free_sr(&sr);
    return EXIT_SUCCESS;
}

#include <libgen.h>
metadata* mkmetadata(char* filename) {
    metadata* rtn;
    TEST((rtn = (metadata*)malloc(sizeof(metadata))) == NULL, "get_metadata(): fail to malloc.");
    TEST((strlen(filename) > 64), "filename exceed max len (64 char here).");
    memset(rtn, 0, sizeof(metadata));
    strcpy(rtn->filename, basename(filename));
    _mkmd5(rtn->filename, rtn->checksum);
    fprintf(stderr, "filename: %s, md5:", filename);
    for (int i = 0;i < MD5_DIGEST_LENGTH;i++) {
        fprintf(stderr, "%02X", (uint8_t)rtn->checksum[i]);
    }
    fputc('\n', stderr);
    return rtn;
}

inline int _mkmd5(char* filename, char* checksumbuf) {
    MD5_CTX context;
    MD5_Init(&context);
    char buf[1024];
    int len = 0;
    int _fd;
    TEST((_fd = open(filename, O_RDONLY)) == -1, "failed to rd file");
    while ((len = read(_fd, buf, 1024)) != 0) {
        MD5_Update(&context, buf, len);
    }
    MD5_Final(checksumbuf, &context);
    close(_fd);
    return EXIT_SUCCESS;
}

void free_sr(sr_obj* sr) {
    if (sr->metadata != NULL) {
        free(sr->metadata);
    }
    if (sr->tmp != NULL) {
        free(sr->tmp);
    }
    pthread_join(sr->sender_daemon, NULL);
    pthread_join(sr->receiver_daemon, NULL);
    free_ll(&sr->sending_lst);
    if (sr->waiting_lst) {
        free_ll(&sr->waiting_lst);
    }
    pthread_mutex_destroy(&sr->base_seq_num_lock);
    return;
}

void sender_sync_wnd(sr_obj* obj) {
    fprintf(stderr, "now wnd size: %ld kB\n", obj->metadata->_wnd_size * PKG_SIZE / 1024);
    TEST((obj->tmp = (sr_pkg*)malloc(sizeof(sr_pkg))) == NULL, "sr.tmp malloc failed");
    *obj->tmp = (sr_pkg){ 0, sizeof(metadata), flg_init, pkg_waiting, PTHREAD_MUTEX_INITIALIZER, "\0" };
    memcpy(obj->tmp->data, obj->metadata, sizeof(metadata));
    ll_push(obj->sending_lst, obj->tmp);
    AWAIT(obj->tmp->stat != pkg_exit, 100);
    pthread_mutex_lock(&obj->base_seq_num_lock);
    obj->base_seq_num++;
    pthread_mutex_unlock(&obj->base_seq_num_lock);
    fprintf(stderr, "wndsize = %lu\n", (obj->wnd_size = MAX(__gcd(obj->metadata->_wnd_size, *(obj->tmp->data)), WND_MIN_SIZE)));
    free(obj->tmp);
    obj->tmp = NULL;
    return;
}

void receiver_sync_wnd(sr_obj* obj, size_t self_wnd_size) {
    TEST((obj->tmp = malloc(sizeof(sr_pkg))) == NULL, "receiver_sync_wnd(): tmp malloc failed");
    *obj->tmp = (sr_pkg){ 0, 0, pkg_init, PTHREAD_MUTEX_INITIALIZER, {0, 0}, "\0" };
    /*while (obj->tmp->stat == pkg_init) {
        usleep(ms_to_us(250));
    }*/
    AWAIT(obj->tmp->stat == pkg_init, 100);
    metadata* _metadata = obj->tmp->data;
    fprintf(stderr, "seq_num = %u \nsender_wnd = %lu\nfilename = %s\n", obj->tmp->seq_num, _metadata->_wnd_size, _metadata->filename);
    TEST((obj->metadata = malloc(sizeof(metadata))) == NULL, "receiver_sync_wnd(): metadta malloc failed");
    memcpy(obj->metadata, _metadata, sizeof(metadata));
    fprintf(stderr, "wndsize = %lu\n", (obj->wnd_size = MAX(__gcd(obj->metadata->_wnd_size, self_wnd_size), WND_MIN_SIZE)));
    sr_pkg* pkg;
    TEST((pkg = (sr_pkg*)malloc(sizeof(sr_pkg))) == NULL, "receiver_sync_wnd(): metadta malloc failed");
    *pkg = (sr_pkg){ 0, sizeof(size_t), flg_init, pkg_init, PTHREAD_MUTEX_INITIALIZER, {0, 0}, (void*)self_wnd_size };
    pthread_mutex_lock(&pkg->pkg_lock);
    pkg->stat = pkg_waiting;
    pthread_mutex_unlock(&pkg->pkg_lock);
    ll_push(obj->sending_lst, pkg);
    pthread_mutex_lock(&obj->base_seq_num_lock);
    obj->base_seq_num++;
    pthread_mutex_unlock(&obj->base_seq_num_lock);
    free(obj->tmp);
    obj->tmp = NULL;
    return;
}
