#include "./build/libs/util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_BUFFER_LEN 256

// Cache miss latency threshold
int CACHE_MISS_LATENCY = 400;

/*
 * Converts an 8-bit binary string to a character
 */
int binary_to_num(const char *binary) {
    int c = 0;
    for (int i = 0; binary[i] != '\0'; i++) {  // Loop until null terminator
        c = (c << 1) | (binary[i] - '0');
    }
    return c;
}

/*
 * Detects a bit by measuring access time and checking if it's a cache hit/miss
 */
bool detect_bit(struct config *config) {
    int misses = 0, hits = 0;

    // Sync with sender
    CYCLES start_t = cc_sync();
    while ((get_time() - start_t) < config->interval) {
        CYCLES access_time = measure_one_block_access_time(config->addr);

        if (access_time > CACHE_MISS_LATENCY) {
            misses++;
        } else {
            hits++;
        }
    }
    return misses >= hits;
}

int main(int argc, char **argv) {
    // Initialize configuration
    struct config config;
    init_config(&config, argc, argv);

    char msg_ch[9];  // Buffer for a single character (8 bits + null terminator)
    char received_message[MAX_BUFFER_LEN] = {0};  // Full message buffer
    int received_index = 0;

    uint32_t bitSequence = 0;
    uint32_t sequenceMask = ((uint32_t) 1 << 6) - 1;
    uint32_t expSequence = 0b101011;  // The expected start sequence

    uint32_t terminationSequence = 0xFE;  // Termination sequence: 11111110 (in binary)
    uint32_t terminationMask = 0xFF;  // Mask to compare the last 8 bits (termination sequence)

    printf("Listening...\n");
    fflush(stdout);
	int flag = 1;
    while (flag) {
        bool bitReceived = detect_bit(&config);

        // Detect start sequence '101011'
        bitSequence = (bitSequence << 1) | bitReceived;
        if ((bitSequence & sequenceMask) == expSequence) {
            int binary_msg_len = 0;
            
            // Receive the message bit by bit
            for (int i = 0; i < 8; i++) {
                binary_msg_len++;
                msg_ch[i] = detect_bit(&config) ? '1' : '0';
            }
            
            printf("%s\n", msg_ch);

            msg_ch[8] = '\0';  // Null-terminate to form a valid string

            // Check for termination sequence '11111111'
						// Check for termination sequence '11111110'
			if (strcmp(msg_ch, "11111110") == 0) {
				printf("Termination sequence detected. Ending transmission...\n");
				flag = 0;
				break;
			}

            int received_char = binary_to_num(msg_ch);

            // Append received character to the message
            if (received_index < MAX_BUFFER_LEN - 1) {
                received_message[received_index++] = received_char;
                received_message[received_index] = '\0';
            }

            printf("Received Char: %c\n", received_char);
        }
    }

    printf("Final Message: %s\n", received_message);
    printf("Receiver finished\n");
    return 0;
}
