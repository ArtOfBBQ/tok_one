#ifndef T1_WIN32_H
#define T1_WIN32_H

#include "T1_stdint.h"

/*
What T1 uses of the Win32 API, declared here instead of including
<windows.h>, so T1 builds without the Windows SDK's headers:
    gcc mymain.c T1.c -o myapp

Only kernel32 is linked, which every Windows program links anyway. The rest
(user32, gdi32, ...) is loaded at runtime by T1_win32_load(), the way
T1_objc.c loads Apple's frameworks, and called through T1_win32.

Names are Win32's with a T1 in front (T1WM_SIZE is WM_SIZE), so they don't
collide with <windows.h> in your own files.
*/

#if defined(_MSC_VER)
#define T1_WINAPI __stdcall
#else
#define T1_WINAPI __attribute__((stdcall))
#endif
#define T1_WIN_IMPORT __declspec(dllimport)

typedef void *    T1WinHandle; // HANDLE, HWND, HDC, HMODULE, HBRUSH, ...
typedef s32       T1WinBool;
typedef wchar_t   T1WChar; // 16 bits on Windows
typedef uintptr_t T1WParam;
typedef intptr_t  T1LParam;
typedef intptr_t  T1LResult;
typedef intptr_t (T1_WINAPI * T1WinFarProc)(void);
typedef T1LResult (T1_WINAPI * T1WndProc)(
    T1WinHandle, u32, T1WParam, T1LParam);
typedef u32 (T1_WINAPI * T1ThreadStartFn)(void *);

typedef struct { s32 left; s32 top; s32 right; s32 bottom; } T1WinRect;
typedef struct { s32 x; s32 y; } T1WinPoint;
typedef struct { u32 low; u32 high; } T1WinFileTime;

typedef struct {
    T1WinHandle hwnd;
    u32 message;
    T1WParam wparam;
    T1LParam lparam;
    u32 time;
    T1WinPoint pt;
    u32 private_;
} T1WinMsg;

typedef struct {
    u32 size;
    u32 style;
    T1WndProc wnd_proc;
    s32 cls_extra;
    s32 wnd_extra;
    T1WinHandle instance;
    T1WinHandle icon;
    T1WinHandle cursor;
    T1WinHandle background;
    const T1WChar * menu_name;
    const T1WChar * class_name;
    T1WinHandle icon_small;
} T1WinWndClassExW;

typedef struct {
    u32 length;
    u32 flags;
    u32 show_cmd;
    T1WinPoint min_position;
    T1WinPoint max_position;
    T1WinRect normal_position;
} T1WinWindowPlacement;

typedef struct {
    u32 size;
    T1WinRect monitor;
    T1WinRect work;
    u32 flags;
} T1WinMonitorInfo;

typedef struct {
    u32 attributes;
    T1WinFileTime creation_time;
    T1WinFileTime last_access_time;
    T1WinFileTime last_write_time;
    u32 size_high;
    u32 size_low;
} T1WinFileAttributeData;

typedef struct {
    u32 attributes;
    T1WinFileTime creation_time;
    T1WinFileTime last_access_time;
    T1WinFileTime last_write_time;
    u32 size_high;
    u32 size_low;
    u32 reserved0;
    u32 reserved1;
    T1WChar file_name[260];
    T1WChar alternate_file_name[14];
} T1WinFindDataW;

typedef struct {
    u32 oem_id;
    u32 page_size;
    void * min_app_address;
    void * max_app_address;
    uintptr_t active_processor_mask;
    u32 number_of_processors;
    u32 processor_type;
    u32 allocation_granularity;
    u16 processor_level;
    u16 processor_revision;
} T1WinSystemInfo;

// XINPUT_GAMEPAD and XINPUT_STATE
typedef struct {
    u16 buttons;
    u8  left_trigger;
    u8  right_trigger;
    s16 thumb_lx;
    s16 thumb_ly;
    s16 thumb_rx;
    s16 thumb_ry;
} T1XInputGamepad;

