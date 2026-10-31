#include "T1_os.h"

#include "T1_std.h"
#include "T1_mem.h"
#include "T1_assert.h"
#include "T1_io.h"
#include "T1_global.h"
#include "T1_fatal_error.h"
#include "T1_macro_settings.h"
#include "T1_win32.h"
#include "T1_gpu_d3d11.h"

/*
The Win32 platform layer. Only kernel32 is linked, everything else is
loaded at runtime (see T1_win32.h), and the optional parts (DPI awareness,
timer resolution, vsync, gamepads) may be missing.

Strings are UTF-8 everywhere in T1, they're converted to UTF-16 for Windows'
W functions so paths with e.g. Japanese user names work.

T1's screen space is macOS's (see T1_global.h): points, (0, 0) at the
bottom left of the main screen. Windows uses pixels from the top left, so
positions and sizes are converted on the way in and out.
*/

#define T1_OS_WIN_PATH_CAP 512 // MAX_PATH is 260, small enough for no stack probes

#define T1_XINPUT_DPAD_UP        0x0001
#define T1_XINPUT_DPAD_DOWN      0x0002
#define T1_XINPUT_DPAD_LEFT      0x0004
#define T1_XINPUT_DPAD_RIGHT     0x0008
#define T1_XINPUT_START          0x0010
#define T1_XINPUT_BACK           0x0020
#define T1_XINPUT_LEFT_THUMB     0x0040
#define T1_XINPUT_RIGHT_THUMB    0x0080
#define T1_XINPUT_LEFT_SHOULDER  0x0100
#define T1_XINPUT_RIGHT_SHOULDER 0x0200
#define T1_XINPUT_A              0x1000
#define T1_XINPUT_B              0x2000
#define T1_XINPUT_X              0x4000
#define T1_XINPUT_Y              0x8000
// XINPUT_GAMEPAD_TRIGGER_THRESHOLD and _LEFT_THUMB_DEADZONE
#define T1_XINPUT_TRIGGER_THRESHOLD 30
#define T1_XINPUT_THUMB_DEADZONE 7849
#define T1_XINPUT_USERS_CAP 4
// Asking for a gamepad that isn't there is slow, so we only look for one
// this often
#define T1_XINPUT_RESCAN_US 2000000

T1Win32 * T1_win32 = NULL;

typedef struct {
    T1WinHandle hwnd;
    T1WinHandle hinstance;
    // the window's placement before fullscreen, to go back to
    T1WinWindowPlacement pre_fullscreen_placement;
    s64 perf_freq;
    u64 xinput_next_scan_us;
    f32 scale; // pixels per point, read once at startup like on macOS
    s32 xinput_user; // the connected gamepad, -1 for none
    b8 quit_requested;
    b8 is_fullscreen;
} T1OSWindowsState;

static T1OSWindowsState * T1_os_win_s = NULL;

/*
Sets *fn to the function called name in dll, NULL if there's no such
function (or dll)
*/
static void T1_win32_get_fn(
    void * fn,
    T1WinHandle dll,
    const char * name)
{
    T1WinFarProc found = dll != NULL ? GetProcAddress(dll, name) : NULL;

    // Every function pointer has the same size, the caller's type is right
    T1_std_memcpy(fn, &found, sizeof(T1WinFarProc));
}

