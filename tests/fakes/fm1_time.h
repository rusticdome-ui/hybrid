#ifndef TEST_FM1_TIME_H
#define TEST_FM1_TIME_H
#include <stdint.h>
extern uint32_t test_fm1_ticks;
static inline uint32_t fm1_ticks(void) { return test_fm1_ticks; }
#define FM1_TICKS_PER_US 24u
#endif
