/**
 * @file sr.h
 * @author yangwei-ewe (jaychiang0127@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-04-29
 *
 * @copyright @楊偉 Copyright 2024
 */

#ifndef __SR_H_
#define __SR_H_
#define _GNU_SOURCE
#include <pthread.h>
#include <stdint.h>
#include <sys/types.h>
#include <openssl/md5.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "defines.h"

typedef struct ll ll_t;
typedef struct buffer buffer_t;
typedef struct bst_t bst_t;

typedef struct {
    char filename[64];
    unsigned char checksum[MD5_DIGEST_LENGTH];
    size_t _wnd_size;
} metadata;

typedef enum pkg_stat {
    pkg_init,   //stat of init
    pkg_waiting,//pkg is waiting to send
    pkg_sending,//pkg sended, wait for timeout-resend or ack
    pkg_acked,  //pkg received ack from receiver
    pkg_exit,   //stat to raise flag to tell cll pop from buffer
} pkg_stat;

typedef enum pkg_flags {
    flg_init = 0,
    flg_close = UINT8_MAX  //flg to raise close sign
}pkg_flags;

#include <sys/time.h>
typedef struct sr_pkg {
    uint32_t seq_num;
    size_t len;
    pkg_flags flags;
    pkg_stat stat;
    pthread_mutex_t pkg_lock;
    struct timeval brust_time; //absolute time for wake up
    char data[PKG_SIZE];
} sr_pkg; //for sending


/** sr_header + data be like:
 * +-------------------------------------------------+
 * |         seq_num        |  len (of data behind)  |
 * +-------------------------------------------------+
 * |                                                 |
 * |                                                 |
 * |                 data (max 2048)                 |
 * |                                                 |
 * |                                                 |
 * +-------------------------------------------------+
 */

typedef struct sr_header {
    uint32_t seq_num;
    size_t len;
    pkg_flags flags;
} sr_header;

#include "bst.h"
typedef struct sr_obj {
    metadata* metadata;
    size_t wnd_size;
    pthread_t* wnd_pid;
    sr_pkg* tmp;
    pthread_mutex_t base_seq_num_lock;
    size_t base_seq_num;
    struct sockaddr_in* serv_info;
    ll_t* sending_lst;
    ll_t* waiting_lst;
    buffer_t* buffer;
    bst_t* bst;
    pthread_t sender_daemon;
    pthread_t receiver_daemon;
    pthread_t waiting_daemon;
    uint8_t is_exit;
    int _fd;
} sr_obj;

/**
 * @brief the zero-initializer of sr_obj.
 *
 * @param obj
 * @return 0 stands for success, 1 otherwise
 * @note still under construction
 */
int sr_init(sr_obj* obj);

/**
 * @brief func to send somthing via sr.
 *
 * @param metadata the metadata of obj to send
 * @param bufsize the size of buffer on this machine, true buf size will be determined afer sync.
 * @param serv_info dest. ip
 * @return int
 */
int sr_send(metadata* metadata, size_t bufsize, struct sockaddr_in* serv_info);

/**
 * @brief func to recv file from sr.
 *
 * @param bifsize the size of buffer on this machine, true buf size will be determined afer sync.
 * @param serv_info
 * @return int
 */
int sr_recv(size_t bifsize, struct sockaddr_in* serv_info);

/**
 * @brief gen metadata from file name, including update checksum, filename and wnd size.
 *
 * @param filename full path of the file
 * @return the pointer to the metadata, note that the memory needs to free brefore terminate the prog.
 */
metadata* mkmetadata(char* filename);

/**
 * @brief free sr after use
 *
 * @param sr obj of sr.
 */
void free_sr(sr_obj* sr);
#endif
