#pragma once

#include "csapp.h"
#include <openssl/sha.h>
#include <stdbool.h>

#define FTP_MAX_CMD_LEN 100
#define FTP_MAX_LINE_SIZE 8192

#define BLK_SIZE (1 << 25)
#define GET_BLK "get_blk"
#define GET_BLK_SUM "get_blk_sum"
#define GET_END "get_end"

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
  unsigned char sum[SHA256_DIGEST_LENGTH];
} Check_Sum;

typedef enum
{
  SF_READ,
  SF_READ_WRITE,
} Seg_File_Mode;

typedef enum
{
  LOG_LV_LOG,
  LOG_LV_INFO,
  LOG_LV_WARNING,
  LOG_LV_ERROR,
} Log_Level;

char const* log_level_str(Log_Level l);

typedef void (*Disp_Fn)(Log_Level ll, char const* format, ...);
extern Disp_Fn fc_disp;
extern bool    fc_show_progress_bar;

bool check_sum_equal(Check_Sum* s1, Check_Sum* s2);

/**
 * @brief initialize the sf structure
 * 
 * @param sf the struct to init
 * @param file_name the name of file
 * @param req_size the final file size, a 0 value means file size
 * @param sfm open mode
 * @param blk_size block size
 * 
 * @return 0 on success, -1 otherwise and errno is set
 * 
 * @note must have a corresponding call to sf_destroy
 * @see sf_destroy
 */
int sf_init(Seg_File* sf, char const* file_name, size_t req_size, Seg_File_Mode sfm,
            size_t blk_size);

/**
 * @brief get the current nb of blocks in the file
 * based on req_size
 * 
 * @param sf the segfile
 * @return the number of blocks contained in the file
 */
size_t sf_nb_blk(Seg_File* sf);

/**
 * @brief get the nb of blocks required in a file
 * based on req_size
 * 
 * @param sf the segfile
 * @return the number of blocks required 
 */
size_t sf_nb_blk_req(Seg_File* sf);

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
 * @brief compute the checksum of the block blk
 * 
 * @param blk 
 * @param sum where the sum will be written
 */
void sf_blk_sum(Block blk, Check_Sum* sum);

/**
 * @brief must be called for a corresponding sf_init
 * 
 * @param sf 
 */
void sf_destroy(Seg_File* sf);

void sf_send_blk(Seg_File* sf, rio_t* rio, size_t no_blk);
void sf_send_blk_sum(Seg_File* sf, rio_t* rio, size_t no_blk);

bool sf_receive_blk(Seg_File* sf, rio_t* rio, size_t no_blk);
bool sf_receive_blk_sum(Seg_File* sf, rio_t* rio, size_t no_blk, Check_Sum* sum);

/**
 * @brief receive a line whitout keeping the newline charater and replacing it with null '\0'
 * 
 * @param rio 
 * @param str 
 * @param maxlen 
 */
ssize_t receive_line(rio_t* rio, char* str, size_t maxlen);

size_t receive_size_t(rio_t* rio);
long   receive_long(rio_t* rio);

/**
 * @brief 
 * protocol:
 * 2. receive ok/err (if file is ready)
 * 3. receive file size
 * 4. receive blk size
 * 5. send commands 
 *   (depending on what we already have and what the user want):
 *   - get_blk \n no
 *   - get_blk_sum \n no
 * 6. send "get_end" end receiving file
 * 
 * @param rio 
 * @param file_name 
 * @return true if file have been received successfully
 * @return false otherwise
 */
bool receive_file(rio_t* rio, char const* file_name);

/**
 * @brief receive result of command
 * 
 * 
 * @param rio 
 * @param com 
 * @return true while result is not finished
 * @return false when end command result is received
 */
bool receive_exec_command(rio_t* rio, char* res, size_t len);

/**
 * @brief send str adding '\n' at the end
 * 
 * @param rio 
 * @param str
 * @param len 
 */
void send_line(rio_t* rio, char const* str);

void send_size_t(rio_t* rio, size_t s);
void send_long(rio_t* rio, long l);

/**
 * @brief 
 * Protocol: 
 * 2. send ok/err (if file is ready)
 * 3. send file size
 * 4. send blk size
 * 5. wait for commands:
 *   - get_blk \n no
 *   - get_blk_sum \n no
 * 6. receiving "get_end" end sending file
 * 
 * @param rio 
 * @param fn 
 * @param blk_size 
 * @return true if file have been sent successfully
 * @return false otherwise
 */
bool send_file(rio_t* rio, char const* fn, size_t blk_size);

void send_exec_command(rio_t* rio, char const* com);
