#include "./build/libs/util.h"
#define MSG_FILE "/home/user/Desktop/arch/Ass1/Flush-Reload/build/msg.txt"
#define MAX_MSG_SIZE 500

/*
 * For a clock length of config->interval,
 * - Sends a bit 1 to the receiver by repeatedly flushing the address
 * - Sends a bit 0 by doing nothing
 */

// Function to convert a character to a binary string
void char_to_binary(char c, char *binary) {
    for (int i = 7; i >= 0; i--) {
        binary[7 - i] = ((c >> i) & 1) ? '1' : '0';
    }
    binary[8] = '\0';  // Null-terminate the string
}

// Function to send a bit (1 or 0) to the receiver by flushing or doing nothing
void send_bit(bool one, struct config *config) {
    CYCLES start_t = cc_sync();
    if (one) {
        // Repeatedly flush addr to indicate a 1
        ADDR_PTR addr = config->addr;
        while ((get_time() - start_t) < config->interval) {
            clflush(addr);
        }    
    } else {
        // Do Nothing to indicate a 0
        while (get_time() - start_t < config->interval) {}
    }
}

int main(int argc, char **argv) {
    // Open the file and read the message
    FILE *fp = fopen(MSG_FILE, "r");
    char message[MAX_MSG_SIZE];
    int msg_size = 0;
    char c;
    while ((c = fgetc(fp)) != EOF && msg_size < MAX_MSG_SIZE - 1) {
        message[msg_size++] = c;
    }
    message[msg_size] = '\0';  // Null terminate the string
    fclose(fp);

    // Initialize config and local variables
    struct config config;
    init_config(&config, argc, argv);

    bool sequence[8] = {1, 0, 1, 0, 1, 0, 1, 1};  // Message start sequence

    // Loop over the message characters
    for (int i = 0; i < msg_size; i++) {
        char text_buf[2] = {message[i], '\0'};  // Convert char to string (unnecessary but good practice)
        
        // Convert the character to a binary string
        char msg[9];  // 8 bits + null terminator
        char_to_binary(message[i], msg);  // Convert char to binary string

        // Send the '10101011' bit sequence to indicate a message is going to be sent
        for (int i = 0; i < 8; i++) {
            send_bit(sequence[i], &config);
        }

        // Send the message bit by bit
        size_t msg_len = strlen(msg);
        for (size_t ind = 0; ind < msg_len; ind++) {
            send_bit(msg[ind] == '1', &config);  // Send 1 or 0 based on the binary representation
        }
    }
	for (int i = 0; i < 8; i++) {
            send_bit(sequence[i], &config);
        }

	bool end_sequence[8] = {1, 1, 1, 1, 1, 1, 1, 0}; // End sequence

    // Send the end sequence to signal termination
	    for (int i = 0; i < 8; i++) {
        send_bit(end_sequence[i], &config);
    }
	
    printf("Sender finished\n");
    return 0;
}
