#include "essentials.h"
#include "crc32.h"

uint32_t CRC32_FORWARD[256];
uint32_t REVERSE[256];

unsigned int reflect32(const unsigned int x) {
    unsigned int bits = 0;
    unsigned int mask = x;

    for(int i = 0; i < sizeof(x) * 8; i++) {
        bits <<= 1;
        if(mask & 1) {
            bits |= 1;
        }
        mask >>= 1;
    }

    return bits;
}

crc32_t crc32(const uint8_t *data, size_t length) {
    static bool initialized = false;
    if(!initialized) {
        for(unsigned short byte = 0; byte < 256; byte++) {
            unsigned int crc = (unsigned int)byte << 24;
            for(char bit = 0; bit < 8; bit++) {
                if(crc & (1L << 31)) {
                    crc = (crc << 1) ^ 0x04C11DB7; // forward Form
                } else {
                    crc = (crc << 1);
                }
            }

            CRC32_FORWARD[byte] = crc;
        }

        for(int byte = 0; byte < 256; byte++) {
            REVERSE[byte] = (reflect32(byte) >> 24) & 0xFF;
        }

        initialized = true;
    }

    unsigned int crc = 0xFFFFFFFF;
    while(length-- > 0) {
        crc = CRC32_FORWARD[((crc >> 24) ^ REVERSE[*data++]) & 0xFF] ^ (crc << 8);
    }
    return reflect32(~crc);
}