typedef struct {
    u32 packet_number;
    T1XInputGamepad gamepad;
} T1XInputState;

#define T1_WIN_INVALID_HANDLE ((T1WinHandle)(intptr_t)-1)
#define T1INVALID_FILE_ATTRIBUTES 0xFFFFFFFFu
#define T1FILE_ATTRIBUTE_DIRECTORY 0x10u
#define T1FILE_ATTRIBUTE_NORMAL 0x80u
#define T1GENERIC_READ 0x80000000u
#define T1GENERIC_WRITE 0x40000000u
#define T1FILE_SHARE_READ 0x1u
#define T1FILE_SHARE_WRITE 0x2u
#define T1MOVEFILE_REPLACE_EXISTING 0x1u
#define T1MOVEFILE_WRITE_THROUGH 0x8u
#define T1CREATE_ALWAYS 2u
#define T1OPEN_EXISTING 3u
#define T1ERROR_SUCCESS 0u
#define T1ERROR_ALREADY_EXISTS 183u
#define T1ERROR_CLASS_ALREADY_EXISTS 1410u
#define T1CP_UTF8 65001u
#define T1MAX_PATH 260
#define T1THREAD_PRIORITY_TIME_CRITICAL 15
#define T1INFINITE 0xFFFFFFFFu
#define T1WAIT_OBJECT_0 0u
#define T1S_OK 0

#define T1WM_NCCREATE 0x0081u
#define T1WM_SIZE 0x0005u
#define T1WM_KILLFOCUS 0x0008u
#define T1WM_CAPTURECHANGED 0x0215u
#define T1WM_MOVE 0x0003u
#define T1WM_CLOSE 0x0010u
#define T1WM_QUIT 0x0012u
#define T1WM_PAINT 0x000Fu
#define T1WM_ERASEBKGND 0x0014u
#define T1WM_KEYDOWN 0x0100u
#define T1WM_KEYUP 0x0101u
#define T1WM_SYSKEYDOWN 0x0104u
#define T1WM_SYSKEYUP 0x0105u
#define T1WM_SYSCHAR 0x0106u
#define T1WM_MOUSEMOVE 0x0200u
#define T1WM_LBUTTONDOWN 0x0201u
#define T1WM_LBUTTONUP 0x0202u
#define T1WM_RBUTTONDOWN 0x0204u
#define T1WM_RBUTTONUP 0x0205u
#define T1WM_MBUTTONDOWN 0x0207u
#define T1WM_MBUTTONUP 0x0208u
#define T1WM_MOUSEWHEEL 0x020Au
#define T1WM_XBUTTONDOWN 0x020Bu
#define T1WM_XBUTTONUP 0x020Cu
#define T1SIZE_MINIMIZED 1u
#define T1WHEEL_DELTA 120
#define T1XBUTTON1 1u

#define T1WS_OVERLAPPEDWINDOW 0x00CF0000u
#define T1WS_POPUP 0x80000000u
#define T1GWL_STYLE (-16)
#define T1SW_SHOW 5
#define T1SW_SHOWNORMAL 1
#define T1SM_CYSCREEN 1
#define T1SPI_GETWORKAREA 0x0030u
#define T1MONITOR_DEFAULTTOPRIMARY 1u
#define T1MONITOR_DEFAULTTONEAREST 2u
#define T1HWND_TOP ((T1WinHandle)0)
#define T1SWP_NOSIZE 0x0001u
#define T1SWP_NOMOVE 0x0002u
#define T1SWP_NOZORDER 0x0004u
#define T1SWP_FRAMECHANGED 0x0020u
#define T1SWP_NOOWNERZORDER 0x0200u
#define T1PM_REMOVE 0x0001u
#define T1QS_ALLINPUT 0x04FFu
#define T1MWMO_INPUTAVAILABLE 0x0004u
#define T1IDC_ARROW ((const T1WChar *)(uintptr_t)32512)
#define T1IDI_APPLICATION ((const T1WChar *)(uintptr_t)32512)
#define T1MAPVK_VK_TO_CHAR 2u

