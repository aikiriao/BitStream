#include <stdio.h>
#include <stdlib.h>
#include "bit_stream.h"

int main(void)
{
  int32_t ret;
  uint64_t buf;
  struct BitStream *w_strm, *r_strm;

  fprintf(stdout, "Work Size: %d \n", BitStream_CalculateWorkSize());

  w_strm = BitStream_Open("test.bin", "wb", NULL, 0);
  if (w_strm == NULL) {
    return -1;
  }

  if (BitStream_PutBit(w_strm, 1) != 0) return -1;
  if (BitStream_PutBit(w_strm, 1) != 0) return -1;
  if (BitStream_PutBit(w_strm, 1) != 0) return -1;
  if (BitStream_PutBit(w_strm, 1) != 0) return -1;
  if (BitStream_PutBit(w_strm, 0) != 0) return -1;
  if (BitStream_PutBit(w_strm, 0) != 0) return -1;
  if (BitStream_PutBit(w_strm, 0) != 0) return -1;
  if (BitStream_PutBit(w_strm, 0) != 0) return -1;

  if (BitStream_PutBits(w_strm, 16, 0x55AA) != 0) return -1;

  BitStream_Close(w_strm);

  r_strm = BitStream_Open("test.bin", "rb", NULL, 0);
  if (r_strm == NULL) {
    return -1;
  }

  if ((ret = BitStream_GetBits(r_strm, 4, &buf)) >= 0) {
    fprintf(stdout, "0x%llx \n", buf);
  }

  if ((ret = BitStream_GetBits(r_strm, 4, &buf)) >= 0) {
    fprintf(stdout, "0x%llx \n", buf);
  }

  while ((ret = BitStream_GetBit(r_strm)) >= 0) {
    fprintf(stdout, "%d \n", ret);
  }

  BitStream_Close(r_strm);

  return 0;
}
