#include "csapp.h"
#include <openssl/sha.h>
#include <stdbool.h>

#define MAX_CMD_LEN 100

// States
#define FTP_OK "0"

typedef struct
{
  size_t         blk_size; //< size of a block that divide the file
  size_t         size;     //< file size
  size_t         req_size; //< final size of the file
  unsigned char* data;     //< mapped file address
  int            fd;       //< file descriptor
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
  SF_READ,
  SF_READ_WRITE,
} Seg_File_Mode;

/**
 * @brief initialize the sf structure
 * 
 * @param sf the struct to init
 * @param file_name the name of file
 * @param req_size the final file size, a 0 value means file size
 * @param sfm open mode
 * @param blk_size block size
 * 
 * @note must have a corresponding call to sf_destroy
 * @see sf_destroy
 */
void sf_init(Seg_File* sf, char const* file_name, size_t req_size, Seg_File_Mode sfm,
             size_t blk_size);

size_t sf_nb_blk(Seg_File* sf);

/**
 * @brief get the block no no
 * resizing the file if necessary
 * 
 * @param sf
 * @param no block to fetch
 * @return Block the mapped file block
 */
Block sf_get_blk(Seg_File* sf, size_t no);

/**
 * @brief compute the sha512 sum of the block blk
 * 
 * @param blk 
 * @param sum where the sum will be written
 */
void sf_blk_sum(Block blk, sha512_sum* sum);

/**
 * @brief must be called for a corresponding sf_init
 * 
 * @param sf 
 */
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