#define T1VK_LBUTTON 0x01
#define T1VK_RBUTTON 0x02
#define T1VK_MBUTTON 0x04
#define T1VK_BACK 0x08
#define T1VK_TAB 0x09
#define T1VK_CLEAR 0x0C
#define T1VK_RETURN 0x0D
#define T1VK_SHIFT 0x10
#define T1VK_CONTROL 0x11
#define T1VK_MENU 0x12
#define T1VK_CAPITAL 0x14
#define T1VK_KANA 0x15
#define T1VK_ESCAPE 0x1B
#define T1VK_SPACE 0x20
#define T1VK_PRIOR 0x21
#define T1VK_NEXT 0x22
#define T1VK_END 0x23
#define T1VK_HOME 0x24
#define T1VK_LEFT 0x25
#define T1VK_UP 0x26
#define T1VK_RIGHT 0x27
#define T1VK_DOWN 0x28
#define T1VK_INSERT 0x2D
#define T1VK_LWIN 0x5B
#define T1VK_RWIN 0x5C
#define T1VK_NUMPAD0 0x60
#define T1VK_NUMPAD9 0x69
#define T1VK_MULTIPLY 0x6A
#define T1VK_ADD 0x6B
#define T1VK_SUBTRACT 0x6D
#define T1VK_DECIMAL 0x6E
#define T1VK_F1 0x70
#define T1VK_F4 0x73
#define T1VK_F12 0x7B

#define T1LOGPIXELSX 88
#define T1BLACK_BRUSH 4

// SRWLOCK, a pointer-sized slot
typedef struct { void * ptr; } T1WinSRWLock;

// kernel32, linked
T1_WIN_IMPORT void T1_WINAPI AcquireSRWLockExclusive(T1WinSRWLock *);
T1_WIN_IMPORT void T1_WINAPI InitializeSRWLock(T1WinSRWLock *);
T1_WIN_IMPORT void T1_WINAPI ReleaseSRWLockExclusive(T1WinSRWLock *);
T1_WIN_IMPORT u8 T1_WINAPI TryAcquireSRWLockExclusive(T1WinSRWLock *);
T1_WIN_IMPORT T1WinBool T1_WINAPI CloseHandle(T1WinHandle);
T1_WIN_IMPORT T1WinBool T1_WINAPI CopyFileW(
    const T1WChar *, const T1WChar *, T1WinBool);
T1_WIN_IMPORT T1WinBool T1_WINAPI CreateDirectoryW(const T1WChar *, void *);
T1_WIN_IMPORT T1WinHandle T1_WINAPI CreateEventW(
    void *, T1WinBool, T1WinBool, const T1WChar *);
T1_WIN_IMPORT T1WinHandle T1_WINAPI CreateFileW(
    const T1WChar *, u32, u32, void *, u32, u32, T1WinHandle);
T1_WIN_IMPORT T1WinHandle T1_WINAPI CreateThread(
    void *, size_t, T1ThreadStartFn, void *, u32, u32 *);
T1_WIN_IMPORT T1WinBool T1_WINAPI DeleteFileW(const T1WChar *);
T1_WIN_IMPORT T1WinBool T1_WINAPI FindClose(T1WinHandle);
T1_WIN_IMPORT T1WinHandle T1_WINAPI FindFirstFileW(
    const T1WChar *, T1WinFindDataW *);
T1_WIN_IMPORT T1WinBool T1_WINAPI FindNextFileW(
    T1WinHandle, T1WinFindDataW *);
T1_WIN_IMPORT T1WinBool T1_WINAPI FlushFileBuffers(T1WinHandle);
T1_WIN_IMPORT u32 T1_WINAPI GetCurrentDirectoryW(u32, T1WChar *);
T1_WIN_IMPORT u32 T1_WINAPI GetCurrentThreadId(void);
T1_WIN_IMPORT u32 T1_WINAPI GetEnvironmentVariableW(
    const T1WChar *, T1WChar *, u32);
