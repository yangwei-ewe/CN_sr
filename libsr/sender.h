/**
 * @file sender.h
 * @author yangwei-ewe (jaychiang0127@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-05-04
 *
 * @copyright @楊偉 Copyright 2024
 */
#ifndef __SENDER_H_
#define __SENDER_H_
#include "sr.h"

void* sender_send_daemon(void* args);
void* sender_recv_daemon(void* args);
void* sender_waiting_daemon(void* args);
int send_file(sr_obj* obj);
#endif