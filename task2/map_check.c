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

#define ALIGNMENT 64    // Alignment requirement (e.g., 64 bytes)
#define SHARED_FILE "/dev/shm/shared_memory_file"  // Shared memory file path
#define MEMORY_SIZE 256*64  // Example size of the shared memory

// Function to align the shared memory address
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
    // Map and align shared memory
    void *mapped = align_shared_mmap(MEMORY_SIZE, ALIGNMENT);
    if (mapped == NULL) {
        return 1;  // Exit if mmap fails
    }

    // Print the aligned memory address
    printf("Mapped and aligned shared memory address: %p\n", mapped);

    // Example of accessing and writing to the aligned memory
    strcpy((char *)mapped, "Hello, this is shared and aligned memory!");

    // Print the message stored in the aligned shared memory
    printf("Message from aligned shared memory: %s\n", (char *)mapped);

    // Clean up: unmap the shared memory region
    if (munmap(mapped, MEMORY_SIZE + ALIGNMENT - 1) == -1) {
        perror("munmap failed");
        return 1;
    }

    return 0;
}
