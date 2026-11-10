#include "T1_os.h"

#include "T1_mem.h"
#include "T1_assert.h"
#include "T1_global.h"
#include "T1_macro_settings.h"

#define T1_THREAD_IDS_CAP 50
typedef struct {
    u64 thread_id_map[T1_THREAD_IDS_CAP];
    u64 thread_id_map_size;
    u32 thread_id_map_mutex_id; // threads register themselves at any time
} T1OSState;

static T1OSState * T1_os_s = NULL;

void T1_os_init(void) {
    T1_os_s = T1_mem_malloc_unmanaged(sizeof(T1OSState));
    T1_std_memset(T1_os_s, 0, sizeof(T1OSState));
    T1_os_s->thread_id_map_mutex_id = T1_os_init_mutex_and_return_id();
}

u64 T1_os_raw_thread_id_to_T1_thread_id(
    u64 raw_in,
    char ** const fatal_error)
{
    if (*fatal_error != 0) { return UINT64_MAX; }
    
    T1_assert(raw_in != 0); // 0 marks a free slot
    // new threads register themselves here, maybe several at once
    T1_os_mutex_lock(T1_os_s->thread_id_map_mutex_id);
    u64 out = UINT64_MAX;
    for (u64 i = 0; i < T1_os_s->thread_id_map_size; i++) {
        if (T1_os_s->thread_id_map[i] == raw_in) {
            out = i;
            break;
        }
    }
    
    // a slot an ended thread gave back (see T1_os_forget_raw_thread_id())
    for (u64 i = 0; out == UINT64_MAX && i < T1_os_s->thread_id_map_size; i++) {
        if (T1_os_s->thread_id_map[i] == 0) {
            T1_os_s->thread_id_map[i] = raw_in;
            out = i;
        }
    }
    
    if (out == UINT64_MAX) {
        if ((T1_os_s->thread_id_map_size + 1) >= T1_THREAD_IDS_CAP) {
            *fatal_error = "T1_os_raw_thread_id_to_T1_thread_id() overflowing T1_THREAD_IDS_CAP";
        } else {
            T1_os_s->thread_id_map[T1_os_s->thread_id_map_size] = raw_in;
            out = T1_os_s->thread_id_map_size;
            T1_os_s->thread_id_map_size += 1;
        }
    }
    T1_os_mutex_unlock(T1_os_s->thread_id_map_mutex_id);
    
    return out;
}

void T1_os_forget_raw_thread_id(u64 raw_in) {
    T1_assert(raw_in != 0); // 0 marks a free slot
    T1_os_mutex_lock(T1_os_s->thread_id_map_mutex_id);
    for (u64 i = 0; i < T1_os_s->thread_id_map_size; i++) {
        if (T1_os_s->thread_id_map[i] == raw_in) {
            T1_os_s->thread_id_map[i] = 0;
            break;
        }
    }
    T1_os_mutex_unlock(T1_os_s->thread_id_map_mutex_id);
}

#define MAX_FILENAME_SIZE  512
#define MAX_SEPARATOR_SIZE   3 // 2 characters and NULL terminator

void T1_os_res_filename_to_pathfile(
    const char * filename,
    char * recipient,
    u32 recip_cap)
{
    T1_assert(filename != NULL);
    T1_assert(recipient != NULL);
    T1_std_memset(recipient, 0, recip_cap);
    
    T1_os_get_res_dir(recipient, recip_cap);
    
    // u32 separator_size = platform_get_directory_separator_size();
    T1_os_get_dir_separator(
        /* recipient: */ recipient + T1_std_strlen(recipient));
    T1_std_strcat_cap(
        recipient,
        recip_cap,
        filename);
    
    T1_assert(recipient[0] != '\0');
}

void T1_os_writable_filename_to_pathfile(
    const char * filename,
    char * recipient,
    u32 recip_cap)
{
    T1_assert(filename != NULL);
    T1_assert(recipient != NULL);
        
    T1_os_get_writables_dir(recipient, recip_cap);
    if (recipient[0] == '\0') {
        return;
    }
    T1_assert(recipient[0] != '\0');
    T1_os_get_dir_separator(
        recipient + T1_std_strlen(recipient));
    T1_std_strcat_cap(
        recipient,
        recip_cap,
        filename);
    
    T1_assert(T1_std_strlen(recipient) < recip_cap);
}

void T1_os_layer_start_window_resize(
    const u64 timestamp)
{
    // The client's objects stay, it decides what to do with them when
    // T1_frame_resized() reports the new size
    T1_global->last_resize_request_us = timestamp;
}

