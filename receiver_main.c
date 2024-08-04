/**
 * @file tmp_receiver.c
 * @author yangwei-ewe (jaychiang0127@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-05-06
 *
 * @copyright @楊偉 Copyright 2024
 */

#define _GNU_SOURCE
#include <string.h>
 //#include <stdio.h>
#include <stdlib.h>
#include "libsr/defines.h"
#include "libsr/sr.h"

int main() {
    puts("\e[1;1H\e[2J");
    struct sockaddr_in serv_info = { AF_INET,htons(1630),inet_addr("127.0.0.1"),"\0" };
    sr_recv(11, &serv_info);
    return EXIT_SUCCESS;
}