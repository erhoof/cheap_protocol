#ifndef _CRC32_H_
#define _CRC32_H_

// CRC-32/ISO-HDLC

typedef uint32_t crc32_t;
crc32_t crc32(const uint8_t *data, size_t length);

#endif // _CRC32_H_