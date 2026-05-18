// SPDX-License-Identifier: MIT

#ifndef SMAUGT_BENCHMARK_CPUCYCLES_H
#define SMAUGT_BENCHMARK_CPUCYCLES_H

#include <stdint.h>

#if defined(__x86_64__) || defined(__i386__)

#ifdef USE_RDPMC /* Needs echo 2 > /sys/devices/cpu/rdpmc */

static inline uint64_t cpucycles(void) {
    const uint32_t ecx = (1U << 30) + 1;
    uint64_t result;

    __asm__ volatile("rdpmc; shlq $32,%%rdx; orq %%rdx,%%rax"
                     : "=a"(result)
                     : "c"(ecx)
                     : "rdx");

    return result;
}

#else

static inline uint64_t cpucycles(void) {
    uint64_t result;

    __asm__ volatile("rdtsc; shlq $32,%%rdx; orq %%rdx,%%rax"
                     : "=a"(result)
                     :
                     : "%rdx");

    return result;
}

#endif
#elif defined(__aarch64__)

/* Apple Silicon / ARMv8 */
static inline uint64_t cpucycles(void) {
    uint64_t result;
    /* ARM virtual counter */
    __asm__ volatile("mrs %0, cntvct_el0" : "=r"(result));
    return result;
}

#else
#error "cpucycles not supported on this architecture"
#endif

uint64_t cpucycles_overhead(void);

#endif /* !SMAUGT_BENCHMARK_CPUCYCLES_H */
