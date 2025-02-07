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
#define THRESHOLD 350 // Adjust based on calibration
#define ALIGNMENT 64

int measure_access_time(void *addr) {
    size_t time = rdtsc();
    maccess(addr);
    return rdtsc() - time;
}



char receive_char(char *mapped) {
    int miss_counts[256] = {0};
    for (int i = 0; i < ITERATIONS; i++) {
        //  usleep(1000);  // Prevent excessive contention
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
    // printf("Misses: %d\n", min_miss);
    // printf("Detected: %d\n", detected_char);

    return (char)detected_char;
}

// void send_char(char c, char *mapped) {
//     for (int i = 0; i < ITERATIONS; i++) {
//         for (int j = 0; j < 256; j++) {
//             flush((void *)(mapped + j * 64)); // Ensure mapped is valid
//         }
//         maccess((void *)(mapped + (unsigned char)c * 64));  // Force access
//         // usleep(1000);  // Delay for synchronization
//     }
// }

void *align_shared_mmap(size_t size, size_t alignment, const char *file) {
    // Open shared memory file (create if doesn't exist)
    int fd = open(file, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
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

void synchronise(char *sync_line1, char *sync_line2) {
//   maccess(sync_line1);
//   maccess(sync_line2);
  while(1){
    flush(sync_line2);
    usleep(1000);
    int time1 = measure_access_time(sync_line1);
    int time2 = measure_access_time(sync_line2);
    
    printf("Time1: %d\n", time1);
    printf("Time2: %d\n", time2);
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

    void *mapped = align_shared_mmap(256 * 64, 64, SHARED_FILE);  // Ensure the correct address type

    char *sync_line1 = mapped+256*64;
    char *sync_line2 = mapped+256*64+64;

    printf("Mapped address: %p\n", mapped);
    printf(
        "Shared memory region: %p\n"
        "Size: %d bytes\n",
        mapped, 256 * 64
    );
    
    int received_start = 0;
    int valid = 0;
    int flag = 0;
    
    
    while(1){
        maccess(sync_line1);
        maccess(sync_line2);
        //sleep(1);
        synchronise(sync_line1,sync_line2);
    char c  = receive_char(mapped);
    printf("Received: %c\n", c);
    }
    // int ack_fd1 = open(ACK_FILE_1, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    // if (ack_fd1 < 0) {
    //     perror("File open failed");
    //     return 1;
    // }
    // int ack_fd2 = open(ACK_FILE_2, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    // if (ack_fd2 < 0) {
    //     perror("File open failed");
    //     return 1;
    // }
    // void *ack_mapped1 = align_shared_mmap(256 * 64, 64,ACK_FILE_1);  // Ensure the correct address type
    // void *ack_mapped2 = align_shared_mmap(256 * 64, 64,ACK_FILE_2);  // Ensure the correct address type
    
    
//    while (flag ==0){
//         char received = receive_char((char *)mapped);  // Detect transmitted character
//         // sleep(2);
//         //check if received is valid
//         printf("Received: %c\n", received);
//         send_char(received, (char *)ack_mapped1);  // Send acknowledgment for detected character
//         printf("ACK sent for: %c\n", received);
//         // for(int i=0;i<1000;i++){
//         char ack_received = receive_char((char *)ack_mapped2); 
//         if (ack_received == 255){
//             printf("ACK received for: %c\n",ack_received);
//             flag = 1;
//             break;
//          }
//         // }
//     }
    int next = 0;
    // printf("sending ack\n");
    // for(int i=0;i<2000;i++){
    //     send_char('a', (char *)ack_mapped);
    // }
    printf("started Listening\n");
    munmap(mapped, 256 * 64);
    close(fd);

    // ********** YOUR CODE ENDS HERE **********
    // ********** DO NOT MODIFY THIS LINE **********
    printf("Accuracy (%%): %f\n", check_accuracy(received_msg, received_msg_size) * 100);
    free(received_msg);
}
