#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "bit_stream.h"

/* アラインメント */
#define BITSTREAM_ALIGNMENT                 16
/* 読みモードオープン */
#define BITSTREAM_FLAGS_FILEOPENMODE_READ   (1 << 0)
/* 書きモードオープン */
#define BITSTREAM_FLAGS_FILEOPENMODE_WRITE  (1 << 1)
/* メモリはワーク渡しか？（1:ワーク渡し, 0:mallocで自前確保） */
#define BITSTREAM_FLAGS_MEMORYALLOC_BYWORK  (1 << 2)

/* 下位n_bitsを取得 */
/* 補足）((1 << n_bits) - 1)は下位の数値だけ取り出すマスクになる */
#define BITSTREAM_GETLOWERBITS(n_bits, val) ((val) & ((1 << n_bits) - 1))

/* ビットストリーム構造体 */
struct BitStream {
  FILE*   fp;         /* ファイル構造体           */
  uint8_t flags;      /* 内部状態フラグ           */
  uint8_t bit_buffer; /* 内部ビット入出力バッファ */
  int8_t  bit_count;  /* 内部ビット入出力カウント */
  void*   work_ptr;   /* ワーク領域先頭ポインタ   */
};

/* ワークサイズの取得 */
int32_t BitStream_CalculateWorkSize(void)
{
  return (sizeof(struct BitStream) + BITSTREAM_ALIGNMENT);
}

/* ビットストリームのオープン */
struct BitStream* BitStream_Open(const char* filepath,
    const char* mode, void *work, int32_t work_size)
{
  struct BitStream* stream;
  FILE*             tmp_fp;
  int8_t            is_malloc_by_work;
  uint8_t*          work_ptr = (uint8_t *)work;

  /* 引数チェック */
  if (mode == NULL
      || ((work != NULL)
          && (work_size < BitStream_CalculateWorkSize()))){
    return NULL;
  }

  /* ワーク渡しか否か？ */
  if ((work == NULL) && (work_size == 0)) {
    is_malloc_by_work = 0;
    work_ptr = (uint8_t *)malloc(BitStream_CalculateWorkSize());
    if (work_ptr == NULL) {
      return NULL;
    }
  } else {
    is_malloc_by_work = 1;
  }

  /* アラインメント切り上げ */
  work_ptr  = (uint8_t *)(((uintptr_t)work_ptr + (BITSTREAM_ALIGNMENT-1)) & ~(uintptr_t)(BITSTREAM_ALIGNMENT-1));

  /* 構造体の配置 */
  stream            = (struct BitStream *)work_ptr;
  work_ptr          += sizeof(struct BitStream);
  stream->work_ptr  = work;
  stream->flags     = 0;

  /* モードの1文字目でオープンモードを確定
   * 内部カウンタもモードに合わせて設定 */
  switch (mode[0]) {
    case 'r':
      stream->flags     |= BITSTREAM_FLAGS_FILEOPENMODE_READ;
      stream->bit_count = 0;
      break;
    case 'w':
      stream->flags     |= BITSTREAM_FLAGS_FILEOPENMODE_WRITE;
      stream->bit_count = 8;
      break;
    default:
      return NULL;
  }

  /* ファイルオープン */
  tmp_fp = fopen(filepath, mode);
  if (tmp_fp == NULL) {
    return NULL;
  }
  stream->fp = tmp_fp;

  /* メモリアロケート方法を記録 */
  if (is_malloc_by_work == 1) {
    stream->flags |= BITSTREAM_FLAGS_MEMORYALLOC_BYWORK;
  }

  /* 内部状態初期化 */
  fseek(stream->fp, SEEK_SET, 0);
  stream->bit_buffer  = 0;

  return stream;
}

/* ビットストリームのクローズ */
void BitStream_Close(struct BitStream* stream)
{
  /* 引数チェック */
  if (stream == NULL) {
    return;
  }

  /* バッファに余ったビットを強制出力 */
  if (stream->flags & BITSTREAM_FLAGS_FILEOPENMODE_WRITE) {
    BitStream_PutBits(stream, 7, 0);
  }

  /* ファイルハンドルクローズ */
  fclose(stream->fp);

  /* 必要ならばメモリ解放 */
  if (!(stream->flags & BITSTREAM_FLAGS_MEMORYALLOC_BYWORK)) {
    free(stream->work_ptr);
  }
}

/* 1bit出力 */
int32_t BitStream_PutBit(struct BitStream* stream, uint8_t bit)
{
  /* 引数チェック */
  if (stream == NULL) {
    return -1;
  }

  /* 書き込みモードでない場合は即時リターン */
  if (!(stream->flags & BITSTREAM_FLAGS_FILEOPENMODE_WRITE)) {
    return -2;
  }

  /* バイト出力するまでのカウントを減らす */
  stream->bit_count--;

  /* ビット出力バッファに値を格納 */
  if (bit != 0) {
    stream->bit_buffer |= (1 << stream->bit_count);
  }

  /* バッファ出力・更新 */
  if (stream->bit_count == 0) {
    if (fputc(stream->bit_buffer, stream->fp) == EOF) {
      return -3;
    }
    stream->bit_buffer = 0;
    stream->bit_count  = 8;
  }

  return 0;
}

