// membench.c - simple memory bandwidth and latency benchmark
// Build:  gcc -O2 -o membench membench.c
// Run:    ./membench > results.csv
#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

static double now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1e9 + ts.tv_nsec;
}

// Bandwidth: sequentially read (sum) an array of 'bytes' size, repeated many times
static double bandwidth_gbps(size_t bytes) {
    size_t n = bytes / sizeof(uint64_t);
    uint64_t *a = malloc(bytes);
    if (!a) { perror("malloc"); exit(1); }
    for (size_t i = 0; i < n; i++) a[i] = i;

    size_t total_bytes = (size_t)1 << 30;            // read ~1 GB in total per size
    size_t reps = total_bytes / bytes;
    if (reps < 1) reps = 1;

    volatile uint64_t sink = 0;
    double t0 = now_ns();
    for (size_t r = 0; r < reps; r++) {
        uint64_t s = 0;
        for (size_t i = 0; i < n; i++) s += a[i];
        sink += s;
    }
    double t1 = now_ns();
    (void)sink;
    free(a);
    return (double)(bytes * reps) / (t1 - t0);       // bytes per ns == GB/s
}

// Latency: pointer chasing through a random cycle so each load depends on the last
static double latency_ns(size_t bytes) {
    size_t n = bytes / sizeof(size_t);
    if (n < 2) n = 2;
    size_t *next = malloc(n * sizeof(size_t));
    size_t *perm = malloc(n * sizeof(size_t));
    if (!next || !perm) { perror("malloc"); exit(1); }
    for (size_t i = 0; i < n; i++) perm[i] = i;
    srand(12345);
    for (size_t i = n - 1; i > 0; i--) {             // Sattolo shuffle -> one single cycle
        size_t j = (size_t)rand() % i;
        size_t t = perm[i]; perm[i] = perm[j]; perm[j] = t;
    }
    for (size_t i = 0; i < n; i++) next[perm[i]] = perm[(i + 1) % n];

    size_t steps = 20000000;
    size_t p = 0;
    double t0 = now_ns();
    for (size_t i = 0; i < steps; i++) p = next[p];
    double t1 = now_ns();
    volatile size_t sink = p; (void)sink;
    free(next); free(perm);
    return (t1 - t0) / (double)steps;
}

int main(void) {
    printf("test,size_bytes,value,unit\n");
    for (size_t sz = 4096; sz <= ((size_t)256 << 20); sz *= 2) {
        printf("bandwidth,%zu,%.3f,GB/s\n", sz, bandwidth_gbps(sz));
        printf("latency,%zu,%.3f,ns\n", sz, latency_ns(sz));
        fflush(stdout);
    }
    return 0;
}