void T1_win32_load(char ** const fatal_error) {
    if (*fatal_error != 0) { return; }

    T1_win32 = T1_mem_malloc_unmanaged(sizeof(T1Win32));
    if (T1_win32 == NULL) {
        *fatal_error = "Malloc fail";
        return;
    }
    T1_std_memset(T1_win32, 0, sizeof(T1Win32));

    T1WinHandle user32 = LoadLibraryW(L"user32.dll");
    T1WinHandle gdi32 = LoadLibraryW(L"gdi32.dll");
    T1WinHandle winmm = LoadLibraryW(L"winmm.dll");
    T1WinHandle xinput = LoadLibraryW(L"xinput1_4.dll");
    if (xinput == NULL) {
        xinput = LoadLibraryW(L"xinput9_1_0.dll");
    }

    #define T1_WIN32_LOAD(dll, name) \
        T1_win32_get_fn(&T1_win32->name, dll, #name)

    T1_WIN32_LOAD(user32, AdjustWindowRectEx);
    T1_WIN32_LOAD(user32, ClientToScreen);
    T1_WIN32_LOAD(user32, CreateWindowExW);
    T1_WIN32_LOAD(user32, DefWindowProcW);
    T1_WIN32_LOAD(user32, DestroyWindow);
    T1_WIN32_LOAD(user32, DispatchMessageW);
    T1_WIN32_LOAD(user32, GetClientRect);
    T1_WIN32_LOAD(user32, GetDC);
    T1_WIN32_LOAD(user32, GetKeyState);
    T1_WIN32_LOAD(user32, GetMonitorInfoW);
    T1_WIN32_LOAD(user32, GetSystemMetrics);
    T1_WIN32_LOAD(user32, GetWindowLongW);
    T1_WIN32_LOAD(user32, GetWindowPlacement);
    T1_WIN32_LOAD(user32, IsIconic);
    T1_WIN32_LOAD(user32, IsWindowVisible);
    T1_WIN32_LOAD(user32, LoadCursorW);
    T1_WIN32_LOAD(user32, LoadIconW);
    T1_WIN32_LOAD(user32, MapVirtualKeyW);
    T1_WIN32_LOAD(user32, MonitorFromWindow);
    T1_WIN32_LOAD(user32, MsgWaitForMultipleObjectsEx);
    T1_WIN32_LOAD(user32, PeekMessageW);
    T1_WIN32_LOAD(user32, RegisterClassExW);
    T1_WIN32_LOAD(user32, ReleaseCapture);
    T1_WIN32_LOAD(user32, ReleaseDC);
    T1_WIN32_LOAD(user32, SetCapture);
    T1_WIN32_LOAD(user32, SetForegroundWindow);
    T1_WIN32_LOAD(user32, SetWindowLongW);
    T1_WIN32_LOAD(user32, SetWindowPlacement);
    T1_WIN32_LOAD(user32, SetWindowPos);
    T1_WIN32_LOAD(user32, ShowWindow);
    T1_WIN32_LOAD(user32, SystemParametersInfoW);
    T1_WIN32_LOAD(user32, ValidateRect);
    T1_WIN32_LOAD(gdi32, GetDeviceCaps);
    T1_WIN32_LOAD(gdi32, GetStockObject);

    // Everything up to here exists on every Windows since XP
    void ** required = (void **)T1_win32;
    u32 required_size = (u32)(
        ((char *)&T1_win32->SetProcessDpiAwarenessContext -
            (char *)T1_win32) / sizeof(void *));
    for (u32 i = 0; i < required_size; i++) {
        if (required[i] == NULL) {
            *fatal_error = "Failed to load a function of user32 or gdi32";
            return;
        }
    }
    if (
        T1_win32->GetDeviceCaps == NULL ||
        T1_win32->GetStockObject == NULL)
    {
        *fatal_error = "Failed to load a function of gdi32";
        return;
    }

    T1_WIN32_LOAD(user32, SetProcessDpiAwarenessContext);
    T1_WIN32_LOAD(user32, SetProcessDPIAware);
    T1_WIN32_LOAD(user32, GetDpiForSystem);
    T1_WIN32_LOAD(winmm, timeBeginPeriod);
    T1_WIN32_LOAD(xinput, XInputGetState);

    #undef T1_WIN32_LOAD
}

static void T1_os_win_utf8_to_wide(
    const char * utf8,
    T1WChar * recip,
    const u32 recip_cap)
{
    recip[0] = 0;
    if (utf8 == NULL) { return; }

    s32 written = MultiByteToWideChar(
        T1CP_UTF8,
        0,
        utf8,
        -1,
        recip,
        (s32)recip_cap);
    if (written <= 0) {
        recip[0] = 0;
    }
}

static void T1_os_win_wide_to_utf8(
    const T1WChar * wide,
    char * recip,
    const u32 recip_cap)
{
    recip[0] = '\0';

    s32 written = WideCharToMultiByte(
        T1CP_UTF8,
        0,
        wide,
        -1,
        recip,
        (s32)recip_cap,
        NULL,
        NULL);
    if (written <= 0) {
        recip[0] = '\0';
    }
}

void T1_os_windows_init(char ** const fatal_error) {
    if (*fatal_error != 0) { return; }

    T1_win32_load(fatal_error);
    if (*fatal_error != 0) { return; }

    T1_os_win_s = T1_mem_malloc_unmanaged(sizeof(T1OSWindowsState));
    T1_std_memset(T1_os_win_s, 0, sizeof(T1OSWindowsState));

    T1_os_win_s->hinstance = GetModuleHandleW(NULL);
    T1_os_win_s->xinput_user = -1;
    QueryPerformanceFrequency(&T1_os_win_s->perf_freq);

    /*
    System DPI aware: we size the window in real pixels for the DPI the
    main screen had at startup. On a screen with another DPI, Windows scales
    our pixels, like macOS does when the backing scale factor changes.
    Before Windows 10, the older SetProcessDPIAware() does the same.
    */
    if (T1_win32->SetProcessDpiAwarenessContext != NULL) {
        // DPI_AWARENESS_CONTEXT_SYSTEM_AWARE
        T1_win32->SetProcessDpiAwarenessContext((T1WinHandle)(intptr_t)-2);
    } else if (T1_win32->SetProcessDPIAware != NULL) {
        T1_win32->SetProcessDPIAware();
    }

    u32 dpi = 96;
    if (T1_win32->GetDpiForSystem != NULL) {
        dpi = T1_win32->GetDpiForSystem();
    } else {
        T1WinHandle screen_dc = T1_win32->GetDC(NULL);
        if (screen_dc != NULL) {
            dpi = (u32)T1_win32->GetDeviceCaps(screen_dc, T1LOGPIXELSX);
            T1_win32->ReleaseDC(NULL, screen_dc);
        }
    }
    T1_os_win_s->scale = (dpi >= 96 && dpi <= 96 * 8) ?
        (f32)dpi / 96.0f :
        1.0f;

    // Sleeping and waiting for events in 1 ms steps instead of 15.6
    if (T1_win32->timeBeginPeriod != NULL) {
        T1_win32->timeBeginPeriod(1);
    }
}

void T1_os_get_dir_separator(char * recipient) {
    recipient[0] = '\\';
    recipient[1] = '\0';
}

u64 T1_os_get_current_time_us(void) {
    s64 now = 0;
    QueryPerformanceCounter(&now);

    u64 freq = (u64)T1_os_win_s->perf_freq;
    u64 ticks = (u64)now;

    // in 2 steps, ticks * 1000000 would overflow after a few days uptime
    return (ticks / freq) * 1000000 + ((ticks % freq) * 1000000) / freq;
}

u64 T1_os_get_clock_frequency(void) {
    return 3600000000; // a guess: 3.6 GHz, like on macOS
}

u64 T1_os_get_filesize(const char * filepath) {
    T1WChar wpath[T1_OS_WIN_PATH_CAP];
    T1_os_win_utf8_to_wide(filepath, wpath, T1_OS_WIN_PATH_CAP);

    T1WinFileAttributeData attribs;
    if (
        !GetFileAttributesExW(wpath, /* GetFileExInfoStandard: */ 0, &attribs) ||
        (attribs.attributes & T1FILE_ATTRIBUTE_DIRECTORY))
    {
        return 0;
    }

    return ((u64)attribs.size_high << 32) | (u64)attribs.size_low;
}

void T1_os_read_file(
    const char * filepath,
    char * recip,
    u32 * recip_size,
    const u64 recip_cap,
    char ** const fatal_error)
{
    if (*fatal_error != 0) { return; }

    *recip_size = 0;

    if (filepath == NULL || filepath[0] == '\0') {
        *fatal_error = "T1_os_read_file() was passed an empty filepath param";
        return;
    }

    T1WChar wpath[T1_OS_WIN_PATH_CAP];
    T1_os_win_utf8_to_wide(filepath, wpath, T1_OS_WIN_PATH_CAP);

    T1WinHandle file = CreateFileW(
        wpath,
        T1GENERIC_READ,
        T1FILE_SHARE_READ,
        NULL,
        T1OPEN_EXISTING,
        T1FILE_ATTRIBUTE_NORMAL,
        NULL);

    s64 file_size = 0;
    if (
        file == T1_WIN_INVALID_HANDLE ||
        !GetFileSizeEx(file, &file_size) ||
        recip_cap < 1 ||
        recip_cap >= UINT32_MAX ||
        (u64)file_size >= UINT32_MAX)
    {
        if (file != T1_WIN_INVALID_HANDLE) { CloseHandle(file); }
        T1_fatal_error_new(
            fatal_error,
            "T1_os_read_file() Error - failed to read: ");
        if (T1_std_strlen(filepath) < 450) {
            T1_std_strcat_cap(*fatal_error, 512, filepath);
        } else {
            T1_std_strcat_cap(*fatal_error, 512, "450+ bytes filepath");
        }
        return;
    }

    // Like on macOS: up to recip_cap bytes, with a 0 after them
    u32 to_read = (u32)file_size;
    if (to_read > recip_cap) { to_read = (u32)recip_cap; }

    u32 total_read = 0;
    while (total_read < to_read) {
        u32 bytes_read = 0;
        if (
            !ReadFile(
                file,
                recip + total_read,
                to_read - total_read,
                &bytes_read,
                NULL) ||
            bytes_read == 0)
        {
            break;
        }
        total_read += bytes_read;
    }
    CloseHandle(file);

    *recip_size = total_read;
    recip[total_read] = '\0';
}

u8 T1_os_file_exists(const char * filepath) {
    T1WChar wpath[T1_OS_WIN_PATH_CAP];
    T1_os_win_utf8_to_wide(filepath, wpath, T1_OS_WIN_PATH_CAP);

    u32 attribs = GetFileAttributesW(wpath);

    return
        attribs != T1INVALID_FILE_ATTRIBUTES &&
        (attribs & T1FILE_ATTRIBUTE_DIRECTORY) == 0;
}

b8 T1_os_mkdir_if_not_exist(const char * dirname) {
    if (dirname == NULL || dirname[0] == '\0') {
        return 0;
    }

    T1WChar wpath[T1_OS_WIN_PATH_CAP];
    T1_os_win_utf8_to_wide(dirname, wpath, T1_OS_WIN_PATH_CAP);

    // A trailing separator (T1_os_writable_path("") gives one) is fine for
    // CreateDirectoryW, but not for checking if it exists
    u32 len = 0;
    while (wpath[len] != 0) { len++; }
    while (len > 3 && (wpath[len - 1] == L'\\' || wpath[len - 1] == L'/')) {
        wpath[len - 1] = 0;
        len--;
    }

    u32 attribs = GetFileAttributesW(wpath);
    if (
        attribs != T1INVALID_FILE_ATTRIBUTES &&
        (attribs & T1FILE_ATTRIBUTE_DIRECTORY))
    {
        return 1;
    }

    // Like macOS's withIntermediateDirectories: make the parents first
    for (u32 i = 3; i < len; i++) {
        if (wpath[i] == L'\\' || wpath[i] == L'/') {
            T1WChar separator = wpath[i];
            wpath[i] = 0;
            CreateDirectoryW(wpath, NULL);
            wpath[i] = separator;
        }
    }

    return
        CreateDirectoryW(wpath, NULL) ||
        GetLastError() == T1ERROR_ALREADY_EXISTS;
}

void T1_os_del_file(const char * filepath) {
    T1WChar wpath[T1_OS_WIN_PATH_CAP];
    T1_os_win_utf8_to_wide(filepath, wpath, T1_OS_WIN_PATH_CAP);

    DeleteFileW(wpath);
}

void T1_os_copy_file(
    const char * filepath_source,
    const char * filepath_destination)
{
    T1_assert(filepath_source != NULL);
    T1_assert(filepath_source[0] != '\0');
    T1_assert(filepath_destination != NULL);
    T1_assert(filepath_destination[0] != '\0');

    T1WChar wsource[T1_OS_WIN_PATH_CAP];
    T1WChar wdest[T1_OS_WIN_PATH_CAP];
    T1_os_win_utf8_to_wide(filepath_source, wsource, T1_OS_WIN_PATH_CAP);
    T1_os_win_utf8_to_wide(filepath_destination, wdest, T1_OS_WIN_PATH_CAP);

    // Like macOS's copyItemAtPath: never overwrites
    CopyFileW(wsource, wdest, /* fail_if_exists: */ 1);
}

void T1_os_write_file(
    const char * filepath,
    const char * output,
    u32 output_size,
    u8 * good)
{
    *good = 0;

    T1WChar wpath[T1_OS_WIN_PATH_CAP];
    T1_os_win_utf8_to_wide(filepath, wpath, T1_OS_WIN_PATH_CAP);

    T1WinHandle file = CreateFileW(
        wpath,
        T1GENERIC_WRITE,
        0,
        NULL,
        T1CREATE_ALWAYS,
        T1FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (file == T1_WIN_INVALID_HANDLE) {
        return;
    }

    u32 total_written = 0;
    while (total_written < output_size) {
        u32 written = 0;
        if (
            !WriteFile(
                file,
                output + total_written,
                output_size - total_written,
                &written,
                NULL) ||
            written == 0)
        {
            break;
        }
        total_written += written;
    }
    CloseHandle(file);

    *good = total_written == output_size;
}

void T1_os_get_filenames_in(
    const char * directory,
    char * filenames,
    const u32 filenames_cap)
{
    char pattern[T1_OS_WIN_PATH_CAP];
    T1_std_strcpy_cap(pattern, T1_OS_WIN_PATH_CAP, directory);
    T1_std_strcat_cap(pattern, T1_OS_WIN_PATH_CAP, "\\*");

    T1WChar wpattern[T1_OS_WIN_PATH_CAP];
    T1_os_win_utf8_to_wide(pattern, wpattern, T1_OS_WIN_PATH_CAP);

    T1WinFindDataW found;
    T1WinHandle find = FindFirstFileW(wpattern, &found);
    if (find == T1_WIN_INVALID_HANDLE) {
        return;
    }

    u32 i = 0;
    do {
        // like on macOS, skips ".", ".." and hidden files
        if (found.file_name[0] == 0 || found.file_name[0] == L'.') {
            continue;
        }

        char name[T1MAX_PATH * 4];
        T1_os_win_wide_to_utf8(found.file_name, name, T1MAX_PATH * 4);

        u32 new_len = (u32)T1_std_strlen(name);
        if (i + new_len + 1 >= filenames_cap) {
            break;
        }

        T1_std_strcpy_cap(filenames + i, filenames_cap - i, name);
        i += new_len + 1;
    } while (FindNextFileW(find, &found));

    FindClose(find);
}

/*
The folder the .exe is in, which is what macOS gives an app that isn't in a
bundle
*/
void T1_os_get_res_dir(char * recip, u32 recip_cap) {
    T1WChar wpath[T1_OS_WIN_PATH_CAP];
    u32 len = GetModuleFileNameW(NULL, wpath, T1_OS_WIN_PATH_CAP);
    if (len == 0 || len >= T1_OS_WIN_PATH_CAP) {
        recip[0] = '\0';
        return;
    }

    while (len > 0 && wpath[len - 1] != L'\\' && wpath[len - 1] != L'/') {
        len--;
    }
    if (len > 0) { len--; } // the separator
    wpath[len] = 0;

    T1_os_win_wide_to_utf8(wpath, recip, recip_cap);
}

void T1_os_get_cwd(char * recip, u32 recip_size) {
    T1WChar wpath[T1_OS_WIN_PATH_CAP];
    u32 len = GetCurrentDirectoryW(T1_OS_WIN_PATH_CAP, wpath);
    if (len == 0 || len >= T1_OS_WIN_PATH_CAP) {
        recip[0] = '\0';
        return;
    }

    T1_os_win_wide_to_utf8(wpath, recip, recip_size);
}

// %APPDATA%\T1_APP_NAME, like macOS's Application Support/T1_APP_NAME
void T1_os_get_writables_dir(char * recipient, const u32 recipient_size) {
    recipient[0] = '\0';

    T1WChar wpath[T1_OS_WIN_PATH_CAP];
    u32 len = GetEnvironmentVariableW(
        L"APPDATA",
        wpath,
        T1_OS_WIN_PATH_CAP);
    if (len == 0 || len >= T1_OS_WIN_PATH_CAP) {
        return;
    }

    T1_os_win_wide_to_utf8(wpath, recipient, recipient_size);
    if (recipient[0] == '\0') {
        return;
    }
    T1_std_strcat_cap(recipient, recipient_size, "\\");
    T1_std_strcat_cap(recipient, recipient_size, T1_APP_NAME);
}

// What a new thread runs: its own copy, so threads started at the same time
// can't overwrite each other's (the thread frees it)
typedef struct {
    void *(* function_to_run)(void *);
    void * argument;
} T1OSThreadStart;

static u32 T1_WINAPI T1_os_win_thread_main(void * thread_start) {
    T1OSThreadStart start = *(T1OSThreadStart *)thread_start;
    T1_mem_free_managed(thread_start);

    char * dud_error = 0;
    u64 T1_thread_id = T1_os_get_T1_thread_id(&dud_error);
    T1_assert(dud_error == 0);
    (void)T1_thread_id;

    start.function_to_run(start.argument);

    // the thread ends: its T1 thread id goes to the next thread
    T1_os_forget_raw_thread_id((u64)GetCurrentThreadId());

    return 0;
}

void T1_os_start_thread(
    void *(*function_to_run)(void *),
    void * argument)
{
    T1OSThreadStart * start = T1_mem_malloc_managed(sizeof(T1OSThreadStart));
    start->function_to_run = function_to_run;
    start->argument = argument;

    T1WinHandle thread = CreateThread(
        NULL,
        0,
        T1_os_win_thread_main,
        start,
        0,
        NULL);
    T1_assert(thread != NULL);
    // nobody joins our threads: their resources go when they end
    if (thread != NULL) {
        CloseHandle(thread);
    }
}

u64 T1_os_get_T1_thread_id(char ** const fatal_error) {
    if (*fatal_error != 0) { return 0; }

    // never 0, which T1_os_raw_thread_id_to_T1_thread_id() reserves
    return T1_os_raw_thread_id_to_T1_thread_id(
        (u64)GetCurrentThreadId(),
        fatal_error);
}

u32 T1_os_get_cpu_logical_core_count(void) {
    T1WinSystemInfo info;
    GetSystemInfo(&info);

    return info.number_of_processors > 0 ? info.number_of_processors : 1;
}

u32 T1_os_get_page_size(void) {
    T1WinSystemInfo info;
    GetSystemInfo(&info);

    return info.page_size > 0 ? info.page_size : 4096;
}

void T1_os_close_app(void) {
    // The client's loop sees T1_window_close_requested() and shuts down
    T1_os_win_s->quit_requested = 1;
}

f32 T1_os_get_screen_backing_scale_factor(void) {
    if (T1_os_win_s == NULL) {
        return 1.0f;
    }

    return T1_os_win_s->scale;
}

float T1_os_x_to_x(const float x) {
    return x;
}

float T1_os_y_to_y(const float y) {
    return y;
}

#if T1_GAMEPAD_ACTIVE == T1_ACTIVE
static void T1_os_win_update_gamepad_key(u8 is_pressed, T1IOKey key) {
    if (is_pressed) {
        T1_io_register_keydown(key, /* debounces: */ 1);
    } else {
        T1_io_register_keyup(key, /* debounces: */ 1);
    }
}

// -1 to 1, 0 inside the dead zone, like GameController gives us on macOS
static f32 T1_os_win_thumb_axis(s16 value) {
    if (value > -T1_XINPUT_THUMB_DEADZONE && value < T1_XINPUT_THUMB_DEADZONE) {
        return 0.0f;
    }

    f32 out = value > 0 ?
        (f32)(value - T1_XINPUT_THUMB_DEADZONE) /
            (f32)(32767 - T1_XINPUT_THUMB_DEADZONE) :
        (f32)(value + T1_XINPUT_THUMB_DEADZONE) /
            (f32)(32768 - T1_XINPUT_THUMB_DEADZONE);

    return T1_std_clampf(out, -1.0f, 1.0f);
}

void T1_os_poll_gamepad_events(void) {
    if (T1_win32->XInputGetState == NULL) {
        return;
    }

    T1XInputState state;
    T1_std_memset(&state, 0, sizeof(T1XInputState));

    if (T1_os_win_s->xinput_user >= 0) {
        if (
            T1_win32->XInputGetState(
                (u32)T1_os_win_s->xinput_user,
                &state) != T1ERROR_SUCCESS)
        {
            T1_os_win_s->xinput_user = -1;
            T1_std_memset(&state, 0, sizeof(T1XInputState));
        }
    } else {
        u64 now = T1_os_get_current_time_us();
        if (now < T1_os_win_s->xinput_next_scan_us) {
            return;
        }
        T1_os_win_s->xinput_next_scan_us = now + T1_XINPUT_RESCAN_US;

        for (s32 user = 0; user < T1_XINPUT_USERS_CAP; user++) {
            if (
                T1_win32->XInputGetState(
                    (u32)user,
                    &state) == T1ERROR_SUCCESS)
            {
                T1_os_win_s->xinput_user = user;
                break;
            }
        }
        if (T1_os_win_s->xinput_user < 0) {
            return;
        }
    }

    // Disconnecting leaves state zeroed, so every button goes up once
    T1XInputGamepad * g = &state.gamepad;

    T1_os_win_update_gamepad_key(
        (g->buttons & T1_XINPUT_DPAD_LEFT) != 0, T1_IO_GAMEPAD_DPAD_LEFT);
    T1_os_win_update_gamepad_key(
        (g->buttons & T1_XINPUT_DPAD_RIGHT) != 0, T1_IO_GAMEPAD_DPAD_RIGHT);
    T1_os_win_update_gamepad_key(
        (g->buttons & T1_XINPUT_DPAD_UP) != 0, T1_IO_GAMEPAD_DPAD_UP);
    T1_os_win_update_gamepad_key(
        (g->buttons & T1_XINPUT_DPAD_DOWN) != 0, T1_IO_GAMEPAD_DPAD_DOWN);

    T1_os_win_update_gamepad_key(
        (g->buttons & T1_XINPUT_LEFT_SHOULDER) != 0,
        T1_IO_GAMEPAD_LSHOULDER);
    T1_os_win_update_gamepad_key(
        (g->buttons & T1_XINPUT_RIGHT_SHOULDER) != 0,
        T1_IO_GAMEPAD_RSHOULDER);
    T1_os_win_update_gamepad_key(
        g->left_trigger > T1_XINPUT_TRIGGER_THRESHOLD,
        T1_IO_GAMEPAD_LTRIGGER);
    T1_os_win_update_gamepad_key(
        g->right_trigger > T1_XINPUT_TRIGGER_THRESHOLD,
        T1_IO_GAMEPAD_RTRIGGER);
    T1_os_win_update_gamepad_key(
        (g->buttons & T1_XINPUT_LEFT_THUMB) != 0,
        T1_IO_GAMEPAD_LTHUMBSTICKBTN);
    T1_os_win_update_gamepad_key(
        (g->buttons & T1_XINPUT_RIGHT_THUMB) != 0,
        T1_IO_GAMEPAD_RTHUMBSTICKBTN);

    T1_os_win_update_gamepad_key(
        (g->buttons & T1_XINPUT_A) != 0, T1_IO_GAMEPAD_A);
    T1_os_win_update_gamepad_key(
        (g->buttons & T1_XINPUT_B) != 0, T1_IO_GAMEPAD_B);
    T1_os_win_update_gamepad_key(
        (g->buttons & T1_XINPUT_X) != 0, T1_IO_GAMEPAD_X);
    T1_os_win_update_gamepad_key(
        (g->buttons & T1_XINPUT_Y) != 0, T1_IO_GAMEPAD_Y);
    // XInput doesn't report the guide button, so no T1_IO_GAMEPAD_HOME
    T1_os_win_update_gamepad_key(
        (g->buttons & T1_XINPUT_START) != 0, T1_IO_GAMEPAD_MENU);
    T1_os_win_update_gamepad_key(
        (g->buttons & T1_XINPUT_BACK) != 0, T1_IO_GAMEPAD_OPTIONS);

    T1_io_register_key_move_to_pos(
        T1_IO_GAMEPAD_LTHUMBSTICK,
        T1_os_win_thumb_axis(g->thumb_lx),
        T1_os_win_thumb_axis(g->thumb_ly));
    T1_io_register_key_move_to_pos(
        T1_IO_GAMEPAD_RTHUMBSTICK,
        T1_os_win_thumb_axis(g->thumb_rx),
        T1_os_win_thumb_axis(g->thumb_ry));
}
#elif T1_GAMEPAD_ACTIVE == T1_INACTIVE
#else
#error
#endif

/*
T1IOKeys of printable characters are their ASCII codes, so the keys whose
character depends on the keyboard layout ask the layout
*/
static u32 T1_os_win_vk_to_T1_key(T1WParam vk) {
    if (vk >= 'A' && vk <= 'Z') { return (u32)vk; }
    if (vk >= '0' && vk <= '9') { return (u32)vk; }
    if (vk >= T1VK_NUMPAD0 && vk <= T1VK_NUMPAD9) {
        return T1_IO_KEYBOARD_0 + (u32)(vk - T1VK_NUMPAD0);
    }
    if (vk >= T1VK_F1 && vk <= T1VK_F12) {
        return T1_IO_KEYBOARD_F1 + (u32)(vk - T1VK_F1);
    }

    switch (vk) {
        case T1VK_LEFT:     return T1_IO_KEYBOARD_LEFTARROW;
        case T1VK_RIGHT:    return T1_IO_KEYBOARD_RIGHTARROW;
        case T1VK_UP:       return T1_IO_KEYBOARD_UPARROW;
        case T1VK_DOWN:     return T1_IO_KEYBOARD_DOWNARROW;
        case T1VK_RETURN:   return T1_IO_KEYBOARD_ENTER;
        case T1VK_ESCAPE:   return T1_IO_KEYBOARD_ESCAPE;
        case T1VK_TAB:      return T1_IO_KEYBOARD_TAB;
        case T1VK_SPACE:    return T1_IO_KEYBOARD_SPACEBAR;
        case T1VK_BACK:     return T1_IO_KEYBOARD_BACKSPACE;
        case T1VK_KANA:     return T1_IO_KEYBOARD_KANA;
        case T1VK_MENU:     return T1_IO_KEYBOARD_ALT;
        case T1VK_LWIN:
        case T1VK_RWIN:     return T1_IO_KEYBOARD_WINKEY;
        case T1VK_CAPITAL:  return T1_IO_KEYBOARD_CAPSLOCK;
        case T1VK_CONTROL:  return T1_IO_KEYBOARD_CONTROL;
        case T1VK_SHIFT:    return T1_IO_KEYBOARD_SHIFT;
        case T1VK_CLEAR:    return T1_IO_KEYBOARD_NUMPADCLEAR;
        case T1VK_INSERT:   return T1_IO_KEYBOARD_INSERT;
        case T1VK_HOME:     return T1_IO_KEYBOARD_HOME;
        case T1VK_PRIOR:    return T1_IO_KEYBOARD_PAGEUP;
        case T1VK_END:      return T1_IO_KEYBOARD_END;
        case T1VK_NEXT:     return T1_IO_KEYBOARD_PAGEDOWN;
        case T1VK_MULTIPLY: return T1_IO_KEYBOARD_ASTERISK;
        case T1VK_ADD:      return T1_IO_KEYBOARD_PLUS;
        case T1VK_SUBTRACT: return T1_IO_KEYBOARD_MINUS;
        case T1VK_DECIMAL:  return T1_IO_KEYBOARD_FULLSTOP;
        default: break;
    }

    // the unshifted character of the key in the current layout
    u32 c = T1_win32->MapVirtualKeyW((u32)vk, T1MAPVK_VK_TO_CHAR) &
        0x7FFFFFFF;
    if (
        (c >= T1_IO_KEYBOARD_EXCLAM && c <= T1_IO_KEYBOARD_AT) ||
        c == T1_IO_KEYBOARD_OPENSQUARE ||
        (c >= T1_IO_KEYBOARD_CLOSESQUARE && c <= T1_IO_KEYBOARD_UNDERSCORE) ||
        (c >= T1_IO_KEYBOARD_OPENCURLY && c <= T1_IO_KEYBOARD_TILDE))
    {
        return c;
    }
    if (c == '\\') { return T1_IO_KEYBOARD_YENSIGN; } // ¥ on Japanese ones

    return T1_IO_KEYBOARD_UNKNOWNBTN;
}

// The window's client area in T1's screen space (see top of file)
static void T1_os_win_update_global_window_pos(void) {
    T1WinHandle hwnd = T1_os_win_s->hwnd;
    if (
        hwnd == NULL ||
        T1_win32->IsIconic(hwnd) ||
        T1_os_win_s->is_fullscreen)
    {
        return;
    }

    T1WinRect client;
    T1_win32->GetClientRect(hwnd, &client);
    T1WinPoint bottom_left;
    bottom_left.x = 0;
    bottom_left.y = client.bottom;
    T1_win32->ClientToScreen(hwnd, &bottom_left);

    f32 screen_height = (f32)T1_win32->GetSystemMetrics(T1SM_CYSCREEN);
    T1_global_update_window_pos(
        (f32)bottom_left.x / T1_os_win_s->scale,
        (screen_height - (f32)bottom_left.y) / T1_os_win_s->scale);
}

// The signed x and y in an lparam of mouse messages
#define T1_LPARAM_X(lparam) ((s32)(s16)((u64)(lparam) & 0xFFFF))
#define T1_LPARAM_Y(lparam) ((s32)(s16)(((u64)(lparam) >> 16) & 0xFFFF))

static void T1_os_win_mouse_moved(T1LParam lparam) {
    if (T1_global->block_mouse) {
        return;
    }

    T1WinRect client;
    T1_win32->GetClientRect(T1_os_win_s->hwnd, &client);

    // like macOS's locationInWindow: points, from the bottom left
    T1_io_register_key_move_to_pos(
        T1_IO_MOUSE,
        (f32)T1_LPARAM_X(lparam) / T1_os_win_s->scale,
        (f32)(client.bottom - T1_LPARAM_Y(lparam)) / T1_os_win_s->scale);
}

static void T1_os_win_mouse_button(u32 key, b8 down) {
    // Keeps sending us the mouse while a button is held outside the window,
    // so we see it come up
    if (down) {
        T1_io_register_keydown(key, 0);
        T1_win32->SetCapture(T1_os_win_s->hwnd);
    } else {
        T1_io_register_keyup(key, /* debounces: */ 0);
        if (
            !(T1_win32->GetKeyState(T1VK_LBUTTON) & 0x8000) &&
            !(T1_win32->GetKeyState(T1VK_RBUTTON) & 0x8000) &&
            !(T1_win32->GetKeyState(T1VK_MBUTTON) & 0x8000))
        {
            T1_win32->ReleaseCapture();
        }
    }
}

static T1LResult T1_WINAPI T1_os_win_window_proc(
    T1WinHandle hwnd,
    u32 message,
    T1WParam wparam,
    T1LParam lparam)
{
    switch (message) {
        case T1WM_NCCREATE:
            // before CreateWindowExW() returns, WM_SIZE etc. need it
            T1_os_win_s->hwnd = hwnd;
            break;
        case T1WM_CLOSE:
            // The client's loop sees T1_window_close_requested() and shuts
            // down, the window goes with the process
            T1_os_close_app();
            return 0;
        case T1WM_SIZE: {
            if (wparam == T1SIZE_MINIMIZED) {
                return 0;
            }
            T1_global_update_window_size(
                (f32)((u64)lparam & 0xFFFF) / T1_os_win_s->scale,
                (f32)(((u64)lparam >> 16) & 0xFFFF) / T1_os_win_s->scale,
                T1_os_get_current_time_us());
            T1_os_win_update_global_window_pos();
            return 0;
        }
        case T1WM_MOVE:
            T1_os_win_update_global_window_pos();
            return 0;
        case T1WM_ERASEBKGND:
            // T1_os_window_present() paints all of it
            return 1;
        case T1WM_PAINT:
            T1_win32->ValidateRect(hwnd, NULL);
            return 0;
        case T1WM_KEYDOWN:
        case T1WM_SYSKEYDOWN:
            // bit 30: was already down, a key repeat
            if ((lparam & (1 << 30)) == 0) {
                T1_io_register_keydown(
                    T1_os_win_vk_to_T1_key(wparam),
                    /* debounces: */ 0);
            }
            /*
            Alt+F4 still closes the window. Other Alt and F10 presses would
            open the window menu, which stops our loop
            */
            if (message == T1WM_SYSKEYDOWN && wparam == T1VK_F4) { break; }
            return 0;
        case T1WM_KEYUP:
        case T1WM_SYSKEYUP:
            T1_io_register_keyup(
                T1_os_win_vk_to_T1_key(wparam),
                /* debounces: */ 0);
            return 0;
        case T1WM_SYSCHAR:
            // no beep for Alt + a key that isn't a menu shortcut
            return 0;
        case T1WM_MOUSEMOVE:
            T1_os_win_mouse_moved(lparam);
            return 0;
        case T1WM_LBUTTONDOWN:
            T1_os_win_mouse_button(T1_IO_MOUSE_LCLICK, 1);
            return 0;
        case T1WM_LBUTTONUP:
            T1_os_win_mouse_button(T1_IO_MOUSE_LCLICK, 0);
            return 0;
        case T1WM_RBUTTONDOWN:
            T1_os_win_mouse_button(T1_IO_MOUSE_RCLICK, 1);
            return 0;
        case T1WM_RBUTTONUP:
            T1_os_win_mouse_button(T1_IO_MOUSE_RCLICK, 0);
            return 0;
        // Like on macOS, the buttons after left and right are "other" ones
        case T1WM_MBUTTONDOWN:
            T1_os_win_mouse_button(T1_IO_MOUSE_OTHERCLICK1, 1);
            return 0;
        case T1WM_MBUTTONUP:
            T1_os_win_mouse_button(T1_IO_MOUSE_OTHERCLICK1, 0);
            return 0;
        case T1WM_XBUTTONDOWN:
        case T1WM_XBUTTONUP:
            T1_os_win_mouse_button(
                ((wparam >> 16) & 0xFFFF) == T1XBUTTON1 ?
                    T1_IO_MOUSE_OTHERCLICK2 :
                    T1_IO_MOUSE_OTHERCLICK3,
                message == T1WM_XBUTTONDOWN);
            return 1;
        case T1WM_MOUSEWHEEL: {
            /*
            The same steps as on macOS, where a notch of a wheel is about
            1.0 of deltaY
            */
            f32 delta =
                (f32)(s16)((wparam >> 16) & 0xFFFF) / (f32)T1WHEEL_DELTA;
            f32 step = 0.1f;
            if (delta > 0.0f) {
                while (delta > step) {
                    T1_io_register_keyup_force_up_short(
                        T1_IO_MOUSE_WHEEL_UP);
                    delta -= step;
                }
            } else {
                while (delta < -step) {
                    T1_io_register_keyup_force_up_short(
                        T1_IO_MOUSE_WHEEL_DOWN);
                    delta += step;
                }
            }
            return 0;
        }
        default:
            break;
    }

    return T1_win32->DefWindowProcW(hwnd, message, wparam, lparam);
}

void T1_os_window_open(char ** const fatal_error) {
    if (*fatal_error != 0) { return; }

    if (!T1_global) {
        *fatal_error = "Tried to T1_window_open() before T1_init()";
        return;
    }

    T1WinWndClassExW window_class;
    T1_std_memset(&window_class, 0, sizeof(T1WinWndClassExW));
    window_class.size = sizeof(T1WinWndClassExW);
    window_class.wnd_proc = T1_os_win_window_proc;
    window_class.instance = T1_os_win_s->hinstance;
    window_class.cursor = T1_win32->LoadCursorW(NULL, T1IDC_ARROW);
    window_class.icon = T1_win32->LoadIconW(NULL, T1IDI_APPLICATION);
    window_class.background = T1_win32->GetStockObject(T1BLACK_BRUSH);
    window_class.class_name = L"T1Window";
    if (!T1_win32->RegisterClassExW(&window_class)) {
        *fatal_error = "Failed to register the window class";
        return;
    }

    // The client area in pixels from the top left of the main screen
    f32 scale = T1_os_win_s->scale;
    s32 screen_height = T1_win32->GetSystemMetrics(T1SM_CYSCREEN);
    T1WinRect rect;
    rect.left = (s32)(T1_global->window_left * scale);
    rect.right = rect.left + (s32)(T1_global->window_wh[0] * scale);
    rect.bottom = screen_height -
        (s32)(T1_global->window_bottom * scale);
    rect.top = rect.bottom - (s32)(T1_global->window_wh[1] * scale);

    u32 style = T1WS_OVERLAPPEDWINDOW;
    T1_win32->AdjustWindowRectEx(&rect, style, 0, 0);

    /*
    The initial size is in points, e.g. 1400 x 1000, which may not fit a
    laptop's screen at 150%. Keep the whole window, title bar included, on
    the work area (the screen without the taskbar).
    */
    T1WinRect work;
    if (T1_win32->SystemParametersInfoW(T1SPI_GETWORKAREA, 0, &work, 0)) {
        s32 width = rect.right - rect.left;
        s32 height = rect.bottom - rect.top;
        if (width > work.right - work.left) {
            width = work.right - work.left;
        }
        if (height > work.bottom - work.top) {
            height = work.bottom - work.top;
        }
        if (rect.left < work.left) { rect.left = work.left; }
        if (rect.top < work.top) { rect.top = work.top; }
        if (rect.left + width > work.right) { rect.left = work.right - width; }
        if (rect.top + height > work.bottom) {
            rect.top = work.bottom - height;
        }
        rect.right = rect.left + width;
        rect.bottom = rect.top + height;
    }

    T1WChar title[256];
    T1_os_win_utf8_to_wide(T1_APP_NAME, title, 256);
    if (title[0] == 0) {
        T1_os_win_utf8_to_wide("Invalid app name", title, 256);
    }

    // WM_SIZE and WM_MOVE update T1_global to the size we really got
    T1_os_win_s->hwnd = T1_win32->CreateWindowExW(
        0,
        L"T1Window",
        title,
        style,
        rect.left,
        rect.top,
        rect.right - rect.left,
        rect.bottom - rect.top,
        NULL,
        NULL,
        T1_os_win_s->hinstance,
        NULL);
    if (T1_os_win_s->hwnd == NULL) {
        *fatal_error = "Failed to create a window";
        return;
    }

    T1_win32->ShowWindow(T1_os_win_s->hwnd, T1SW_SHOW);
    T1_win32->SetForegroundWindow(T1_os_win_s->hwnd);
}

void T1_os_destroy_main_window_if_possible(void) {
    if (T1_os_win_s->hwnd != NULL) {
        T1_win32->DestroyWindow(T1_os_win_s->hwnd);
        T1_os_win_s->hwnd = NULL;
    }
}

/*
Borderless fullscreen: the window loses its frame and covers its monitor.
No video mode change, so switching is instant.
*/
void T1_os_toggle_fullscreen(void) {
    T1WinHandle hwnd = T1_os_win_s->hwnd;
    if (hwnd == NULL) {
        return;
    }

    u32 style = (u32)T1_win32->GetWindowLongW(hwnd, T1GWL_STYLE);

    if (!T1_os_win_s->is_fullscreen) {
        T1WinMonitorInfo monitor;
        T1_std_memset(&monitor, 0, sizeof(T1WinMonitorInfo));
        monitor.size = sizeof(T1WinMonitorInfo);
        T1_os_win_s->pre_fullscreen_placement.length =
            sizeof(T1WinWindowPlacement);
        if (
            !T1_win32->GetWindowPlacement(
                hwnd,
                &T1_os_win_s->pre_fullscreen_placement) ||
            !T1_win32->GetMonitorInfoW(
                T1_win32->MonitorFromWindow(
                    hwnd,
                    T1MONITOR_DEFAULTTOPRIMARY),
                &monitor))
        {
            return;
        }

        T1_os_win_s->is_fullscreen = 1;
        T1_global->fullscreen = 1;
        T1_win32->SetWindowLongW(
            hwnd,
            T1GWL_STYLE,
            (s32)((style & ~T1WS_OVERLAPPEDWINDOW) | T1WS_POPUP));
        T1_win32->SetWindowPos(
            hwnd,
            T1HWND_TOP,
            monitor.monitor.left,
            monitor.monitor.top,
            monitor.monitor.right - monitor.monitor.left,
            monitor.monitor.bottom - monitor.monitor.top,
            T1SWP_NOOWNERZORDER | T1SWP_FRAMECHANGED);
    } else {
        T1_os_win_s->is_fullscreen = 0;
        T1_global->fullscreen = 0;
        T1_win32->SetWindowLongW(
            hwnd,
            T1GWL_STYLE,
            (s32)((style & ~T1WS_POPUP) | T1WS_OVERLAPPEDWINDOW));
        T1_win32->SetWindowPlacement(
            hwnd,
            &T1_os_win_s->pre_fullscreen_placement);
        T1_win32->SetWindowPos(
            hwnd,
            NULL,
            0, 0, 0, 0,
            T1SWP_NOMOVE | T1SWP_NOSIZE | T1SWP_NOZORDER |
            T1SWP_NOOWNERZORDER | T1SWP_FRAMECHANGED);
    }
}

void T1_os_enter_fullscreen(void) {
    if (!T1_os_win_s->is_fullscreen) {
        T1_os_toggle_fullscreen();
    }
}

/*
Note: while the user drags the window's border or title bar, Windows runs
its own loop inside DispatchMessageW, so the client's frames stop until
they let go (TODO).
*/
void T1_os_window_pump_events(const u64 max_wait_us) {
    if (max_wait_us > 0) {
        u32 wait_ms = (u32)((max_wait_us + 999) / 1000);
        T1_win32->MsgWaitForMultipleObjectsEx(
            0,
            NULL,
            wait_ms,
            T1QS_ALLINPUT,
            T1MWMO_INPUTAVAILABLE);
    }

    T1WinMsg msg;
    while (
        !T1_os_win_s->quit_requested &&
        T1_win32->PeekMessageW(&msg, NULL, 0, 0, T1PM_REMOVE))
    {
        if (msg.message == T1WM_QUIT) {
            T1_os_close_app();
            break;
        }
        T1_win32->DispatchMessageW(&msg);
    }
}

u8 T1_os_window_close_requested(void) {
    return T1_os_win_s->quit_requested;
}

u8 T1_os_window_visible(void) {
    // False while minimized
    return
        T1_os_win_s->hwnd != NULL &&
        T1_win32->IsWindowVisible(T1_os_win_s->hwnd) &&
        !T1_win32->IsIconic(T1_os_win_s->hwnd);
}

u8 T1_os_window_present(const u32 tex) {
    if (T1_os_win_s->hwnd == NULL) {
        return 0;
    }

    // waits for the display's refresh, like on macOS
    return T1_gpu_d3d11_present_tex(T1_os_win_s->hwnd, tex);
}
