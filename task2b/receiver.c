#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <x86intrin.h>  // For rdtscp(), clflush()
#include <unistd.h>
#include <sched.h>
#include <string.h>

#define LLC_SIZE (8 * 1024 * 1024)  // 8MB LLC
#define LINE_SIZE 64
#define NUM_LINES (LLC_SIZE / LINE_SIZE)
#define THRESHOLD 300  // Adjust based on system timing
#define DELAY 5000  // Must match sender's delay
#include "cacheutils.h"
#include "utils.h"
uint8_t *probe_buffer;

// Measure access time

int measure_access_time(void *addr) {
    size_t time = rdtsc();
    maccess(addr);
    return rdtsc() - time;
}
// Probe function
int probe() {
    uint64_t total_time = 0;
    
    for (size_t i = 0; i < NUM_LINES; i++) {
        total_time += measure_access_time(&probe_buffer[i * LINE_SIZE]);
    }
    
    return total_time / NUM_LINES;
}

// Function to receive bits
void receive_message(int bits) {
    printf("[Receiver] Listening...\n");
    
    for (int i = 0; i < bits; i++) {
        usleep(DELAY);  // Sync with sender
        int access_time = probe();
        
        int received_bit = (access_time > THRESHOLD) ? 1 : 0;
        printf("%d", received_bit);
        fflush(stdout);
    }
    printf("\n");
}

int main() {
    // Allocate extra memory to ensure proper alignment
    void *raw_memory = malloc(LLC_SIZE + LINE_SIZE);
    if (!raw_memory) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    // Align the pointer manually
    probe_buffer = (uint8_t *)(((uintptr_t)raw_memory + LINE_SIZE - 1) & ~(LINE_SIZE - 1));

    // Ensure memory is initialized

    // Check alignment
    if ((uintptr_t)probe_buffer % LINE_SIZE != 0) {
        fprintf(stderr, "Memory is not properly aligned!\n");
        free(raw_memory);
        exit(EXIT_FAILURE);
    }

    printf("Aligned memory at %p\n", probe_buffer);

    receive_message(6);  // Expecting 6 bits ("101010")

    free(raw_memory);  // Free original allocated block
    return 0;
}
