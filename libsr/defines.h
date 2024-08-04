/**
 * @file defines.h
 * @author yangwei-ewe (jaychiang0127@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-04-29
 *
 * @copyright @楊偉 Copyright 2024
 */
#ifndef __DEFINES_H__
#define __DEFINES_H__
#include <stdio.h>
#include <stdlib.h>
 /**
  * @brief print error_msg then exit if cond is true
  */
#define TEST(cond, error_msg) {if (cond) {    \
        fprintf(stderr, error_msg);\
        fputc('\n', stderr);\
        exit(EXIT_FAILURE);\
        }\
    }

#define MIN(a, b) (a > b ? b : a)
#define MAX(a, b) (a > b ? a : b)
#define PKG_SIZE 8 * 1024 //8kib per pkg
#define WND_MIN_SIZE 4
#define ms_to_us(ms) (ms*1000)

  //inline size_t __gcd(size_t a, size_t b) {
static inline size_t __gcd(size_t a, size_t b) {
  while (b != 0) {
    size_t temp = b;
    b = a % b;
    a = temp;
  }
  return a;
}
#endif
