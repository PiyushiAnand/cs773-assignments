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
#include "./cacheutils.h"

#define SHARED_FILE "/dev/shm/newchnel"
// #define ACK_FILE_1 "/dev/shm/ack1"
// #define ACK_FILE_2 "/dev/shm/ack2"
#define ITERATIONS 2000
#define SYNC_FLAG_OFFSET 256 * 64
#define THRESHOLD 500// Adjust based on calibration
#define ALIGNMENT 64

int measure_access_time(void *addr) {
    size_t time = rdtsc();
    maccess(addr);
    return rdtsc() - time;
}
char receive_char(char *mapped) {
    int miss_counts[256] = {0};
    for (int i = 0; i < ITERATIONS; i++) {
        for (int j = 0; j < 256; j++) {
            int access_time = measure_access_time(&mapped[j * 64]);
            if (access_time > THRESHOLD) {
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
    return (char)detected_char;
}


void *align_shared_mmap(size_t size, size_t alignment, const char *file) {
    int fd = open(file, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        perror("Failed to open shared memory file");
        return NULL;
    }
    if (ftruncate(fd, size + alignment - 1) == -1) {
        perror("Failed to set size of shared memory");
        close(fd);
        return NULL;
    }
    void *addr = mmap(NULL, size + alignment - 1, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        perror("mmap failed");
        close(fd);
        return NULL;
    }
    uintptr_t aligned_addr = ((uintptr_t)addr + alignment - 1) & ~(alignment - 1);
    close(fd);
    return (void *)aligned_addr;
}

void synchronise(char *sync_line1, char *sync_line2) {
  while(1){
    flush(sync_line2);
    int time1 = measure_access_time(sync_line1);
    int time2 = measure_access_time(sync_line2);
    if(time1 > THRESHOLD && time2 > THRESHOLD){
      break;
    }
    
  }
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

    // Map shared memory
    void *mapped = align_shared_mmap(256 * 64, 64, SHARED_FILE);  // Ensure the correct address type
    // Set up synchronization
    char *sync_line1 = mapped+256*64;
    char *sync_line2 = mapped+256*64+64;
    int started = 0;
    int ended = 0;
    while(ended == 0){
        maccess(sync_line1);
        maccess(sync_line2);
        synchronise(sync_line1,sync_line2);
        char c  = receive_char(mapped);
        if(started == 1){
            if(c == 4){
                ended = 1;
            }
            else{
                printf("Received: %c\n",c);
                //check of it is a letter
                if((c>=65&&c<=90)||(c>=97&&c<=122)){
                
                received_msg[received_msg_size] = c;
                received_msg_size++;
                }
            }
        }
        if(c == '\x02'){
            started = 1;
        }
    }

    munmap(sync_line1, 64);
    munmap(sync_line2, 64);
    munmap(mapped, 256 * 64);
    close(fd);
    printf("Received message: %s\n", received_msg);
    // ********** YOUR CODE ENDS HERE **********
    // ********** DO NOT MODIFY THIS LINE **********
    printf("Accuracy (%%): %f\n", check_accuracy(received_msg, received_msg_size) * 100);
    free(received_msg);
}
