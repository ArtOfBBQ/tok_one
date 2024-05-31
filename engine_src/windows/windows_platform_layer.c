#include "platform_layer.h"

static bool32_t cached = 0;
static LARGE_INTEGER cached_performance_frequency;

uint64_t
platform_get_current_time_microsecs(void)
{
    if (!cached) {
        BOOL result = QueryPerformanceFrequency(
          /* [out] LARGE_INTEGER *lpFrequency: */
            &cached_performance_frequency);
        assert(result > 0);
        cached = true;
        assert(cached_performance_frequency.QuadPart > 0);
    }
    
    LARGE_INTEGER current;
    assert(
        QueryPerformanceCounter(
            /* [out] LARGE_INTEGER *lpPerformanceCount: */
                &current));
    
    return
        ((uint64_t)current.QuadPart * 1000000) /
            (uint64_t)cached_performance_frequency.QuadPart;
}

uint32_t platform_init_mutex_and_return_id(void) {
    // TODO: implement me!
    return 0;
}

bool32_t platform_mutex_trylock(const uint32_t mutex_id) {
    // TODO: implement me!
    return true;
}

void platform_mutex_lock(const uint32_t mutex_id) {
    // TODO: implement me!
}

int32_t platform_mutex_unlock(const uint32_t mutex_id) {
    // TODO: implement me!
    return 0;
}

uint32_t platform_get_directory_separator_size(void) {
    return 1;
}

void platform_get_directory_separator(char * recipient) {
    recipient[0] = '\\';
    recipient[1] = '\0';
    return;
}

void platform_get_resources_path(
    char * recipient,
    const uint32_t recipient_size)
{
    // TODO: implement resources path instead of hardcoding
    strcpy_capped(
        recipient,
        recipient_size,
        "C:\\users\\osora\\documents\\github\\tok_one\\build\\"
        "windows\\tok_one");
}

void platform_get_writables_path(
    char * recipient,
    const uint32_t recipient_size)
{
    // TODO: implement writables path instead of hardcoding
    strcpy_capped(
        recipient,
        recipient_size,
        "C:\\users\\osora\\documents\\github\\tok_one\\build\\"
        "windows\\writables");
}

bool32_t platform_file_exists(const char * filepath) {
    DWORD dwAttrib = GetFileAttributes(filepath);
    
    return (
        dwAttrib != INVALID_FILE_ATTRIBUTES && 
        !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

void platform_delete_file(const char * filepath) {
    // TODO: implement me!
}

void platform_write_file(
    const char * filepath_destination,
    const char * output,
    const uint32_t output_size,
    bool32_t * good)
{
    // TODO: implement me!
}

void platform_read_file(
    const char * filepath,
    FileBuffer * out_preallocatedbuffer)
{
    HANDLE file_handle = CreateFileA(
        /* [in]           LPCSTR                lpFileName: */
            filepath,
        /* [in]           DWORD                 dwDesiredAccess: */
            GENERIC_READ,
        /* [in]           DWORD                 dwShareMode: */
            FILE_SHARE_READ,
        /* [in, optional] LPSECURITY_ATTRIBUTES lpSecurityAttributes: */
            NULL,
        /* [in]           DWORD                 dwCreationDisposition: */
            OPEN_EXISTING,
        /* [in]           DWORD                 dwFlagsAndAttributes: */
            FILE_ATTRIBUTE_NORMAL,
        /* [in, optional] HANDLE                hTemplateFile: */
            NULL);
    
    if (file_handle == INVALID_HANDLE_VALUE) {
        out_preallocatedbuffer->size_without_terminator = 0;
        out_preallocatedbuffer->good = true;
        return;
    }

    DWORD bytes_to_read = platform_get_filesize(filepath);

    if (
        bytes_to_read < 1 ||
        bytes_to_read > out_preallocatedbuffer->size_without_terminator)
    {
        out_preallocatedbuffer->size_without_terminator = 0;
        out_preallocatedbuffer->good = true;
        return;
    }
    
    BOOL read = ReadFile(
        /* [in]            HANDLE     hFile: */
            file_handle,
        /* [out]           LPVOID     lpBuffer: */
            out_preallocatedbuffer->contents,
        /* [in]            DWORD      nNumberOfBytesToRead: */
            bytes_to_read,
        /* [out, optional] LPDWORD    lpNumberOfBytesRead: */
            NULL,
        /* [in, out, optiona LPOVERLAPPED lpOverlapped: */
            NULL);
    
    log_assert(CloseHandle(file_handle));
    out_preallocatedbuffer->good = true;
}

uint64_t platform_get_filesize(const char * filepath) {
    
    HANDLE file_handle = CreateFile(
        filepath,
        GENERIC_READ, 
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING, 
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    
    if (file_handle==INVALID_HANDLE_VALUE) {
        return -1; // could call GetLastError to find out more
    }
    
    LARGE_INTEGER size;
    if (!GetFileSizeEx(file_handle, &size)) {
        CloseHandle(file_handle);
        return -1; // could call GetLastError to find out more
    }
    
    CloseHandle(file_handle);
    return size.QuadPart;
}

void platform_close_application(void) {
    application_running = false;
}

void platform_open_folder_in_window_if_possible(
    const char * folderpath)
{
    // TODO: implement me!
}

void * platform_malloc_unaligned_block(
    const uint64_t size)
{
    return VirtualAlloc(
        /* [in, optional] LPVOID lpAddress: */
            0,
        /* [in]           SIZE_T dwSize: */
            size,
        /* [in]           DWORD  flAllocationType: */
            MEM_RESERVE | MEM_COMMIT,
        /* [in]           DWORD  flProtect: */
            PAGE_READWRITE);
}

