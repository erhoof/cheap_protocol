#include "essentials.h"
#include "crc32.h"
#include "message.h"

#define IN_FILENAME "data_in.txt"
#define OUT_FILENAME "data_out.txt"

int main(int argc, char *argv[]) {
    int rc = executeMessageFlow(IN_FILENAME, OUT_FILENAME);
    return rc ? EXIT_FAILURE : EXIT_SUCCESS;
}