// the 2nd parameter is GetFileExInfoStandard, 0
T1_WIN_IMPORT T1WinBool T1_WINAPI GetFileAttributesExW(
    const T1WChar *, s32, void *);
T1_WIN_IMPORT u32 T1_WINAPI GetFileAttributesW(const T1WChar *);
T1_WIN_IMPORT T1WinHandle T1_WINAPI GetCurrentThread(void);
T1_WIN_IMPORT T1WinBool T1_WINAPI GetFileSizeEx(T1WinHandle, s64 *);
T1_WIN_IMPORT u32 T1_WINAPI GetLastError(void);
T1_WIN_IMPORT u32 T1_WINAPI GetModuleFileNameW(T1WinHandle, T1WChar *, u32);
T1_WIN_IMPORT T1WinHandle T1_WINAPI GetModuleHandleW(const T1WChar *);
T1_WIN_IMPORT u64 T1_WINAPI GetTickCount64(void);
T1_WIN_IMPORT T1WinFarProc T1_WINAPI GetProcAddress(
    T1WinHandle, const char *);
T1_WIN_IMPORT void T1_WINAPI GetSystemInfo(T1WinSystemInfo *);
T1_WIN_IMPORT T1WinHandle T1_WINAPI LoadLibraryW(const T1WChar *);
T1_WIN_IMPORT T1WinBool T1_WINAPI MoveFileExW(
    const T1WChar *, const T1WChar *, u32);
T1_WIN_IMPORT s32 T1_WINAPI MultiByteToWideChar(
    u32, u32, const char *, s32, T1WChar *, s32);
T1_WIN_IMPORT T1WinBool T1_WINAPI QueryPerformanceCounter(s64 *);
T1_WIN_IMPORT T1WinBool T1_WINAPI QueryPerformanceFrequency(s64 *);
T1_WIN_IMPORT T1WinBool T1_WINAPI ReadFile(
    T1WinHandle, void *, u32, u32 *, void *);
T1_WIN_IMPORT T1WinBool T1_WINAPI SetEvent(T1WinHandle);
T1_WIN_IMPORT T1WinBool T1_WINAPI SetThreadPriority(T1WinHandle, s32);
T1_WIN_IMPORT void T1_WINAPI Sleep(u32);
T1_WIN_IMPORT u32 T1_WINAPI WaitForSingleObject(T1WinHandle, u32);
T1_WIN_IMPORT s32 T1_WINAPI WideCharToMultiByte(
    u32, u32, const T1WChar *, s32, char *, s32, const char *, T1WinBool *);
T1_WIN_IMPORT T1WinBool T1_WINAPI WriteFile(
    T1WinHandle, const void *, u32, u32 *, void *);

