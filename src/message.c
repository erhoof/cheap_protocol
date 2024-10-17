#include "essentials.h"
#include "errors.h"
#include "crc32.h"

#include "message.h"

void handleError(FILE *outFile, int error) {
    if(ERR_IS_TYPE(ERR_SYS, error)) {
        fprintf(outFile, "[System Error] ");
    } else if(ERR_IS_TYPE(ERR_MSG, error)) {
        fprintf(outFile, "[Message Error] ");
    } else if(ERR_IS_TYPE(ERR_APP, error)) {
        fprintf(outFile, "[Application Error] ");
    } else {
        fprintf(outFile, "[Unknown Error] ");
    }

    switch(error) {
        case ERR_SYS_FILE_READ:
            fprintf(outFile, "File read error");
            break;
        case ERR_SYS_FILE_LENGTH:
            fprintf(outFile, "File length error");
            break;
        case ERR_SYS_FILE_EOF:
            fprintf(outFile, "End of file");
            break;
        case ERR_MSG_FORMAT:
            fprintf(outFile, "Message format is wrong");
            break;
        case ERR_MSG_GENERAL:
            fprintf(outFile, "General message error");
            break;
        case ERR_MSG_CONVERSION:
            fprintf(outFile, "Message conversion error");
            break;
        case ERR_MSG_CHECKSUM:
            fprintf(outFile, "Message checksum check failure");
            break;
        case ERR_APP_WRONG_ARGUMENTS:
            fprintf(outFile, "Wrong arguments (null pointer possibility)");
            break;
        case ERR_APP_MEMORY_ALLOCATION:
            fprintf(outFile, "Memory allocation failure");
            break;
        default:
            fprintf(outFile, "General error, rc: %d", error);
            break;
    }

    fprintf(outFile, "\n");
}

int readType(FILE *inFile, Message *msg) {
    char hexValue[2]; // ex. FF
    int rc = fread(hexValue, sizeof(hexValue[0]), sizeof(hexValue), inFile);
    if(sizeof(hexValue) != rc) {
        return ERR_SYS_FILE_LENGTH;
    }

    rc = hexToBinary(&msg->type, sizeof(msg->type), hexValue, sizeof(hexValue));
    if(rc <= 0) {
        return ERR_MSG_CONVERSION;
    }

    return 0;
}

int readLength(FILE *inFile, Message *msg) {
    char hexValue[2]; // ex. FF
    int rc = fread(hexValue, sizeof(hexValue[0]), sizeof(hexValue), inFile);
    if(sizeof(hexValue) != rc) {
        return ERR_SYS_FILE_LENGTH;
    }

    rc = hexToBinary(&msg->payloadLength, sizeof(msg->payloadLength), hexValue, sizeof(hexValue));
    if(rc <= 0) {
        return ERR_MSG_CONVERSION;
    }

    msg->dataLength = msg->payloadLength - sizeof(msg->crc32);
    return 0;
}

int readPayload(FILE *inFile, Message *msg) {
    const int lenInHEX = (msg->dataLength) * 2;

    char *dataHexString = malloc(lenInHEX);
    if(!dataHexString) {
        return ERR_APP_MEMORY_ALLOCATION;
    }

    printf("Allocated %p of %d bytes for message data (in HEX)\n", dataHexString, lenInHEX);

    msg->data = malloc(msg->dataLength);
    if(!msg->data) {
        free(dataHexString);
        return ERR_APP_MEMORY_ALLOCATION;
    }

    printf("Allocated %p of %u bytes for message data\n", msg->data, msg->dataLength);

    int rc = fread(dataHexString, sizeof(dataHexString[0]), lenInHEX, inFile);
    if(lenInHEX != rc) {
        free(dataHexString);
        free(msg->data);
        return ERR_SYS_FILE_LENGTH;
    }

    rc = hexToBinary(msg->data, msg->dataLength, dataHexString, lenInHEX);
    if(rc <= 0) {
        return ERR_MSG_CONVERSION;
    }

    free(dataHexString);
    return 0;
}

int readCRC32(FILE *inFile, Message *msg) {
    char hexValue[sizeof(crc32_t) * 2]; // ex. AABBCCDD
    int rc = fread(hexValue, sizeof(hexValue[0]), sizeof(hexValue), inFile);
    if(sizeof(hexValue) != rc) {
        return ERR_SYS_FILE_LENGTH;
    }

    rc = hexToBinary(&msg->crc32, sizeof(msg->crc32), hexValue, sizeof(hexValue));
    if(rc <= 0) {
        return ERR_MSG_CONVERSION;
    }

    msg->crc32 = htonl(msg->crc32);
    return 0;
}

void dumpMessage(Message *msg, FILE *outFile, const char *title, int count) {
    fprintf(outFile, "[%d] %s:\n", count, title);
    fprintf(outFile, "  Type: %d\n", msg->type);
    fprintf(outFile, "  Payload length: %d\n", msg->payloadLength);
    fprintf(outFile, "  Data length: %zu\n", msg->dataLength);
    fprintf(outFile, "  Data pointer: %p\n", msg->data);
    fprintf(outFile, "  CRC32: %08X\n", msg->crc32);
    fprintf(outFile, "  Dump:  ");
    for(size_t i = 0; i < msg->dataLength; i++) {
        fprintf(outFile, "%02X ", msg->data[i]);
    }
    fprintf(outFile, "\n");
}

