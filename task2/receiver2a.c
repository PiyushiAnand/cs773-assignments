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

#define SHARED_FILE "/dev/shm/mychanel"
#define ITERATIONS 1000
#define SYNC_FLAG_OFFSET 256 * 64
// #define THRESHOLD 245  // Adjust based on calibration
#define ALIGNMENT 64
int calibrate_threshold(char *mapped) {
    int sum = 0;
    for (int i = 0; i < 1000; i++) {
        sum += measure_access_time(&mapped[0]);
    }
    return sum / 1000 + 10;  // Add margin
}

int measure_access_time(void *addr) {
    uint32_t aux;
    uint64_t start, end;
    volatile char *ptr = (char *)addr;
    
    asm volatile ("lfence");  // Ensure previous operations are completed
    start = __rdtscp(&aux);  
    asm volatile ("lfence");  // Prevent speculative execution
    *ptr;                    
    asm volatile ("lfence");  // Ensure load is completed before measuring
    end = __rdtscp(&aux);
    
    return (int)(end - start);
}

char receive_char(char *mapped) {
    int hit_counts[256] = {0};
    int threshold = calibrate_threshold(mapped);
    //print if the sync flag is true or not
    printf(mapped[SYNC_FLAG_OFFSET] == 0 ? "Waiting for sender...\n" : "Sender ready\n");
    while (mapped[SYNC_FLAG_OFFSET] == 0) {
        usleep(10);
    }
    for (int i = 0; i < ITERATIONS; i++) {
        for (int j = 0; j < 256; j++) {
            int access_time = measure_access_time(&mapped[j * 64]);
            // printf("%d\n",access_time);
            if (access_time < threshold) {
                // printf("Hit: %d\n", j);
                hit_counts[j]++;
            }
        }
        usleep(50);  // Prevent excessive contention
    }


    // Find the character with the highest cache hits
    int max_hits = 0, detected_char = '?';
    for (int j = 0; j < 256; j++) {
        if (hit_counts[j] > max_hits) {
            // printf("Hit: %d\n", j);
            max_hits = hit_counts[j];
            detected_char = j;
        }
    }

    return (char)detected_char;
}

void *align_shared_mmap(size_t size, size_t alignment) {
    // Open shared memory file (create if doesn't exist)
    int fd = open(SHARED_FILE, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        perror("Failed to open shared memory file");
        return NULL;
    }

    // Set the size of the shared memory region
    if (ftruncate(fd, size + alignment - 1) == -1) {
        perror("Failed to set size of shared memory");
        close(fd);
        return NULL;
    }

    // Map the shared memory with extra space for alignment
    void *addr = mmap(NULL, size + alignment - 1, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        perror("mmap failed");
        close(fd);
        return NULL;
    }

    // Align the address to the nearest multiple of the alignment
    uintptr_t aligned_addr = ((uintptr_t)addr + alignment - 1) & ~(alignment - 1);

    // Close the file descriptor as it's no longer needed
    close(fd);

    // Return the aligned address
    return (void *)aligned_addr;
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

    void *mapped = align_shared_mmap(256 * 64, 64);
    // char *mapped = mmap(NULL, 256 * 64, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    
    // if (mapped == MAP_FAILED) {
    //     perror("mmap failed");
    //     return 1;
    // }
    // posix_memalign((void**)&mapped, 64, 256 * 64);
    printf("Mapped address: %p\n", mapped);

    printf(
        "Shared memory region: %p\n"
        "Size: %d bytes\n",
        mapped, 256 * 64
    );
    //
    int THRESHOLD = calibrate_threshold(mapped);
    printf("Waiting for sender...\n");
    
    int received_start = 0;
    while (1) {
        char c = receive_char(mapped);
        printf("Received: %c\n", c);

        // Check for start and end markers
        if (c == '\x02') {
            received_msg_size = 0; // Reset buffer
            received_start = 1;
        } else if (c == '\x03') {
            break; // End transmission
        } 
        if(received_start)received_msg[received_msg_size++] = c;
        
        usleep(1000);  // Allow sender time to process
    }

    received_msg[received_msg_size] = '\0'; // Null-terminate the string
    printf("Received message: %s\n", received_msg);
    munmap(mapped, 256 * 64);
    close(fd);

    // ********** YOUR CODE ENDS HERE **********
    // ********** DO NOT MODIFY THIS LINE **********
    printf("Accuracy (%%): %f\n", check_accuracy(received_msg, received_msg_size) * 100);
    free(received_msg);
}
