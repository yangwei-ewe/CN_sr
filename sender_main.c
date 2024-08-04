/**
 * @file tmp_sender.c
 * @author yangwei-ewe (jaychiang0127@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-05-05
 *
 * @copyright @楊偉 Copyright 2024
 */

#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "libsr/defines.h"
#include <unistd.h>
#include "libsr/sr.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
 //
//ip:127.0.0.1:1620
//ip:127.0.0.1:1630
int main() {
    puts("\e[1;1H\e[2J");
    struct sockaddr_in serv_info = { AF_INET,htons(1620),inet_addr("127.0.0.1"),"\0" };
    sr_send(mkmetadata("hk416.gif"), 22, &serv_info);
}