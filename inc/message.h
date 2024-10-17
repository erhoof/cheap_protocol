#ifndef _MESSAGE_H_
#define _MESSAGE_H_

#define MSG_HEADER "mess="
#define MASK_HEADER "mask="

// Type (1 byte) | Length (1 byte) | Data (2 - 253 bytes) | CRC32 (4 bytes)
// sorted for memory aligning, as data len is unknown, we do not use __packed__
typedef struct {
    uint8_t type;
    uint8_t payloadLength;
    uint8_t dataLength;
    crc32_t crc32;
    uint8_t *data;
} Message;

int executeMessageFlow(const char *inFilename, const char *outFilename);

#endif