/*
 * valの右側（下位）n_bits 出力（最大64bit出力可能）
 * BitStream_PutBits(stream, 3, 6);は次と同じ:
 * BitStream_PutBit(stream, 1); BitStream_PutBit(stream, 1); BitStream_PutBit(stream, 0); 
 */
int32_t BitStream_PutBits(struct BitStream* stream, uint16_t n_bits, uint64_t val)
{
  /* 引数チェック */
  if (stream == NULL) {
    return -1;
  }

  /* 書き込みモードでない場合は即時リターン */
  if (!(stream->flags & BITSTREAM_FLAGS_FILEOPENMODE_WRITE)) {
    return -2;
  }

  /* 出力可能な最大ビット数を越えている */
  if (n_bits > sizeof(uint64_t) * 8) {
    return -3;
  }

  /* valの上位ビットから順次出力
   * 初回ループでは端数（出力に必要なビット数）分を埋め出力
   * 2回目以降は8bit単位で出力 */
  while (n_bits >= stream->bit_count) {
    n_bits              -= stream->bit_count;
    stream->bit_buffer  |= BITSTREAM_GETLOWERBITS(stream->bit_count, val >> n_bits);
    if (fputc(stream->bit_buffer, stream->fp) == EOF) {
      return -4;
    }
    stream->bit_buffer  = 0;
    stream->bit_count   = 8;
  }

  /* 端数ビットの処理:
   * 残った分をバッファの上位ビットにセット */
  stream->bit_count   -= n_bits;
  stream->bit_buffer  |= BITSTREAM_GETLOWERBITS(n_bits, val) << stream->bit_count;

  return 0;
}

/* 1bit取得 */
int32_t BitStream_GetBit(struct BitStream* stream)
{
  int32_t ch;

  /* 引数チェック */
  if (stream == NULL) {
    return -1;
  }

  /* 読み込みモードでない場合は即時リターン */
  if (!(stream->flags & BITSTREAM_FLAGS_FILEOPENMODE_READ)) {
    return -2;
  }

  /* 入力ビットカウントを1減らし、バッファの対象ビットを出力 */
  stream->bit_count--;
  if (stream->bit_count >= 0) {
    return (stream->bit_buffer >> stream->bit_count) & 1;
  }

  /* 1バイト読み込みとエラー処理 */
  if ((ch = getc(stream->fp)) == EOF) {
    if (feof(stream->fp)) {
      /* ファイル終端に達した */
      return -3;
    } else {
      /* それ以外のエラー */
      return -4;
    }
  }

  /* カウンタとバッファの更新 */
  stream->bit_count   = 7;
  stream->bit_buffer  = ch;

  /* 取得したバッファの最上位ビットを出力 */
  return (stream->bit_buffer >> 7) & 1;
}

/* n_bits 取得（最大64bit）し、その値を右詰めして出力 */
int32_t BitStream_GetBits(struct BitStream* stream, uint16_t n_bits, uint64_t *val)
{
  int32_t  ch;
  uint64_t tmp = 0;

  /* 引数チェック */
  if (stream == NULL || val == NULL) {
    return -1;
  }

  /* 読み込みモードでない場合は即時リターン */
  if (!(stream->flags & BITSTREAM_FLAGS_FILEOPENMODE_READ)) {
    return -2;
  }

  /* 入力可能な最大ビット数を越えている */
  if (n_bits > sizeof(uint64_t) * 8) {
    return -3;
  }

  /* ファイル終端に達している */
  if (feof(stream->fp)) {
    return -4;
  }

  /* 最上位ビットからデータを埋めていく
   * 初回ループではtmpの上位ビットにセット
   * 2回目以降は8bit単位で入力しtmpにセット */
  while (n_bits > stream->bit_count) {
    n_bits  -= stream->bit_count;
    tmp     |= BITSTREAM_GETLOWERBITS(stream->bit_count, stream->bit_buffer) << n_bits;
    /* 1バイト読み込みとエラー処理 */
    if ((ch = getc(stream->fp)) == EOF) {
      if (feof(stream->fp)) {
        /* 途中でファイル終端に達していたら、ループを抜ける */
        break;
      } else {
        /* それ以外のエラー */
        return -5;
      }
    }
    stream->bit_buffer  = ch;
    stream->bit_count   = 8;
  }

  /* 端数ビットの処理 
   * 残ったビット分をtmpの最上位ビットにセット */
  stream->bit_count -= n_bits;
  tmp               |= BITSTREAM_GETLOWERBITS(n_bits, stream->bit_buffer >> stream->bit_count);

  /* 正常終了 */
  *val = tmp;
  return 0;
}
