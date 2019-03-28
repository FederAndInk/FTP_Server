#include <ftp_com.h>

#define BLK_SIZE 1024

int main(int argc, char const* argv[])
{
  if (argc != 2)
  {
    printf("usage: %s file_name\n", argv[0]);
    exit(1);
  }

  Seg_File sf;
  sf_init(&sf, argv[1], 0, SF_READ, BLK_SIZE);

  printf("nb blk: %zu\n", sf_nb_blk_req(&sf));
  printf("size: %zu\n", sf.size);
  printf("req_size: %zu\n", sf.req_size);

  Block b = sf_get_blk(&sf, sf_nb_blk_req(&sf) - 1);
  printf("size of last block: %zu\n", b.blk_size);

  printf("req_size == (nb_blocks - 1) * blk_size + last_blk_size:\n");
  size_t cSize = (sf_nb_blk_req(&sf) - 1) * sf.blk_size + b.blk_size;
  printf("%zu == %zu: ", sf.req_size, cSize);
  if (sf.size != cSize)
  {
    printf("error\n");
  }
  else
  {
    printf("ok\n");
  }

  sf_destroy(&sf);

  sf_init(&sf, argv[1], cSize + 1, SF_READ_WRITE, BLK_SIZE);
  b = sf_get_blk(&sf, sf_nb_blk_req(&sf) - 1);
  cSize = (sf_nb_blk_req(&sf) - 1) * sf.blk_size + b.blk_size;
  printf("%zu == %zu: ", sf.req_size, cSize);
  if (sf.size != cSize)
  {
    printf("error\n");
  }
  else
  {
    printf("ok\n");
  }

  b.data[b.blk_size - 1] = '0';
  sf_destroy(&sf);

  return 0;
}