int readMask(FILE *inFile, uint32_t *mask) {
    // 'mask=' header
    {
        char maskHeader[sizeof(MASK_HEADER)];
        int rc = fread(maskHeader, sizeof(maskHeader[0]), sizeof(maskHeader) - 1, inFile);
        if(rc <= 0) {
            return ERR_SYS_FILE_LENGTH;
        }

        maskHeader[sizeof(MASK_HEADER) - 1] = '\0';
        if(strncmp(maskHeader, MASK_HEADER, sizeof(MASK_HEADER))) {
            return ERR_MSG_CONVERSION;
        }
    }

    // Mask itself (uint32_t in HEX)
    {
        char hexValue[sizeof(uint32_t) * 2]; // ex. AABBCCDD in HEX
        int rc = fread(hexValue, sizeof(hexValue[0]), sizeof(hexValue), inFile);
        if(sizeof(hexValue) != rc) {
            return ERR_SYS_FILE_LENGTH;
        }

        rc = hexToBinary(mask, sizeof(*mask), hexValue, sizeof(hexValue));
        if(rc <= 0) {
            return ERR_MSG_CONVERSION;
        }
        *mask = htonl(*mask);

        printf("Got new mask: %08X\n", *mask);
    }

    return 0;
}

int checkCRC32(Message *msg) {
    crc32_t checksum = crc32(msg->data, msg->dataLength);
    printf("Checking package CRC32:\n");
    printf("  Message: %08X\n", msg->crc32);
    printf("  Calculated: %08X\n", checksum);

    if(checksum != msg->crc32) {
        return ERR_MSG_CHECKSUM;
    }

    printf("Checksum passed\n");
    return 0;
}

int addPadding(Message *msg) {
    const int MSG_PADDING = 4;

    size_t requredPadding = msg->dataLength % MSG_PADDING;
    if(requredPadding) {
        printf("Padding is required, adding %zu more bytes\n",
            (MSG_PADDING - requredPadding));

        msg->dataLength = msg->dataLength + (MSG_PADDING - requredPadding);

        uint8_t *newPtr = realloc(msg->data, msg->dataLength);
        if(!newPtr) {
            return ERR_APP_MEMORY_ALLOCATION;
        }

        msg->data = newPtr;

        printf("Reallocated data of message, current pointer: %p\n", msg->data);
        memset(msg->data + msg->dataLength - requredPadding, 0, requredPadding);
    }

    return 0;
}

void applyMask(Message *msg, uint32_t mask) {
    printf("Applying mask: \n");
    for(size_t i = 1; i < msg->dataLength / 4; i += 2) {
        uint32_t value = htonl(((uint32_t *)msg->data)[i]);
        printf(" Before: %08X\n", value);
        value &= mask;
        ((uint32_t *)msg->data)[i] = htonl(value);
        printf(" After: %08X\n", value);
    }

    // Update CRC32
    msg->crc32 = crc32(msg->data, msg->dataLength);
}

int checkMessage(FILE *inFile) {
    char msgHeader[sizeof(MSG_HEADER)];

    // check for MSG_HEADER size as \r\n may appear at the end
    int rc = fread(msgHeader, sizeof(msgHeader[0]), sizeof(msgHeader) - 1, inFile) >= (sizeof(MSG_HEADER) - 1);
    if(!rc) {
        return feof(inFile) ? ERR_SYS_FILE_EOF : ERR_SYS_FILE_LENGTH;
    }

    // -1 as line doesn't contain '\0'
    msgHeader[sizeof(MSG_HEADER) - 1] = '\0';
    if(strncmp(msgHeader, MSG_HEADER, sizeof(MSG_HEADER))) {
        return ERR_MSG_FORMAT;
    }

    return 0;
}

int parseMessages(FILE *inFile, FILE *outFile) {
    int count = 0;

    // Reading Messages (one by one)
    int rc;
    while(!(rc = checkMessage(inFile))) {
        Message msg;
        rc = readType(inFile, &msg);
        if(rc) {
            break;
        }

        rc = readLength(inFile, &msg);
        if(rc) {
            break;
        }

        rc = readPayload(inFile, &msg);
        if(rc) {
            break;
        }

        rc = readCRC32(inFile, &msg);
        if(rc) {
            free(msg.data);
            break;
        }

        dumpMessage(&msg, outFile, "Original message", count);

        uint32_t mask;
        rc = readMask(inFile, &mask);
        if(rc) {
            free(msg.data);
            break;
        }

        // End of message reading
        rc = checkCRC32(&msg);
        if(rc) {
            fprintf(outFile, "Checksum verification failed");
            free(msg.data);
            break;
        }

        rc = addPadding(&msg);
        if(rc) {
            free(msg.data);
            break;
        }

        applyMask(&msg, mask);

        dumpMessage(&msg, outFile, "Updated message", count);

        free(msg.data);

        count++;
    }

    // as EOF only occures before message read, always occures as file have \r\n at the end (or just \n)
    if(rc && (ERR_SYS_FILE_EOF != rc)) {
        handleError(outFile, rc);
    }

    return rc;
}

int executeMessageFlow(const char *inFilename, const char *outFilename) {
    if(!inFilename || !outFilename) {
        perror("Wrong arguments for Input / Output files");
        return ERR_APP_WRONG_ARGUMENTS;
    }

    FILE *inFile = fopen(inFilename, "r");
    if(!inFile) {
        perror("Failed to open input file");
        return ERR_SYS_FILE_READ;
    }

    FILE *outFile = fopen(outFilename, "w");
    if(!outFile) {
        perror("Failed to open output file");
        fclose(inFile);
        return ERR_SYS_FILE_READ;
    }

    int rc = parseMessages(inFile, outFile);
    if(!rc) {
        printf("All messages are parsed succesfully");
    }

    fclose(outFile);
    fclose(inFile);
    return rc ? ERR_MSG_GENERAL : 0;
}
