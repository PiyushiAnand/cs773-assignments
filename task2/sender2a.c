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

#define ALIGNMENT 64
#define SHARED_FILE "/dev/shm/mychanel"
#define ITERATIONS 1000
#define THRESHOLD 245  // Adjust based on calibration
#define SYNC_FLAG_OFFSET 256 * 64

void flush(void *addr) {
    _mm_clflush(addr);
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
    printf("Sending character: %c (0x%02X)\n", c, (unsigned char)c); 
    // printf(mapped[SYNC_FLAG_OFFSET] == 0 ? "Waiting for receiver...\n" : "Receiver ready\n");
    mapped[SYNC_FLAG_OFFSET] = 1;
    for (int i = 0; i < ITERATIONS; i++) {
        // Flush all 256 cache lines
        for (int j = 0; j < 256; j++) {
            flush((char *)mapped + j * 64); // Ensure mapped is valid
        }

        // Perform a read/write to access the intended address
        volatile char dummy = mapped[(unsigned char)c * 64]; // Force access

        usleep(50);  // Delay for synchronization
    }
    mapped[SYNC_FLAG_OFFSET] = 0;
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

    printf("Mapped address: %p\n", mapped);
    printf(
        "Shared memory region: %p\n"
        "Size: %d bytes\n",
        mapped, 256 * 64
    );

    usleep(100000); // Ensure receiver starts

    // Start Transmission with STX (Start of Text)
    send_char('\x02', (char *)mapped);
    
    for (size_t i = 0; i < msg_size; i++) {
        send_char(msg[i], (char *)mapped);
        usleep(10000);  // Allow receiver time to process
    }
    
    // End Transmission with ETX (End of Text)
    send_char('\x03', (char *)mapped);
   
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
