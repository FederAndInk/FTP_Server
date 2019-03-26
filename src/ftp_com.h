#include "csapp.h"
#include <openssl/sha.h>

#define MAX_CMD_LEN 100

// States
#define FTP_OK "0"

typedef struct
{
  size_t         blk_size;
  size_t         size;
  unsigned char* data;
  int            fd;
} Seg_File;

typedef struct
{
  unsigned char* data;
  size_t         blk_size;
} Block;

typedef struct
{
  unsigned char sum[SHA512_DIGEST_LENGTH];
} sha512_sum;

typedef enum
{
  SF_WRITE,
  SF_READ,
  SF_READ_WRITE,
} Seg_File_Mode;

void sf_init(Seg_File* sf, char const* file_name, Seg_File_Mode sfm, size_t blk_size);

Block sf_read_blk(Seg_File* sf, size_t no);

void sf_blk_sum(Block blk, sha512_sum* sum);

void sf_write_blk(Seg_File* sf, size_t no, Block blk);

void sf_destroy(Seg_File* sf);

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
