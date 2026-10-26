#include "T1_buf.h"

#include "T1_std.h"
#include "T1_mem.h"
#include "T1_assert.h"
#include "T1_os.h"

typedef struct {
    u64 size;
    b8  used;
} T1BufSlot;

static T1BufSlot * T1_bufs = NULL;

void T1_buf_init(void) {
    T1_bufs = T1_mem_malloc_unmanaged(sizeof(T1BufSlot) * T1_BUFS_CAP);
    T1_std_memset(T1_bufs, 0, sizeof(T1BufSlot) * T1_BUFS_CAP);
}

T1Buf T1_buf_new(u64 size, char ** const fatal_error) {
    if (*fatal_error != 0) { return T1_BUF_NONE; }
    
    T1_assert(size > 0);
    
    u32 i = 0;
    while (i < T1_BUFS_CAP && T1_bufs[i].used) { i++; }
    if (i == T1_BUFS_CAP) {
        *fatal_error = "T1_buf_new(): all T1_BUFS_CAP buffers are in use";
        return T1_BUF_NONE;
    }
    
    u8 good = 0;
    T1_os_gpu_buf_new(i, size, &good);
    if (!good) {
        *fatal_error = "T1_buf_new(): the GPU couldn't allocate the buffer";
        return T1_BUF_NONE;
    }
    
    T1_bufs[i].size = size;
    T1_bufs[i].used = 1;
    
    return (T1Buf)i;
}

void T1_buf_write(T1Buf buf, u64 offset, const void * data, u64 size) {
    T1_assert(buf < T1_BUFS_CAP && T1_bufs[buf].used);
    if (buf >= T1_BUFS_CAP || !T1_bufs[buf].used) { return; }
    T1_assert(data != NULL);
    T1_assert(size > 0);
    T1_assert(offset + size <= T1_bufs[buf].size);
    if (size == 0 || offset + size > T1_bufs[buf].size) { return; }
    
    T1_os_gpu_buf_write(buf, offset, data, size);
}

u32 T1_buf_read_start(T1Buf buf, u64 offset, u64 size) {
    T1_assert(buf < T1_BUFS_CAP && T1_bufs[buf].used);
    if (buf >= T1_BUFS_CAP || !T1_bufs[buf].used) { return T1_BUF_READ_NONE; }
    T1_assert(size > 0);
    T1_assert(offset + size <= T1_bufs[buf].size);
    if (size == 0 || offset + size > T1_bufs[buf].size) {
        return T1_BUF_READ_NONE;
    }
    
    return T1_os_gpu_buf_read_start(buf, offset, size);
}

u8 T1_buf_read_finish(u32 read, void * recip) {
    T1_assert(read < T1_BUF_READS_CAP);
    T1_assert(recip != NULL);
    if (read >= T1_BUF_READS_CAP || recip == NULL) { return 0; }
    
    return T1_os_gpu_buf_read_finish(read, recip);
}

void T1_buf_delete(T1Buf buf) {
    T1_assert(buf < T1_BUFS_CAP && T1_bufs[buf].used);
    if (buf >= T1_BUFS_CAP || !T1_bufs[buf].used) { return; }
    
    T1_os_gpu_buf_delete(buf);
    T1_bufs[buf].used = 0;
    T1_bufs[buf].size = 0;
}
