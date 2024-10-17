#ifndef _ERRORS_H_
#define _ERRORS_H_

#define ERR_SYS -1000
#define ERR_SYS_FILE_READ -1001
#define ERR_SYS_FILE_LENGTH -1002

#define ERR_MSG -2000
#define ERR_MSG_FORMAT -2001
#define ERR_MSG_GENERAL -2002
#define ERR_MSG_CONVERSION -2003
#define ERR_MSG_CHECKSUM -2004

#define ERR_APP -3000
#define ERR_APP_WRONG_ARGUMENTS -3001
#define ERR_APP_MEMORY_ALLOCATION -3002

#define ERR_TYPE_LEN 1000
#define ERR_IS_TYPE(type, value) ((type >= value) && ((type - ERR_TYPE_LEN) < value))

#endif // _ERRORS_H_