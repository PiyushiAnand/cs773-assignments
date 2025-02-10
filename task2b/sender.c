#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <x86intrin.h>  // For rdtscp(), clflush()
#include <unistd.h>
#include <sched.h>

#define LLC_SIZE (8 * 1024 * 1024)  // 8MB LLC (adjust based on system)
#define LINE_SIZE 64
#define NUM_LINES (LLC_SIZE / LINE_SIZE)
#define DELAY 5000  // Microseconds

uint8_t *eviction_buffer;

// Prime function (fills LLC)
void prime() {
    for (size_t i = 0; i < NUM_LINES; i++) {
        eviction_buffer[i * LINE_SIZE] = i;
    }
}

// Function to send bits
void send_bit(int bit) {
    if (bit == 1) {
        prime();  // Fill LLC (Evict Receiver's cache)
    }
    usleep(DELAY);  // Control timing
}

int main() {
    // Set CPU affinity to avoid noise
    // cpu_set_t set;
    // CPU_ZERO(&set);
    // CPU_SET(0, &set);  // Bind to CPU 0
    // sched_setaffinity(0, sizeof(set), &set);
    
    // Allocate large eviction buffer
    eviction_buffer = aligned_alloc(LINE_SIZE, LLC_SIZE);
    if (!eviction_buffer) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    char *message = "101010";  // Example binary message

    printf("[Sender] Sending message: %s\n", message);
    
    for (size_t i = 0; message[i] != '\0'; i++) {
        send_bit(message[i] - '0');
    }

    free(eviction_buffer);
    return 0;
}
