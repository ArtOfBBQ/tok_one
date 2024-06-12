#include "platform_layer.h"

static bool32_t cached = 0;
static LARGE_INTEGER cached_performance_frequency;

static HANDLE mutexes[MUTEXES_SIZE];
static uint32_t next_mutex_id = 0;

#define THREADARGS_QUEUE_SIZE 20
static int32_t thread_args[THREADARGS_QUEUE_SIZE];
static uint32_t thread_args_i = 0;

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
    
    log_assert(next_mutex_id + 1 < MUTEXES_SIZE);
    
    mutexes[next_mutex_id] = CreateMutex( 
        NULL,              // default security attributes
        FALSE,             // initially not owned
        NULL);             // unnamed mutex
    uint32_t return_value = next_mutex_id;
    next_mutex_id += 1;
    
    return return_value;
}

bool32_t platform_mutex_trylock(const uint32_t mutex_id) {
    
    log_assert(mutex_id < MUTEXES_SIZE);
    
    DWORD result = WaitForSingleObject( 
        mutexes[mutex_id],    // handle to mutex
        0);  // don't wait at all 
    
    return result == WAIT_OBJECT_0;
}

void platform_mutex_lock(const uint32_t mutex_id) {
    log_assert(mutex_id < MUTEXES_SIZE);
    
    DWORD result = WaitForSingleObject( 
        mutexes[mutex_id],    // handle to mutex
        INFINITE);  // no time-out interval
    
    log_assert(result == WAIT_OBJECT_0);
}

void platform_mutex_unlock(const uint32_t mutex_id) {
    log_assert(mutex_id < MUTEXES_SIZE);
    
    ReleaseMutex(
        /* [in] HANDLE hMutex: */
            mutexes[mutex_id]);
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
    // if 1st parameter is NULL, this finds the running .exe's root folder
    int bytes = GetModuleFileName(NULL, recipient, recipient_size);
    log_assert(bytes > 0);
    
    PathRemoveFileSpecA(recipient);
}

void platform_get_application_path(
    char * recipient,
    const uint32_t recipient_size)
{
    // if 1st parameter is NULL, this finds the running .exe's root folder
    int bytes = GetModuleFileName(NULL, recipient, recipient_size);
    log_assert(bytes > 0);
    
    PathRemoveFileSpecA(recipient);
}

