#include "essentials.h"

unsigned int atoh(const char *hexstr) {
    return (unsigned int)strtol(hexstr, NULL, 16);
}

int hexToBinary(void *buffer, size_t size, const char *data, size_t length) {
    if (length % 2 != 0) {
        // Hex string must have an even length
        return -1; // Error: Invalid hex string
    }

    size_t requiredSize = length / 2;
    if (requiredSize > size) {
        // Not enough space in the buffer
        return -2; // Error: Buffer too small
    }

    unsigned char *byteBuffer = (unsigned char *)buffer;
    for (size_t i = 0; i < requiredSize; i++) {
        // Convert each pair of hex characters to a byte
        char hexPair[3] = {data[i * 2], data[i * 2 + 1], '\0'};
        byteBuffer[i] = (unsigned char)strtol(hexPair, NULL, 16);
    }

    return requiredSize; // Return the number of bytes written to the buffer
}
