/**
 * @file receiver.h
 * @author yangwei-ewe (jaychiang0127@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-05-04
 *
 * @copyright @楊偉 Copyright 2024
 */

#ifndef __RECEIVER_H_
#define __RECEIVER_H_
#include "sr.h"

 /*typedef struct {
     unsigned int seq_num;
     pkg_stat stat;
     size_t len;
 } sr_header;*/

void* receiver_send_daemon(void* args);
void* receiver_recv_daemon(void* args);
int recv_file(sr_obj* obj);
#endif
