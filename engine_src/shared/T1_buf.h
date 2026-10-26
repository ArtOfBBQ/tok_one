#ifndef T1_BUF_H
#define T1_BUF_H

#include "T1_stdint.h"
#include "T1_types_public.h"

/*
GPU buffers for the client's shaders. A T1Buf is the index of its slot, the
platform layer keeps the memory itself (see T1_os_gpu_buf_new()).
*/
#define T1_BUFS_CAP 256

void T1_buf_init(void);

T1Buf T1_buf_new(u64 size, char ** const fatal_error);
void  T1_buf_write(T1Buf buf, u64 offset, const void * data, u64 size);
void  T1_buf_delete(T1Buf buf);

u32 T1_buf_read_start(T1Buf buf, u64 offset, u64 size);
u8  T1_buf_read_finish(u32 read, void * recip);

#endif // T1_BUF_H
