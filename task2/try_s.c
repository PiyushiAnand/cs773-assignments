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
#include <time.h>
#include "./cacheutils.h"

#define ALIGNMENT 64
#define SHARED_FILE "/dev/shm/newchnel"
#define ACK_FILE "/dev/shm/ack"
#define ITERATIONS 1000
#define THRESHOLD 245  // Adjust based on calibration
#define SYNC_FLAG_OFFSET 256 * 64

// void flush(void *addr) {
//     _mm_clflush(addr);
// }
int measure_access_time(void *addr) {
    size_t time = rdtsc();
    maccess(addr);
    return rdtsc() - time;
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




void send_char(char c, char *mapped) {
    for (int i = 0; i < ITERATIONS; i++) {
        for (int j = 0; j < 256; j++) {
            flush((void *)(mapped + j * 64)); // Ensure mapped is valid
        }
        maccess((void *)(mapped + (unsigned char)c * 64));  // Force access
        // usleep(1000);  // Delay for synchronization
    }
}

char receive_char(char *mapped) {
    int miss_counts[256] = {0};
    for (int i = 0; i < ITERATIONS; i++) {
        // usleep(1000);  // Prevent excessive contention
        for (int j = 0; j < 256; j++) {
            int access_time = measure_access_time(&mapped[j * 64]);
            if (access_time > THRESHOLD) {
                // printf("Hit: %d\n", j);
                miss_counts[j]++;
            }
        }
    }
    int min_miss = INT32_MAX, detected_char = 0;
    for (int j = 0; j < 256; j++) {
        if (miss_counts[j] < min_miss) {
            // printf("Hit: %d\n", j);
            min_miss = miss_counts[j];
            detected_char = j;
        }
    }

    printf("Misses: %d\n", min_miss);
    printf("Detected: %d\n", detected_char);

    return (char)detected_char;
}

void send_letter(void *mapped, void *ack_mapped, char c) {
    int flag = 0;
    int i=0;
    while(flag == 0||i<2000){
        send_char('a', (char *)mapped);
        i++;
        printf("%d\n",i);
        if(i==2000){
             char c = receive_char((char *)ack_mapped);
            while(c != 'a'){
                c = receive_char((char *)ack_mapped);
                if(c == 'a'){
                    flag = 1;
                    break;
                }
            }
        }
    }
   
    printf("STX received\n");
}

int main() {
    // ********** DO NOT MODIFY THIS SECTION **********
    FILE *fp = fopen(MSG_FILE, "r");
    if(fp == NULL){
        printf("Error opening file\n");
        return 1;
    }

    char msg[MAX_MSG_SIZE];
    int msg_size = 0;
    char c;
    while((c = fgetc(fp)) != EOF){
        msg[msg_size++] = c;
    }
    fclose(fp);

    clock_t start = clock();
    // **********************************************

    // ********** YOUR CODE STARTS HERE **********
    
    int fd = open(SHARED_FILE, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        perror("File open failed");
        return 1;
    }

    // Mapping shared memory
    void *mapped = align_shared_mmap(256 * 64, 64);  // Ensure the correct address type
    if (mapped == NULL) {
        perror("Memory mapping failed");
        return 1;
    }
    int ack_fd = open(ACK_FILE, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (ack_fd < 0) {
        perror("File open failed");
        return 1;
    }
    void *ack_mapped = align_shared_mmap(256 * 64, 64);  // Ensure the correct address type
    // Start Transmission with STX (Start of Text)
    
    //wait for ack
    send_letter(mapped, ack_mapped, 'a');

   
    munmap(mapped, 256 * 64 + 63);  // Include alignment padding
    close(fd);

    // ********** YOUR CODE ENDS HERE **********
    // ********** DO NOT MODIFY THIS SECTION **********
    clock_t end = clock();
    double time_taken = ((double)end - start) / CLOCKS_PER_SEC;
    printf("Message sent successfully\n");
    printf("Time taken to send the message: %f\n", time_taken);
    printf("Message size: %d\n", msg_size);
    printf("Bits per second: %f\n", msg_size * 8 / time_taken);
    // **********************************************
}
