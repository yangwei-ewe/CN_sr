#include "xorshift.h"
#ifdef __linux
#include <sys/random.h>
#endif

uint32_t xorshift128_next(xorshift128_t* seed) {
    uint32_t t = seed->state[3];
    t ^= t << 11;
    t ^= t >> 8;
    seed->state[3] = seed->state[2];
    seed->state[2] = seed->state[1];
    seed->state[1] = seed->state[0];
    seed->state[0] = t ^ (t >> 19) ^ (seed->state[0] ^ (seed->state[0] >> 19));
    return seed->state[0];
}

void xorshift128_init(xorshift128_t* seed) {
    srand(time(NULL));
    for (int i = 0;i < 4;i++) {
        seed->state[i] = (uint32_t)rand();
    }
    return;
}

uint32_t xorshift32_next(xorshift32_t* seed) {
    uint32_t x = seed->state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return (seed->state = x);
}

void xorshift32_init(xorshift32_t* seed) {
    getrandom(&seed->state, sizeof(uint32_t), GRND_NONBLOCK);
    //seed->state = time(NULL) % UINT32_MAX;
    return;
}
