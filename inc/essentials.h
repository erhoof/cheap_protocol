#ifndef _ESSENTIALS_H_
#define _ESSENTIALS_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <arpa/inet.h>

unsigned int atoh(const char *hexstr);
int hexToBinary(void *buffer, size_t size, const char *data, size_t length);

#endif // _ESSENTIALS_H_