#ifndef __XORSHIFT_H_
#define __XORSHIFT_H_
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct {
    uint32_t state;
} xorshift32_t;
typedef struct {
    uint32_t state[4];
} xorshift128_t;

uint32_t xorshift128_next(xorshift128_t* seed);
void xorshift128_init(xorshift128_t* seed);
uint32_t xorshift32_next(xorshift32_t* seed);
void xorshift32_init(xorshift32_t* seed);
#endif