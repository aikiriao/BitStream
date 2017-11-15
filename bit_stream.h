#ifndef _BIT_STREAM_H_INCLUDED_
#define _BIT_STREAM_H_INCLUDED_

#include <stdint.h>

/* ビットストリーム構造体 */
struct BitStream;

/* ビットストリーム構造体生成に必要なワークサイズ計算 */
int32_t BitStream_CalculateWorkSize(void);

/* ビットストリームのオープン */
struct BitStream* BitStream_Open(const char* filepath, 
    const char *mode, void *work, int32_t work_size);

/* ビットストリームのクローズ */
void BitStream_Close(struct BitStream* stream);

/* 1bit出力 */
int32_t BitStream_PutBit(struct BitStream* stream, uint8_t bit);

/*
 * valの右側（下位）n_bits 出力（最大64bit出力可能）
 * BitStream_PutBits(stream, 3, 6);は次と同じ:
 * BitStream_PutBit(stream, 1); BitStream_PutBit(stream, 1); BitStream_PutBit(stream, 0); 
 */
int32_t BitStream_PutBits(struct BitStream* stream, uint16_t n_bits, uint64_t val);

/* 1bit取得 */
int32_t BitStream_GetBit(struct BitStream* stream);

/* n_bits 取得（最大64bit）し、その値を右詰めして出力 */
int32_t BitStream_GetBits(struct BitStream* stream, uint16_t n_bits, uint64_t *val);

#endif /* _BIT_STREAM_H_INCLUDED_ */
