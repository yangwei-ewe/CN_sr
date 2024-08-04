//#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "defines.h"
#include "buffer_queue.h"
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <fcntl.h>
#include "sr.h"

int exit_sgn = 1;
buffer_t* buffer;

void* my_pop(void* args) {
    sr_obj* obj = (sr_obj*)args;
    while (obj->is_exit) {
        while (obj->is_exit && ll_peek(obj->sending_lst)) {
            puts("no data...");
            usleep(ms_to_us(1));
        }
        if (!obj->is_exit) {
            break;
        }
        sr_pkg* pkg;
        TEST((pkg = ll_pop(obj->sending_lst)) == NULL, "get null from sending list which should not be empty");
        //printf("seq: %5u\t len: %5d\ndata: %0.*s", pkg->seq_num, pkg->len, pkg->len, pkg->data);
        printf("seq: %5u len: %5d acked.\n", pkg->seq_num, pkg->len);
        wnd_ack(obj->buffer, pkg->seq_num);
        usleep(ms_to_us(2));
    }
    pthread_exit(NULL);
}

int main() {
    pthread_t thread;
    ll_t* sending_lst = NULL;
    ll_init(&sending_lst);
    sr_obj* obj;
    TEST((obj = (sr_obj*)malloc(sizeof(sr_obj))) == NULL, "malloc failed.");
    sr_init(obj);
    obj->sending_lst = sending_lst;
    obj->metadata = mkmetadata("hk416.gif");
    obj->wnd_size = obj->metadata->_wnd_size = 11;
    fprintf(stderr, "filename: %s\nwnd: %d\n", obj->metadata->filename, obj->metadata->_wnd_size);
    //TEST((obj->_fd = open("receiver.c", O_RDONLY)) == -1, "not such file");
    buffer_init(&buffer, obj);
    pthread_create(&thread, NULL, my_pop, obj);
    obj->buffer = buffer;
    fprintf(stderr, "ok\n");
    //fprintf(stderr, "ok2\n");
    while (obj->is_exit) {
        buffer_update(obj->buffer);
        fprintf(stderr, "updated\n");
        usleep(ms_to_us(2));
    }
    exit_sgn = 0;
    pthread_join(thread, NULL);
    free_buffer(&obj->buffer);
    fprintf(stderr, "buffer freed.\n");
    free_sr(obj);
    fprintf(stderr, "sr_obj freed.\n");
    return EXIT_SUCCESS;
}

/*int main() {
    int _fd = open("receiver.c", O_RDONLY);
    char buf[1024];
    ssize_t len;
    int count = 0;
    while (count < 3 && (len = read(_fd, buf, 1024)) >= 0) {
        if (len == 0) {
            printf("EOF: %d\n", ++count);
            continue;
        }
        printf("%ld: ", len);
        printf("%.*s", len, buf);
    }
    puts("===eof time 3===\n");
    close(_fd);
    return EXIT_SUCCESS;
}*/