// loaded at runtime, NULL if optional and missing
typedef struct {
    // user32
    T1WinBool (T1_WINAPI * AdjustWindowRectEx)(
        T1WinRect *, u32, T1WinBool, u32);
    T1WinBool (T1_WINAPI * AttachThreadInput)(u32, u32, T1WinBool);
    T1WinBool (T1_WINAPI * BringWindowToTop)(T1WinHandle);
    T1WinBool (T1_WINAPI * ClientToScreen)(T1WinHandle, T1WinPoint *);
    T1WinHandle (T1_WINAPI * CreateWindowExW)(
        u32, const T1WChar *, const T1WChar *, u32,
        s32, s32, s32, s32,
        T1WinHandle, T1WinHandle, T1WinHandle, void *);
    T1LResult (T1_WINAPI * DefWindowProcW)(
        T1WinHandle, u32, T1WParam, T1LParam);
    T1WinBool (T1_WINAPI * DestroyWindow)(T1WinHandle);
    T1LResult (T1_WINAPI * DispatchMessageW)(const T1WinMsg *);
    T1WinBool (T1_WINAPI * GetClientRect)(T1WinHandle, T1WinRect *);
    T1WinHandle (T1_WINAPI * GetDC)(T1WinHandle);
    T1WinHandle (T1_WINAPI * GetForegroundWindow)(void);
    s16 (T1_WINAPI * GetKeyState)(s32);
    T1WinBool (T1_WINAPI * GetMonitorInfoW)(T1WinHandle, T1WinMonitorInfo *);
    s32 (T1_WINAPI * GetSystemMetrics)(s32);
    s32 (T1_WINAPI * GetWindowLongW)(T1WinHandle, s32);
    T1WinBool (T1_WINAPI * GetWindowPlacement)(
        T1WinHandle, T1WinWindowPlacement *);
    u32 (T1_WINAPI * GetWindowThreadProcessId)(T1WinHandle, u32 *);
    T1WinBool (T1_WINAPI * IsIconic)(T1WinHandle);
    T1WinBool (T1_WINAPI * IsWindowVisible)(T1WinHandle);
    T1WinHandle (T1_WINAPI * LoadCursorW)(T1WinHandle, const T1WChar *);
    T1WinHandle (T1_WINAPI * LoadIconW)(T1WinHandle, const T1WChar *);
    u32 (T1_WINAPI * MapVirtualKeyW)(u32, u32);
    T1WinHandle (T1_WINAPI * MonitorFromRect)(const T1WinRect *, u32);
    T1WinHandle (T1_WINAPI * MonitorFromWindow)(T1WinHandle, u32);
    u32 (T1_WINAPI * MsgWaitForMultipleObjectsEx)(
        u32, const T1WinHandle *, u32, u32, u32);
    T1WinBool (T1_WINAPI * PeekMessageW)(
        T1WinMsg *, T1WinHandle, u32, u32, u32);
    u16 (T1_WINAPI * RegisterClassExW)(const T1WinWndClassExW *);
    T1WinBool (T1_WINAPI * ReleaseCapture)(void);
    s32 (T1_WINAPI * ReleaseDC)(T1WinHandle, T1WinHandle);
    T1WinHandle (T1_WINAPI * SetCapture)(T1WinHandle);
    T1WinHandle (T1_WINAPI * SetFocus)(T1WinHandle);
    T1WinBool (T1_WINAPI * SetForegroundWindow)(T1WinHandle);
    s32 (T1_WINAPI * SetWindowLongW)(T1WinHandle, s32, s32);
    T1WinBool (T1_WINAPI * SetWindowPlacement)(
        T1WinHandle, const T1WinWindowPlacement *);
    T1WinBool (T1_WINAPI * SetWindowPos)(
        T1WinHandle, T1WinHandle, s32, s32, s32, s32, u32);
    T1WinBool (T1_WINAPI * ShowWindow)(T1WinHandle, s32);
    T1WinBool (T1_WINAPI * SystemParametersInfoW)(u32, u32, void *, u32);
    T1WinBool (T1_WINAPI * ValidateRect)(T1WinHandle, const T1WinRect *);
    // user32, optional: Windows 10+, Vista+, Windows 10+
    T1WinBool (T1_WINAPI * SetProcessDpiAwarenessContext)(T1WinHandle);
    T1WinBool (T1_WINAPI * SetProcessDPIAware)(void);
    u32 (T1_WINAPI * GetDpiForSystem)(void);

    // gdi32
    s32 (T1_WINAPI * GetDeviceCaps)(T1WinHandle, s32);
    T1WinHandle (T1_WINAPI * GetStockObject)(s32);

    // optional: winmm, xinput
    u32 (T1_WINAPI * timeBeginPeriod)(u32);
    u32 (T1_WINAPI * XInputGetState)(u32, T1XInputState *);
} T1Win32;

extern T1Win32 * T1_win32;

// Loads T1_win32, *fatal_error if something that isn't optional is missing
void T1_win32_load(char ** const fatal_error);

#endif // T1_WIN32_H
