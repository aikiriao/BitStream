#ifndef _BIT_STREAM_H_INCLUDED_
#define _BIT_STREAM_H_INCLUDED_

#include <stdint.h>
#include <stdio.h>

/* BitStream_Seek関数の探索コード */
#define BITSTREAM_SEEK_SET  SEEK_SET
#define BITSTREAM_SEEK_CUR  SEEK_CUR
#define BITSTREAM_SEEK_END  SEEK_END

/* ビットストリーム構造体 */
struct BitStream;

/* ビットストリーム構造体生成に必要なワークサイズ計算 */
int32_t BitStream_CalculateWorkSize(void);

/* ビットストリームのオープン */
struct BitStream* BitStream_Open(const char* filepath, 
    const char *mode, void *work, int32_t work_size);

/* ビットストリームのクローズ */
void BitStream_Close(struct BitStream* stream);

/* シーク(fseek準拠)
 * 注意）バッファをクリアするので副作用がある */
int32_t BitStream_Seek(struct BitStream* stream, uint32_t offset, int32_t wherefrom);

/* 現在位置(ftell)準拠 */
int32_t BitStream_Tell(struct BitStream* stream);

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