void platform_get_writables_path(
    char * recipient,
    const uint32_t recipient_size)
{
    //PWSTR path = NULL;
    //
    //HRESULT result = SHGetKnownFolderPath(
    //  /*
    //  [in]           REFKNOWNFOLDERID rfid:      
    //  A reference to the KNOWNFOLDERID that identifies the folder.
    //  */
    //      FOLDERID_ProgramData,
    //  /*
    //  [in]           DWORD            dwFlags:
    //  Flags that specify special retrieval options. This value can be 0;
    //  otherwise, one or more of the KNOWN_FOLDER_FLAG values.
    //  */
    //      0,
    //  /*
    //  [in: optional] HANDLE           hToken:
    //  An access token that represents a particular user. If this
    //  parameter is NULL, which is the most common usage, the function
    //  requests the known folder for the current user.
    //  */
    //      0,
    //  /*
    //  [out]          PWSTR            *ppszPath:
    //  When this method returns, contains the address of a pointer to a
    //  null-terminated Unicode string that specifies the path of the
    //  known folder. The calling process is responsible for freeing this
    //  resource once it is no longer needed by calling CoTaskMemFree,
    //  whether SHGetKnownFolderPath succeeds or not. The returned path
    //  does not include a trailing backslash. For example, "C:\Users" is
    //  returned rather than "C:\Users\".
    //  */
    //      &path);
    //PathAppend(path, APPLICATION_NAME);
    //
    //strcpy_capped(
    //    recipient,
    //    recipient_size,
    //    path);
    //
    //log_assert(result == S_OK);
    
    // if 1st parameter is NULL, this finds the running .exe's root folder
    int bytes = GetModuleFileName(NULL, recipient, recipient_size);
    log_assert(bytes > 0);
    
    PathRemoveFileSpecA(recipient);
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
    const char* filepath,
    FileBuffer* out_preallocatedbuffer)
{
    log_assert(out_preallocatedbuffer->size_without_terminator > 0);

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

    DWORD bytes_to_read = out_preallocatedbuffer->
        size_without_terminator;

    if (bytes_to_read < 1)
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

    out_preallocatedbuffer->contents[out_preallocatedbuffer->size_without_terminator] = '\0';

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

void platform_open_folder_in_window_if_possible(
    const char * folderpath)
{
    // TODO: implement me!
}

void * platform_malloc_unaligned_block(
    const uint64_t size)
{
    void * return_value = VirtualAlloc(
        /* [in, optional] LPVOID lpAddress: */
            0,
        /* [in]           SIZE_T dwSize: */
            size,
        /* [in]           DWORD  flAllocationType: */
            MEM_RESERVE | MEM_COMMIT,
        /* [in]           DWORD  flProtect: */
            PAGE_READWRITE);
    
    log_assert(return_value != NULL);
    
    // virtualalloc returns NULL to signal failure
    if (return_value != NULL) {
        memset(
            return_value,
            0,
            size);
    }
    
    return return_value;
}

void platform_start_thread(
    void (*function_to_run)(int32_t),
    int32_t argument)
{
    thread_args[thread_args_i] = argument;
    
    CreateThread(
      /*
      [in: optional]  LPSECURITY_ATTRIBUTES   lpThreadAttributes:
      A pointer to a SECURITY_ATTRIBUTES structure that determines
      whether the returned handle can be inherited by child processes.
      If lpThreadAttributes is NULL, the handle cannot be inherited. 
      */
          NULL,
      /*
      [in]            SIZE_T                  dwStackSize:
      The initial size of the stack, in bytes. The system rounds this
      value to the nearest page. If this parameter is zero, the new
      thread uses the default size for the executable. For more
      information, see Thread Stack Size.
      */
          0,
      /*
      [in]            LPTHREAD_START_ROUTINE  lpStartAddress:
      A pointer to the application-defined function to be executed by
      the thread. This pointer represents the starting address of the
      thread. For more information on the thread function, see ThreadProc.
      */
          (LPTHREAD_START_ROUTINE)function_to_run,
      /*
      [in: optional]  __drv_aliasesMem LPVOID lpParameter:
      A pointer to a variable to be passed to the thread.
      */
          &thread_args[thread_args_i],
      /*
      [in]            DWORD                   dwCreationFlags:
      The flags that control the creation of the thread.
      Value	Meaning
      0         The thread runs immediately after creation.
      */
          0,
      /*
      [out: optional] LPDWORD                 lpThreadId:
      A pointer to a variable that receives the thread identifier.
      If this parameter is NULL, the thread identifier is not returned.
      */
          NULL);
    
    thread_args_i += 1;
    thread_args_i %= THREADARGS_QUEUE_SIZE;
}

void platform_mkdir_if_not_exist(
    const char * dirname)
{
    BOOL result = CreateDirectoryA(
        /* [in]           LPCSTR                lpPathName: */
            dirname,
        /* [in, optional] LPSECURITY_ATTRIBUTES lpSecurityAttributes: */
            NULL);
    log_assert(result != ERROR_PATH_NOT_FOUND);
}

void platform_copy_file(
    const char * filepath_source,
    const char * filepath_destination)
{
    BOOL result = CopyFile(
      /* [in] LPCTSTR lpExistingFileName: */
          filepath_source,
      /* [in] LPCTSTR lpNewFileName: */
          filepath_destination,
      /* [in] BOOL    bFailIfExists: */
          false);
}

void platform_get_filenames_in(
    const char * directory,
    char filenames[2000][500])
{
    memset(filenames, 0, 2000 * 500);
    
    log_assert(strlen(directory) <= (MAX_PATH - 3));
    
    // Prepare string for use with FindFile functions.  First, copy the
    // string to a buffer, then append '\*' to the directory name.
   
    HANDLE handle = INVALID_HANDLE_VALUE; 
    char fulldirectory[MAX_PATH];
    strcpy_capped(fulldirectory, MAX_PATH, directory);
    strcat_capped(fulldirectory, MAX_PATH, "\\*");
    
    // Find the first file in the directory.
    WIN32_FIND_DATA ffd;
    handle = FindFirstFile(fulldirectory, &ffd);
    
    if (handle == INVALID_HANDLE_VALUE) 
    {
        log_assert(0);
        return;
    }
    
    int32_t filename_i = 0;
    while (FindNextFile(handle, &ffd) != 0 && filename_i < 1200) {
       if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
       {
            // pass
       }
       else
       {
           uint32_t string_len = get_string_length(ffd.cFileName) + 1;
           log_assert(string_len < 500);
           strcpy_capped(
               filenames[filename_i],
               string_len,
               ffd.cFileName);
            filename_i += 1;
       }
       log_assert(filename_i < 2000);
    }
    
    log_assert(GetLastError() == ERROR_NO_MORE_FILES);
    
    FindClose(handle);
    return;
}

