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

#define SHARED_FILE "/dev/shm/covert_channel"
#define ITERATIONS 1000
#define THRESHOLD 200  // Adjust based on calibration

void flush(void *addr) {
    _mm_clflush(addr);
}

void send_char(char c, char *mapped) {
    for (int i = 0; i < ITERATIONS; i++) {
        // Flush all 256 cache lines
        for (int j = 0; j < 256; j++) {
            flush(&mapped[j * 64]); // Assuming 64-byte cache line
        }
        // Access the address corresponding to the character
        *(volatile char *)&mapped[(unsigned char)c * 64];
        usleep(50);  // Delay for synchronization
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

    // Start Transmission with STX (Start of Text)
    send_char('\x02', mapped);
    
    for (size_t i = 0; i < msg_size; i++) {
        send_char(msg[i], mapped);
        usleep(1000);  // Allow receiver time to process
    }

    // End Transmission with ETX (End of Text)
    send_char('\x03', mapped);

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
