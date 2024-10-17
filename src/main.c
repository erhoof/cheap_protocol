#include "essentials.h"
#include "crc32.h"

#define IN_FILENAME "data_in.txt"
#define OUT_FILENAME "data_out.txt"

#define MSG_HEADER "mess="
#define MASK_HEADER "mask="

// Type (1 byte) | Length (1 byte) | Data (2 - 253 bytes) | CRC32 (4 bytes)
// sorted for memory aligning, as data len is unknown, we do not use __packed__

typedef struct {
    uint8_t type;
    uint8_t length;
    crc32_t crc32;
    uint8_t *data;
} Message;

int main(int argc, char *argv[]) {
    FILE *inFile = fopen(IN_FILENAME, "r");
    if(!inFile) {
        perror("Failed to open input file, name: " IN_FILENAME);
        return EXIT_FAILURE;
    }

    char msgHeader[sizeof(MSG_HEADER)];

    // Reading Messages (one by one)
    // -1 as line doesn't contain '\0'
    while(fread(msgHeader, sizeof(msgHeader[0]), sizeof(msgHeader) - 1, inFile)) {
        msgHeader[sizeof(MSG_HEADER) - 1] = '\0';
        if(strncmp(msgHeader, MSG_HEADER, sizeof(MSG_HEADER))) {
            perror("Mismatched file format");
            return EXIT_FAILURE;
        }

        Message msg;

        // Type
        {
            char hexValue[2]; // ex. FF
            int rc = fread(hexValue, sizeof(hexValue[0]), sizeof(hexValue), inFile);
            if(sizeof(hexValue) != rc) {
                perror("Unexpected file length");
                return EXIT_FAILURE;
            }

            rc = hexToBinary(&msg.type, sizeof(msg.type), hexValue, sizeof(hexValue));
            if(rc <= 0) {
                perror("Conversion error");
                return EXIT_FAILURE;
            }
        }

        // Length
        {
            char hexValue[2]; // ex. FF
            int rc = fread(hexValue, sizeof(hexValue[0]), sizeof(hexValue), inFile);
            if(sizeof(hexValue) != rc) {
                perror("Unexpected file length");
                return EXIT_FAILURE;
            }

            rc = hexToBinary(&msg.length, sizeof(msg.length), hexValue, sizeof(hexValue));
            if(rc <= 0) {
                perror("Conversion error");
                return EXIT_FAILURE;
            }

            // TODO: add checks
            // As payload is DATA + CRC32
            msg.length = msg.length - sizeof(msg.crc32);
        }

        // Payload
        {
            const int lenInHEX = (msg.length) * 2;

            char *dataHexString = malloc(lenInHEX);
            if(!dataHexString) {
                perror("Memory allocation error");
                return EXIT_FAILURE;
            }

            printf("Allocated %p of %d bytes for message data (in HEX)\n", dataHexString, lenInHEX);

            msg.data = malloc(msg.length);
            if(!msg.data) {
                perror("Memory allocation error");
                return EXIT_FAILURE;
            }

            printf("Allocated %p of %u bytes for message data\n", msg.data, msg.length);

            int rc = fread(dataHexString, sizeof(dataHexString[0]), lenInHEX, inFile);
            if(lenInHEX != rc) {
                perror("Mismatched file format");
                return EXIT_FAILURE; // no need to free memory, closing anyway
            }

            rc = hexToBinary(msg.data, msg.length, dataHexString, lenInHEX);
            if(rc <= 0) {
                perror("Conversion error");
                return EXIT_FAILURE;
            }

            free(dataHexString);
        }

        // CRC32 (I know, that CRC32 is a part of Payload, but its easier to read this way)
        {
            char hexValue[sizeof(crc32_t) * 2]; // ex. AABBCCDD
            int rc = fread(hexValue, sizeof(hexValue[0]), sizeof(hexValue), inFile);
            if(sizeof(hexValue) != rc) {
                perror("Unexpected file length");
                return EXIT_FAILURE;
            }

            rc = hexToBinary(&msg.crc32, sizeof(msg.crc32), hexValue, sizeof(hexValue));
            if(rc <= 0) {
                perror("Conversion error");
                return EXIT_FAILURE;
            }
            msg.crc32 = htonl(msg.crc32);
        }

        printf("Got new message:\n");
        printf("  Type: %d\n", msg.type);
        printf("  Payload length: %d\n", msg.length + sizeof(crc32_t));
        printf("  Data length: %zu\n", msg.length);
        printf("  Data pointer: %p\n", msg.data);
        printf("  CRC32: %08X\n", msg.crc32);
        printf("  Dump:  ");
        for (size_t i = 0; i < msg.length; i++) {
            printf("%02X ", msg.data[i]);
        }
        printf("\n");

        // Required mask to apply
        uint32_t mask;
        {
            // 'mask=' header
            {
                char maskHeader[sizeof(MASK_HEADER)];
                int rc = fread(maskHeader, sizeof(maskHeader[0]), sizeof(maskHeader) - 1, inFile);
                if(rc <= 0) {
                    perror("Unexpected end of file");
                    return EXIT_FAILURE;
                }

                maskHeader[sizeof(MASK_HEADER) - 1] = '\0';
                if(strncmp(maskHeader, MASK_HEADER, sizeof(MASK_HEADER))) {
                    perror("Mismatched file format");
                    return EXIT_FAILURE;
                }
            }

            // Mask itself (CRC32 in HEX)
            {
                char hexValue[sizeof(crc32_t) * 2]; // ex. AABBCCDD
                int rc = fread(hexValue, sizeof(hexValue[0]), sizeof(hexValue), inFile);
                if(sizeof(hexValue) != rc) {
                    perror("Unexpected file length");
                    return EXIT_FAILURE;
                }

                rc = hexToBinary(&mask, sizeof(mask), hexValue, sizeof(hexValue));
                if(rc <= 0) {
                    perror("Conversion error");
                    return EXIT_FAILURE;
                }
                mask = htonl(mask);

                printf("Got new mask:\n");
                printf("  CRC32: %08X\n", mask);
            }
        }

        // Check message CRC32
        {
            crc32_t checksum = crc32(msg.data, msg.length);
            printf("Checking package CRC32:\n");
            printf("  CRC32: %08X\n", checksum);

            if(checksum != msg.crc32) {
                perror("Checksum of message is invalid");
                return EXIT_FAILURE;
            } else {
                printf("Checksum of message is correct!\n");
            }
        }

        // Zero padding
        // TODO: move 4 somewhere
        size_t padding = msg.length % 4;
        if(padding) {
            printf("Padding is required, adding %zu more bytes\n", (4 - padding));

            msg.length = msg.length + (4 - padding);
            msg.data = realloc(msg.data, msg.length);
            if(!msg.data) {
                perror("Allocation error");
                return EXIT_FAILURE;
            }
            printf("Reallocated data of message, current pointer: %p\n", msg.data);

            memset(msg.data + msg.length - padding, 0, padding);
        }

        // Apply mask
        {
            printf("Applying mask: \n");
            for (size_t i = 1; i < msg.length / 4; i++) {
                uint32_t value = htonl(((uint32_t *)msg.data)[i]);
                printf(" Before: %08X\n", value);
                value &= mask;
                ((uint32_t *)msg.data)[i] = htonl(value);
                printf(" After: %08X\n", value);
            }

            printf("  Dump (after mask):  ");
            for (size_t i = 0; i < msg.length; i++) {
                printf("%02X ", msg.data[i]);
            }
            printf("\n");
        }
    }

    return 0;
}