#include "csapp.h"

#define MAX_CMD_LEN 100

// States
#define FTP_OK "0"

/**
 * @brief receive a line whitout keeping the newline charater and replacing it with null '\0'
 * 
 * @param rp 
 * @param str 
 * @param maxlen 
 */
ssize_t receive_line(rio_t* rp, char* str, size_t maxlen);

/**
 * @brief send str adding '\n' at the end
 * 
 * @param rp 
 * @param str
 * @param len 
 */
void send_line(int fd, char const* str);
