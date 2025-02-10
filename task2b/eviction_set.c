#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <x86intrin.h> // For rdtsc, clflush
#include <sched.h>     // For setting CPU affinity
#include <unistd.h>

#define CACHE_LINE_SIZE 64
#define PROBE_TRIALS 10000
#define THRESHOLD 200 // Adjust based on system timing

// Measure access time to a given memory address
uint64_t measure_access_time(void *addr) {
    uint64_t start, end;
    volatile uint8_t *p = (volatile uint8_t*)addr;
    
    start = __rdtscp(&end); // Start timing
    (void)*p; // Access the memory
    end = __rdtscp(&start); // End timing
    
    return end - start;
}

// Function to find eviction set
void **find_eviction_set(size_t buffer_size, size_t *eviction_count) {
    uint8_t *buffer = aligned_alloc(CACHE_LINE_SIZE, buffer_size);
    if (!buffer) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }
    
    size_t num_addresses = buffer_size / CACHE_LINE_SIZE;
    void **eviction_set = malloc(num_addresses * sizeof(void*));
    size_t count = 0;
    
    printf("Finding eviction set...\n");
    for (size_t i = 0; i < num_addresses; i++) {
        void *addr = &buffer[i * CACHE_LINE_SIZE];
        _mm_clflush(addr); // Flush from cache
        uint64_t time1 = measure_access_time(addr);
        (void)*(volatile uint8_t*)addr;
        uint64_t time2 = measure_access_time(addr);
        
        if (time2 > THRESHOLD) {
            printf("Eviction candidate: %p (time: %lu cycles)\n", addr, time2);
            eviction_set[count++] = addr;
        }
    }
    *eviction_count = count;
    return eviction_set;
}

int main() {
    size_t buffer_size = 1024 * 1024; // 1MB buffer
    size_t eviction_count = 0;
    
    // Set CPU affinity
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(0, &set); // Bind to CPU 0
    sched_setaffinity(0, sizeof(set), &set);
    
    void **eviction_set = find_eviction_set(buffer_size, &eviction_count);
    
    printf("Eviction set found with %zu addresses.\n", eviction_count);
    free(eviction_set);
    return 0;
}
