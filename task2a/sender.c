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
#define ACK_FILE_1 "/dev/shm/ack1"
#define ACK_FILE_2 "/dev/shm/ack2"
#define ITERATIONS 2000
#define THRESHOLD 500 //Adjust based on calibration
#define SYNC_FLAG_OFFSET 256 * 64

int measure_access_time(void *addr) {
    size_t time = rdtsc();
    maccess(addr);
    return rdtsc() - time;
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
    flush(sync_line1);
    int time1 = measure_access_time(sync_line2);
    int time2 = measure_access_time(sync_line1);
    if(time1 > THRESHOLD && time2 > THRESHOLD){
      break;
    }
  }
}


void send_char(char c, char *mapped) {
    for (int i = 0; i < ITERATIONS; i++) {
        for (int j = 0; j < 256; j++) {
            flush((void *)(mapped + j * 64)); // Ensure mapped is valid
        }
        maccess((void *)(mapped + (unsigned char)c * 64));  // Force access
    }
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
    void *mapped = align_shared_mmap(256 * 64, 64,SHARED_FILE);  // Ensure the correct address type
    if (mapped == NULL) {
        perror("Memory mapping failed");
        return 1;
    }
   
    char *sync_line1 = mapped+256*64;
    char *sync_line2 = mapped+256*64+64;
    maccess(sync_line1);
    maccess(sync_line2);
    synchronise(sync_line1,sync_line2);
    send_char('\x02',mapped);
    for(int i=0;i<msg_size;i++){
        maccess(sync_line1);
        maccess(sync_line2);
        synchronise(sync_line1,sync_line2);
        printf("Sending char %c\n",msg[i]);
        send_char(msg[i],mapped);
    }
    synchronise(sync_line1,sync_line2);
    send_char(4,mapped);
    munmap(sync_line1, 64);
    munmap(sync_line2, 64);
    munmap(mapped, 256 * 64);
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
