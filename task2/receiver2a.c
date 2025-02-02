#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <x86intrin.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include "utils.h"

#define SHARED_FILE "/dev/shm/covert_channel"
#define ITERATIONS 1000
#define THRESHOLD 200  // Adjust based on calibration

int measure_access_time(void *addr) {
    uint64_t start, end;
    volatile char *ptr = (char *)addr;
    start = __rdtscp(&start);
    *ptr;
    end = __rdtscp(&start);
    return (int)(end - start);
}

char receive_char(char *mapped) {
    int hit_counts[256] = {0};

    for (int i = 0; i < ITERATIONS; i++) {
        for (int j = 0; j < 256; j++) {
            int access_time = measure_access_time(&mapped[j * 64]);
            if (access_time < THRESHOLD) {
                hit_counts[j]++;
            }
        }
        usleep(50);  // Prevent excessive contention
    }

    // Find the character with the highest cache hits
    int max_hits = 0, detected_char = '?';
    for (int j = 0; j < 256; j++) {
        if (hit_counts[j] > max_hits) {
            max_hits = hit_counts[j];
            detected_char = j;
        }
    }

    return (char)detected_char;
}

int main() {
    // ********** DO NOT MODIFY THIS SECTION **********
    char* received_msg = (char *)malloc(MAX_MSG_SIZE);
    int received_msg_size = 0;
    // **********************************************

    // ********** YOUR CODE STARTS HERE **********

    int fd = open(SHARED_FILE, O_RDWR);
    if (fd < 0) {
        perror("File open failed");
        return 1;
    }

    char *mapped = mmap(NULL, 256 * 64, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mapped == MAP_FAILED) {
        perror("mmap failed");
        return 1;
    }

    printf("Waiting for sender...\n");
    // sleep(2);
    while (1) {
        char c = receive_char(mapped);
        printf("Received: %c\n", c);

        // Check for start and end markers
        if (c == '\x02') {
            received_msg_size = 0; // Reset buffer
        } else if (c == '\x03') {
            break; // End transmission
        } else {
            received_msg[received_msg_size++] = c;
        }
    }

    received_msg[received_msg_size] = '\0'; // Null-terminate the string

    munmap(mapped, 256 * 64);
    close(fd);

    // ********** YOUR CODE ENDS HERE **********
    // ********** DO NOT MODIFY THIS LINE **********
    printf("Accuracy (%%): %f\n", check_accuracy(received_msg, received_msg_size) * 100);
    free(received_msg);
}
